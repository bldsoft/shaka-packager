// Copyright 2017 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/file/callback_file.h"

#include "packager/base/logging.h"

namespace shaka {

CallbackFile::CallbackFile(const char* file_name, const char* mode)
    : File(file_name), file_mode_(mode), position_(0) {}

CallbackFile::~CallbackFile() {}

bool CallbackFile::Close() {
  delete this;
  return true;
}

int64_t CallbackFile::Read(void* buffer, uint64_t length) {
  if (!callback_params_->read_func) {
    LOG(ERROR) << "Read function not defined.";
    return -1;
  }
  return callback_params_->read_func(name_, buffer, length);
}

int64_t CallbackFile::Write(const void* buffer, uint64_t length) {
  if (!callback_params_->write_func) {
    LOG(ERROR) << "Write function not defined.";
    return -1;
  }
  int64_t size = callback_params_->write_func(name_, buffer, length);
  position_ += size;
  return size;
}

int64_t CallbackFile::Size() {
  if (!callback_params_->write_func) {
    LOG(ERROR) << "Write function not defined.";
    return 0;
  }
  return callback_params_->write_func(name_, nullptr, 1);
}

bool CallbackFile::Flush() {
  // Do nothing on Flush.
  return true;
}

bool CallbackFile::Seek(uint64_t position) {
  VLOG(1) << "CallbackFile does not support Seek().";
  return false;
}

bool CallbackFile::Tell(uint64_t* position) {
  //VLOG(1) << "CallbackFile does not support Tell().";
  *position = position_;
  return true;
}

bool CallbackFile::Open() {
  if (file_mode_ != "r" && file_mode_ != "w" && file_mode_ != "rb" &&
      file_mode_ != "wb") {
    LOG(ERROR) << "CallbackFile does not support file mode " << file_mode_;
    return false;
  }
  position_ = 0;
  return ParseCallbackFileName(file_name(), &callback_params_, &name_);
}

bool CallbackFile::Delete() {
  if (!callback_params_->write_func) {
    LOG(ERROR) << "Write function not defined.";
    return false;
  }
  return callback_params_->write_func(name_, nullptr, 0) != 0;
}

bool CallbackFile::Delete(const std::string& file_name) {
  CallbackFile file(file_name.c_str(), "r");
  if(file.Open()){
    return file.Delete();
  }
  return false;
}

}  // namespace shaka
