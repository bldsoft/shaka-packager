// Copyright 2017 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/file/callback_file.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "packager/file/file.h"
#include "packager/file/file_closer.h"

using testing::_;
using testing::Eq;
using testing::Invoke;
using testing::MockFunction;
using testing::Return;
using testing::StrEq;
using testing::WithArgs;

namespace shaka {
namespace {

const uint8_t kBuffer[] = {1, 2, 3, 4, 5, 6, 7, 8};
const size_t kBufferSize = sizeof(kBuffer);
const size_t kSizeLessThanBufferSize = kBufferSize - 2;
const size_t kSizeLargerThanBufferSize = kBufferSize + 2;
const char kBufferLabel[] = "some name";
const int kFileError = -10;

}  // namespace

TEST(CallbackFileTest, ReadSatisfied) {
  MockFunction<int64_t(const std::string& name, void* buffer, uint64_t length)>
      mock_read_func;
  BufferCallbackParams callback_params;
  callback_params.read_func = mock_read_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  const size_t size = kSizeLessThanBufferSize;

  EXPECT_CALL(mock_read_func, Call(StrEq(kBufferLabel), _, size))
      .WillOnce(WithArgs<1, 2>(Invoke([](void* buffer, uint64_t size) {
        size_t size_to_copy = std::min(static_cast<size_t>(size), kBufferSize);
        memcpy(buffer, kBuffer, size_to_copy);
        return size_to_copy;
      })));

  std::unique_ptr<File, FileCloser> reader(File::Open(file_name.c_str(), "r"));
  ASSERT_TRUE(reader);
  uint8_t read_buffer[size];
  ASSERT_EQ(static_cast<int64_t>(size), reader->Read(read_buffer, size));
  EXPECT_EQ(0, memcmp(kBuffer, read_buffer, size));
}

TEST(CallbackFileTest, ReadNotSatisfied) {
  MockFunction<int64_t(const std::string& name, void* buffer, uint64_t length)>
      mock_read_func;
  BufferCallbackParams callback_params;
  callback_params.read_func = mock_read_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  const size_t size = kSizeLargerThanBufferSize;

  EXPECT_CALL(mock_read_func, Call(StrEq(kBufferLabel), _, size))
      .WillOnce(WithArgs<1, 2>(Invoke([](void* buffer, uint64_t size) {
        size_t size_to_copy = std::min(static_cast<size_t>(size), kBufferSize);
        memcpy(buffer, kBuffer, size_to_copy);
        return size_to_copy;
      })));

  std::unique_ptr<File, FileCloser> reader(File::Open(file_name.c_str(), "r"));
  ASSERT_TRUE(reader);
  uint8_t read_buffer[size];
  ASSERT_EQ(static_cast<int64_t>(kBufferSize), reader->Read(read_buffer, size));
  EXPECT_EQ(0, memcmp(kBuffer, read_buffer, kBufferSize));
}

TEST(CallbackFileTest, ReadFailed) {
  MockFunction<int64_t(const std::string& name, void* buffer, uint64_t length)>
      mock_read_func;
  BufferCallbackParams callback_params;
  callback_params.read_func = mock_read_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_read_func, Call(StrEq(kBufferLabel), _, _))
      .WillOnce(WithArgs<1, 2>(
          Invoke([](void* buffer, uint64_t size) { return kFileError; })));

  std::unique_ptr<File, FileCloser> reader(File::Open(file_name.c_str(), "r"));
  ASSERT_TRUE(reader);
  uint8_t read_buffer[kBufferSize];
  ASSERT_EQ(kFileError, reader->Read(read_buffer, kBufferSize));
}

TEST(CallbackFileTest, ReadFunctionNotDefined) {
  BufferCallbackParams callback_params;
  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  std::unique_ptr<File, FileCloser> reader(File::Open(file_name.c_str(), "r"));
  ASSERT_TRUE(reader);
  uint8_t read_buffer[kBufferSize];
  ASSERT_EQ(-1, reader->Read(read_buffer, kBufferSize));
}

TEST(CallbackFileTest, WriteSatisfied) {
  MockFunction<int64_t(const std::string& name, const void* buffer,
                       uint64_t length)>
      mock_write_func;
  BufferCallbackParams callback_params;
  callback_params.write_func = mock_write_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_write_func,
              Call(StrEq(kBufferLabel), Eq(kBuffer), kBufferSize))
      .WillOnce(Return(kBufferSize));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(static_cast<int64_t>(kBufferSize),
            writer->Write(kBuffer, kBufferSize));
}

TEST(CallbackFileTest, WriteFailed) {
  MockFunction<int64_t(const std::string& name, const void* buffer,
                       uint64_t length)>
      mock_write_func;
  BufferCallbackParams callback_params;
  callback_params.write_func = mock_write_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_write_func,
              Call(StrEq(kBufferLabel), Eq(kBuffer), kBufferSize))
      .WillOnce(Return(kFileError));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(kFileError, writer->Write(kBuffer, kBufferSize));
}

TEST(CallbackFileTest, WriteFunctionNotDefined) {
  BufferCallbackParams callback_params;
  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(-1, writer->Write(kBuffer, kBufferSize));
}

TEST(CallbackFileTest, SizeSatisfied) {
  MockFunction<int64_t(const std::string& name)>
      mock_size_func;
  BufferCallbackParams callback_params;
  callback_params.size_func = mock_size_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_size_func,
              Call(StrEq(kBufferLabel)))
      .WillOnce(Return(static_cast<int64_t>(0)));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(static_cast<int64_t>(0), writer->Size());
}

TEST(CallbackFileTest, SizeFailed) {
  MockFunction<int64_t(const std::string& name)>
      mock_size_func;
  BufferCallbackParams callback_params;
  callback_params.size_func = mock_size_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_size_func,
              Call(StrEq(kBufferLabel)))
      .WillOnce(Return(static_cast<int64_t>(kFileError)));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(static_cast<int64_t>(kFileError), writer->Size());
}

TEST(CallbackFileTest, SizeFunctionNotDefined) {
  BufferCallbackParams callback_params;
  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(-1, writer->Size());
}

TEST(CallbackFileTest, DeleteSatisfied) {
  MockFunction<bool(const std::string& name)>
      mock_delete_func;
  BufferCallbackParams callback_params;
  callback_params.delete_func = mock_delete_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_delete_func,
              Call(StrEq(kBufferLabel)))
      .WillOnce(Return(true));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(true, writer->Delete(file_name.c_str()));
}

TEST(CallbackFileTest, DeleteFailed) {
  MockFunction<bool(const std::string& name)>
      mock_delete_func;
  BufferCallbackParams callback_params;
  callback_params.delete_func = mock_delete_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_delete_func,
              Call(StrEq(kBufferLabel)))
      .WillOnce(Return(false));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(false, writer->Delete(file_name.c_str()));
}

TEST(CallbackFileTest, DeleteFunctionNotDefined) {
  BufferCallbackParams callback_params;
  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(false, writer->Delete(file_name.c_str()));
}

TEST(CallbackFileTest, FlushSatisfied) {
  MockFunction<bool(const std::string& name)>
      mock_flush_func;
  BufferCallbackParams callback_params;
  callback_params.flush_func = mock_flush_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_flush_func,
              Call(StrEq(kBufferLabel)))
      .WillRepeatedly(Return(true));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(true, writer->Flush());
}

TEST(CallbackFileTest, FlushFailed) {
  MockFunction<bool(const std::string& name)>
      mock_flush_func;
  BufferCallbackParams callback_params;
  callback_params.flush_func = mock_flush_func.AsStdFunction();

  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  EXPECT_CALL(mock_flush_func,
              Call(StrEq(kBufferLabel)))
      .WillOnce(Return(false))
      .WillOnce(Return(true));

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(false, writer->Flush());
}

TEST(CallbackFileTest, FlushFunctionNotDefined) {
  BufferCallbackParams callback_params;
  std::string file_name =
      File::MakeCallbackFileName(callback_params, kBufferLabel);

  std::unique_ptr<File, FileCloser> writer(File::Open(file_name.c_str(), "w"));
  ASSERT_TRUE(writer);
  ASSERT_EQ(true, writer->Flush());
}

}  // namespace shaka
