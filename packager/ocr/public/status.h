// Copyright 2021 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_OCR_STATUS_H_
#define PACKAGER_OCR_STATUS_H_

#include <iostream>
#include <string>

#include "packager/export.h"

namespace shaka {
namespace ocr {

class SHAKA_EXPORT Status {
 public:
  /// Status codes for the ocr subsystem.
  enum Code : int {
    // An undefined error
    kUndefinedError,
    // Not an error; returned on success
    kOK = 0,
  };

 public:
  /// Creates a "successful" status.
  Status() noexcept : code_(kOK) {}

  /// Create a status with the specified code, and error message.
  /// If "error_code == error::OK", error_message is ignored and a Status
  /// object identical to Status::OK is constructed.
  Status(int error_code, std::string error_message) noexcept
      : code_(error_code), message_(std::move(error_message)) {}

  /// Move constructor
  Status(Status&&) noexcept = default;

  /// Move assignment operator
  Status& operator=(Status&&) noexcept = default;

  /// If "ok()", stores "new_status" into *this.  If "!ok()", preserves
  /// the current "code()/message()",
  ///
  /// Convenient way of keeping track of the first status encountered.
  /// Instead of:
  ///   if (overall_status.ok()) overall_status = new_status
  /// Use:
  ///   overall_status.Update(new_status);
  bool Update(Status new_status) noexcept {
    if (ok()) {
      *this = std::move(new_status);
      return true;
    }
    return false;
  }

  bool ok() const noexcept { return code_ == kOK; }
  int code() const noexcept { return code_; }
  const std::string& message() const noexcept { return message_; }

  bool operator==(const Status& x) const noexcept {
    return code_ == x.code() && message_ == x.message();
  }
  bool operator!=(const Status& x) const noexcept { return !(*this == x); }

 private:
  int code_;
  std::string message_;

 private:
  Status(const Status&) = delete;
  Status& operator=(const Status&) = delete;
};
}  // namespace ocr
}  // namespace shaka

#endif  // PACKAGER_OCR_STATUS_H_
