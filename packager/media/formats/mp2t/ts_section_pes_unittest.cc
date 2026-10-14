// Copyright 2026 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "packager/media/base/timestamp.h"
#include "packager/media/formats/mp2t/es_parser.h"
#include "packager/media/formats/mp2t/ts_section_pes.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

namespace shaka {
namespace media {
namespace mp2t {

namespace {

const int64_t kRollover = 1LL << 33;

class MockEsParser : public EsParser {
 public:
  MockEsParser() : EsParser(0) {}

  MOCK_METHOD4(Parse,
               bool(const uint8_t* buf, int size, int64_t pts, int64_t dts));
  MOCK_METHOD0(Flush, bool());
  MOCK_METHOD0(Reset, void());
};

void AppendTimestamp(uint8_t prefix, int64_t timestamp,
                     std::vector<uint8_t>* pes) {
  pes->push_back(static_cast<uint8_t>((prefix << 4) |
                                      (((timestamp >> 30) & 0x7) << 1) | 1));
  pes->push_back(static_cast<uint8_t>(timestamp >> 22));
  pes->push_back(static_cast<uint8_t>(((timestamp >> 15) & 0x7f) << 1 | 1));
  pes->push_back(static_cast<uint8_t>(timestamp >> 7));
  pes->push_back(static_cast<uint8_t>((timestamp & 0x7f) << 1 | 1));
}

// Video PES with a one byte payload. A negative |dts| means PTS only.
std::vector<uint8_t> CreateVideoPes(int64_t pts, int64_t dts) {
  const bool has_dts = dts >= 0;
  const uint8_t header_data_length = has_dts ? 10 : 5;
  const int pes_packet_length = 3 + header_data_length + 1;
  std::vector<uint8_t> pes = {
      0x00, 0x00, 0x01, 0xE0,
      static_cast<uint8_t>(pes_packet_length >> 8),
      static_cast<uint8_t>(pes_packet_length),
      0x80,
      static_cast<uint8_t>(has_dts ? 0xC0 : 0x80),
      header_data_length,
  };
  AppendTimestamp(has_dts ? 0x3 : 0x2, pts, &pes);
  if (has_dts)
    AppendTimestamp(0x1, dts, &pes);
  pes.push_back(0x00);
  return pes;
}

}  // namespace

// GStreamer mpegtsmux only writes DTS when it differs from PTS, so a DTS can
// first appear after PTS has already wrapped around 2^33.
TEST(TsSectionPesTest, DtsUnrolledAfterPtsOnlyRollover) {
  std::unique_ptr<MockEsParser> mock_es_parser(new MockEsParser);
  {
    InSequence s;
    EXPECT_CALL(*mock_es_parser, Parse(_, 1, kRollover - 3003, kNoTimestamp))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_es_parser, Parse(_, 1, kRollover + 3003, kNoTimestamp))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_es_parser,
                Parse(_, 1, kRollover + 9009, kRollover + 6006))
        .WillOnce(Return(true));
  }
  TsSectionPes ts_section_pes(std::move(mock_es_parser));

  for (const auto& pes : {CreateVideoPes(kRollover - 3003, -1),
                          CreateVideoPes(3003, -1),
                          CreateVideoPes(9009, 6006)}) {
    ASSERT_TRUE(ts_section_pes.Parse(true, pes.data(),
                                     static_cast<int>(pes.size())));
  }
}

TEST(TsSectionPesTest, BFramesAcrossRollover) {
  const int64_t kFrameDuration = 3003;
  const int64_t kBase = kRollover - 5 * kFrameDuration;
  const int kDisplayIndex[] = {0, 3, 1, 2, 6, 4, 5, 9, 7, 8};

  std::unique_ptr<MockEsParser> mock_es_parser(new MockEsParser);
  std::vector<std::vector<uint8_t>> pes_list;
  {
    InSequence s;
    for (size_t i = 0; i < arraysize(kDisplayIndex); ++i) {
      const int64_t dts = kBase + i * kFrameDuration;
      const int64_t pts = kBase + (kDisplayIndex[i] + 1) * kFrameDuration;
      // Like GStreamer mpegtsmux, write DTS only when it differs from PTS.
      const bool has_dts = pts != dts;
      EXPECT_CALL(*mock_es_parser,
                  Parse(_, 1, pts, has_dts ? dts : kNoTimestamp))
          .WillOnce(Return(true));
      pes_list.push_back(CreateVideoPes(pts, has_dts ? dts : -1));
    }
  }
  TsSectionPes ts_section_pes(std::move(mock_es_parser));

  for (const auto& pes : pes_list) {
    ASSERT_TRUE(ts_section_pes.Parse(true, pes.data(),
                                     static_cast<int>(pes.size())));
  }
}

}  // namespace mp2t
}  // namespace media
}  // namespace shaka
