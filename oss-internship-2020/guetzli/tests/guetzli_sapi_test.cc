// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>

#include "gtest/gtest.h"
#include "sandboxed_api/sandbox2/util/fileops.h"
#include "sandboxed_api/vars.h"

#include "guetzli_sandbox.h"

namespace guetzli {
namespace sandbox {
namespace tests {

namespace {

constexpr const char* kInPngFilename = "bees.png";
constexpr const char* kInJpegFilename = "nature.jpg";
constexpr const char* kPngReferenceFilename = "bees_reference.jpg";
constexpr const char* kJpegReferenceFIlename = "nature_reference.jpg";

constexpr int kDefaultQualityTarget = 95;
constexpr int kDefaultMemlimitMb = 6000;

constexpr const char* kRelativePathToTestdata =
  "/guetzli_sandboxed/tests/testdata/";

std::string GetPathToInputFile(const char* filename) {
  return absl::StrCat(getenv("TEST_SRCDIR"), kRelativePathToTestdata, filename);
}

std::string ReadFromFile(const std::string& filename) {
  std::ifstream stream(filename, std::ios::binary);

  if (!stream.is_open()) {
    return "";
  }

  std::stringstream result;
  result << stream.rdbuf();
  return result.str();
}

} // namespace

class GuetzliSapiTest : public ::testing::Test {
protected:
  void SetUp() override {
    sandbox_ = std::make_unique<GuetzliSapiSandbox>();
    sandbox_->Init().IgnoreError();
    api_ = std::make_unique<GuetzliApi>(sandbox_.get());
  }
  
  std::unique_ptr<GuetzliSapiSandbox> sandbox_;
  std::unique_ptr<GuetzliApi> api_;
};

// This test can take up to few minutes depending on your hardware
TEST_F(GuetzliSapiTest, ProcessRGB) {
  sapi::v::Fd in_fd(open(GetPathToInputFile(kInPngFilename).c_str(), 
    O_RDONLY));
  ASSERT_TRUE(in_fd.GetValue() != -1) << "Error opening input file";
  ASSERT_EQ(api_->sandbox()->TransferToSandboxee(&in_fd), absl::OkStatus())
    << "Error transfering fd to sandbox";
  ASSERT_TRUE(in_fd.GetRemoteFd() != -1) << "Error opening remote fd";
  sapi::v::Struct<ProcessingParams> processing_params;
  *processing_params.mutable_data() = {in_fd.GetRemoteFd(), 
                                      0,
                                      kDefaultQualityTarget,
                                      kDefaultMemlimitMb
  };
  sapi::v::LenVal output(0);
  auto processing_result = api_->ProcessRgb(processing_params.PtrBefore(), 
                                            output.PtrBoth());
  ASSERT_TRUE(processing_result.value_or(false)) << "Error processing rgb data";
  std::string reference_data = 
    ReadFromFile(GetPathToInputFile(kPngReferenceFilename));
  ASSERT_EQ(output.GetDataSize(), reference_data.size()) 
    << "Incorrect result data size";
  ASSERT_EQ(std::string(output.GetData(),
    output.GetData() + output.GetDataSize()), reference_data)
    << "Processed data doesn't match reference output";
}

// This test can take up to few minutes depending on your hardware
TEST_F(GuetzliSapiTest, ProcessJpeg) {
  sapi::v::Fd in_fd(open(GetPathToInputFile(kInJpegFilename).c_str(), 
    O_RDONLY));
  ASSERT_TRUE(in_fd.GetValue() != -1) << "Error opening input file";
  ASSERT_EQ(api_->sandbox()->TransferToSandboxee(&in_fd), absl::OkStatus())
    << "Error transfering fd to sandbox";
  ASSERT_TRUE(in_fd.GetRemoteFd() != -1) << "Error opening remote fd";
  sapi::v::Struct<ProcessingParams> processing_params;
  *processing_params.mutable_data() = {in_fd.GetRemoteFd(), 
                                      0,
                                      kDefaultQualityTarget,
                                      kDefaultMemlimitMb
  };
  sapi::v::LenVal output(0);
  auto processing_result = api_->ProcessJpeg(processing_params.PtrBefore(), 
                                            output.PtrBoth());
  ASSERT_TRUE(processing_result.value_or(false)) << "Error processing jpg data";
  std::string reference_data = 
    ReadFromFile(GetPathToInputFile(kJpegReferenceFIlename));
  ASSERT_EQ(output.GetDataSize(), reference_data.size()) 
    << "Incorrect result data size";
  ASSERT_EQ(std::string(output.GetData(),
    output.GetData() + output.GetDataSize()), reference_data)
    << "Processed data doesn't match reference output";
}

}  // namespace tests
}  // namespace sandbox
}  // namespace guetzli
