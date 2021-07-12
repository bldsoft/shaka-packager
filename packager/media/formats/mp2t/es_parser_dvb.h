// Copyright 2020 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_H_
#define PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_H_

#include <unordered_map>
#include <vector>

#include "packager/base/callback.h"
#include "packager/media/base/byte_queue.h"
#include "packager/media/formats/dvb/dvb_sub_parser.h"
#include "packager/media/formats/mp2t/es_parser.h"
#include "packager/ocr/public/text_extractor_builder.h"

namespace shaka {
namespace media {
namespace mp2t {

class EsParserDvb : public EsParser {
 public:
  EsParserDvb(
      uint32_t pid,
      const NewStreamInfoCB& new_stream_info_cb,
      const EmitTextSampleCB& emit_sample_cb,
      const uint8_t* descriptor,
      size_t descriptor_length,
      std::shared_ptr<const ocr::TextExtractorBuilder> text_extracor_builder);
  ~EsParserDvb() override;

  // EsParser implementation.
  bool Parse(const uint8_t* buf, int size, int64_t pts, int64_t dts) override;
  bool Flush() override;
  void Reset() override;
  bool Init() override;

 private:
  EsParserDvb(const EsParserDvb&) = delete;
  EsParserDvb& operator=(const EsParserDvb&) = delete;

  bool ParseInternal(const uint8_t* data, size_t size, int64_t pts);

  inline DvbSubParser& GetOrCreatePageSubParser(uint16_t page_id);
  inline const std::string& GetPageLanguage(uint16_t page_id) const;

  // Callbacks:
  // - to signal a new audio configuration,
  // - to send ES buffers.
  NewStreamInfoCB new_stream_info_cb_;
  EmitTextSampleCB emit_sample_cb_;

  // A map of page_id to parser.
  std::unordered_map<uint16_t, DvbSubParser> parsers_;
  // A map of page_id to language.
  std::unordered_map<uint16_t, std::string> languages_;
  // A container with all languages
  std::vector<std::string> all_languages_;
  // A text extractor builder(ocr)
  std::shared_ptr<const ocr::TextExtractorBuilder> text_extracor_builder_;
};

}  // namespace mp2t
}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_FORMATS_MP2T_ES_PARSER_DVB_H_
