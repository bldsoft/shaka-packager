// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGER_OCR_TEXT_EXTRACTOR_BUILDER_H_
#define PACKAGER_OCR_TEXT_EXTRACTOR_BUILDER_H_

#include <memory>
#include <string>
#include <vector>

#include "packager/export.h"
#include "packager/ocr/public/status.h"
#include "packager/ocr/public/text_extractor.h"

namespace shaka {
namespace ocr {

// Text extractors builder interface
class SHAKA_EXPORT TextExtractorBuilder {
 public:
  TextExtractorBuilder() noexcept = default;
  virtual ~TextExtractorBuilder() = default;

  TextExtractorBuilder(TextExtractorBuilder&&) noexcept = default;
  TextExtractorBuilder& operator=(TextExtractorBuilder&&) noexcept = default;

  /// Builds text extractor object.
  /// @param languages is container with languages.
  /// @param [out] extractor is pointer on resulting object.
  /// @return status of the object creation.
  virtual Status CreateTextExtractor(
      const std::vector<std::string>& languages,
      std::unique_ptr<TextExtractor>* extractor) const noexcept = 0;

 private:
  TextExtractorBuilder(const TextExtractorBuilder&) = delete;
  TextExtractorBuilder& operator=(const TextExtractorBuilder&) = delete;
};
}  // namespace ocr
}  // namespace shaka

#endif
