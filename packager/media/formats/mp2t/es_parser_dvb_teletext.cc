// Copyright 2020 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/media/formats/mp2t/es_parser_dvb_teletext.h"

#include "packager/media/base/bit_reader.h"
#include "packager/media/base/text_stream_info.h"
#include "packager/media/base/timestamp.h"
#include "packager/media/formats/mp2t/mp2t_common.h"

namespace shaka {
namespace media {
namespace mp2t {

//-----------------------------------------------------------------------------
// From various original sources.
//-----------------------------------------------------------------------------

namespace {
// Static table to swap bits in a byte.
const uint8_t REVERSE_8[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0,
    0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4,
    0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
    0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca,
    0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6,
    0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
    0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
    0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
    0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3,
    0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7,
    0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf,
    0x3f, 0xbf, 0x7f, 0xff};
}

void EsParserDvbTeletext::HandlePESPacket(const uint8_t* data,
                                          size_t size,
                                          int64_t pts) {
  // The first byte is a data_identifier.
  if (size < 1 || *data < TELETEXT_PES_FIRST_EBU_DATA_ID ||
      *data > TELETEXT_PES_LAST_EBU_DATA_ID) {
    // Not a valid Teletext PES packet.
    return;
  }
  data++;
  size--;

  // Loop on all data units inside the PES payload.
  while (size >= 2) {
    // Data unit header (2 bytes).
    const TeletextDataUnitId unitId = TeletextDataUnitId(data[0]);
    const uint8_t unitSize = data[1];
    size -= 2;
    data += 2;

    // Filter Teletext packets.
    if (unitSize <= size && unitSize == TELETEXT_PACKET_SIZE &&
        (unitId == TeletextDataUnitId::NON_SUBTITLE ||
         unitId == TeletextDataUnitId::SUBTITLE)) {
      // Reverse bitwise endianess of each data byte via lookup table, ETS 300
      // 706, chapter 7.1.
      uint8_t pkt[TELETEXT_PACKET_SIZE];
      for (int i = 0; i < unitSize; ++i) {
        pkt[i] = REVERSE_8[data[i]];
      }

      std::vector<std::shared_ptr<TextSample>> samples;
      teletext_parser_.ProcessTeletextPacket(pid(), context_, unitId, pkt, pts,
                                             &samples);

      for (auto& sample : samples) {
        emit_sample_cb_.Run(sample);
      }
    }

    // Point to next data unit.
    size -= unitSize;
    data += unitSize;
  }
}

EsParserDvbTeletext::EsParserDvbTeletext(
    uint32_t pid,
    const NewStreamInfoCB& new_stream_info_cb,
    const EmitTextSampleCB& emit_sample_cb,
    const uint8_t* descriptor,
    size_t descriptor_length)
    : EsParser(pid),
      new_stream_info_cb_(new_stream_info_cb),
      emit_sample_cb_(emit_sample_cb) {
  /*
  if (!ParseSubtitlingDescriptor(descriptor, descriptor_length, &languages_)) {
    LOG(WARNING) << "Error parsing subtitling descriptor";
  }
  */
}

EsParserDvbTeletext::~EsParserDvbTeletext() = default;

bool EsParserDvbTeletext::Parse(const uint8_t* buf,
                                int size,
                                int64_t pts,
                                int64_t dts) {
  if (!sent_info_) {
    sent_info_ = true;
    std::shared_ptr<TextStreamInfo> info = std::make_shared<TextStreamInfo>(
        pid(), kMpeg2Timescale, kInfiniteDuration, kCodecText,
        /* codec_string= */ "", /* codec_config= */ "", /* width= */ 0,
        /* height= */ 0, /* language= */ "");

    // for (const auto& pair : languages_) {
    //   info->AddSubStream(pair.first, {pair.second});
    // }

    new_stream_info_cb_.Run(info);
  }

  // TODO: Handle buffering and multiple reads?  All content so far has been
  // a whole segment, so it may not be needed.
  return ParseInternal(buf, size, pts);
}

bool EsParserDvbTeletext::Flush() {
  std::vector<std::shared_ptr<TextSample>> samples;
  teletext_parser_.Flush(pid(), context_, &samples);

  for (auto& sample : samples) {
    emit_sample_cb_.Run(sample);
  }

  return true;
}

void EsParserDvbTeletext::Reset() {
  context_ = DvbTeletextParser::PIDContext{};
}

bool EsParserDvbTeletext::ParseInternal(const uint8_t* data,
                                        size_t size,
                                        int64_t pts) {
  HandlePESPacket(data, size, pts);

  return true;
}

}  // namespace mp2t
}  // namespace media
}  // namespace shaka
