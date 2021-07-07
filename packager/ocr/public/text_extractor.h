// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGER_OCR_TEXT_EXTRACTOR_H_
#define PACKAGER_OCR_TEXT_EXTRACTOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "packager/export.h"
#include "packager/ocr/public/status.h"

namespace shaka {
namespace ocr {

// Interface for a text extractor and static methods to build the objects
class TextExtractor {
 public:
  TextExtractor() noexcept = default;
  virtual ~TextExtractor() = default;

  TextExtractor(TextExtractor&&) noexcept = default;
  TextExtractor& operator=(TextExtractor&&) noexcept = default;

  /// Performs recognition of text from the png image, and returns the utf8
  /// result.
  /// @param png_image is the bytes of png image.
  /// @param [out] text is result of the text recognition(utf8).
  /// @return status of the text recognition.
  virtual Status RecognizeTextUtf8FromPng(
      const std::vector<std::uint8_t>& png_image,
      std::string& text) noexcept = 0;

 private:
  TextExtractor(const TextExtractor&) = delete;
  TextExtractor& operator=(const TextExtractor&) = delete;
};
}  // namespace ocr
}  // namespace shaka

#endif
