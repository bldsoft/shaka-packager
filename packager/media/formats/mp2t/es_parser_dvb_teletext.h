// Copyright 2020 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_TELETEXT_H_
#define PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_TELETEXT_H_

#include "packager/base/callback.h"
#include "packager/media/base/byte_queue.h"
#include "packager/media/formats/mp2t/es_parser.h"

#include "packager/media/formats/dvb/dvb_teletext_charset.h"
#include "packager/media/formats/dvb/dvb_teletext_parser.h"
#include "packager/media/formats/dvb/dvb_teletext.h"

namespace shaka {
namespace media {
namespace mp2t {

class EsParserDvbTeletext : public EsParser {
 public:
  EsParserDvbTeletext(uint32_t pid,
                      const NewStreamInfoCB& new_stream_info_cb,
                      const EmitTextSampleCB& emit_sample_cb,
                      const uint8_t* descriptor,
                      size_t descriptor_length);
  ~EsParserDvbTeletext() override;

  // EsParser implementation.
  bool Parse(const uint8_t* buf, int size, int64_t pts, int64_t dts) override;
  bool Flush() override;
  void Reset() override;

 private:
  EsParserDvbTeletext(const EsParserDvbTeletext&) = delete;
  EsParserDvbTeletext& operator=(const EsParserDvbTeletext&) = delete;

  bool ParseInternal(const uint8_t* data, size_t size, int64_t pts);
  void HandlePESPacket(const uint8_t* data, size_t size, int64_t pts);

 private:
  // Callbacks:
  // - to signal a new text configuration,
  // - to send ES buffers.
  NewStreamInfoCB new_stream_info_cb_;
  EmitTextSampleCB emit_sample_cb_;

  DvbTeletextParser::PIDContext context_;
  DvbTeletextParser teletext_parser_;

  bool sent_info_ = false;
};

}  // namespace mp2t
}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_H_
