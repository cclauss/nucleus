/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// Testing C++ utilities.
#ifndef THIRD_PARTY_NUCLEUS_TESTING_TEST_UTILS_H_
#define THIRD_PARTY_NUCLEUS_TESTING_TEST_UTILS_H_

#include <gmock/gmock-generated-matchers.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>


#include "tensorflow/core/platform/test.h"
#include "nucleus/io/reader_base.h"
#include "nucleus/protos/reads.pb.h"
#include "nucleus/protos/reference.pb.h"
#include "nucleus/vendor/statusor.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/io/record_reader.h"
#include "tensorflow/core/lib/io/record_writer.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/types.h"

namespace nucleus {

using tensorflow::StringPiece;
using tensorflow::string;
using tensorflow::uint64;

constexpr char kBioTFCoreTestDataDir[] = "nucleus/testdata";

// N.B. this will be set to "" in OSS.
constexpr char kDefaultWorkspace[] = "";


// Simple getter for test files in the right testdata path.
// This uses JoinPath, so no leading or trailing "/" are necessary.
string GetTestData(
    tensorflow::StringPiece path,
    tensorflow::StringPiece test_data_dir = kBioTFCoreTestDataDir);

// Returns a path to a temporary file with filename in the appropriate test
// directory.
string MakeTempFile(tensorflow::StringPiece filename);

// Reads all of the records from path into a vector of parsed Proto. Path
// must point to a TFRecord formatted file.
template <typename Proto>
std::vector<Proto> ReadProtosFromTFRecord(tensorflow::StringPiece path) {
  tensorflow::Env* env = tensorflow::Env::Default();
  std::unique_ptr<tensorflow::RandomAccessFile> read_file;
  TF_CHECK_OK(env->NewRandomAccessFile(path.ToString(), &read_file));
  tensorflow::io::RecordReader reader(read_file.get());
  std::vector<Proto> results;
  uint64 offset = 0;
  string data;
  while (reader.ReadRecord(&offset, &data).ok()) {
    Proto proto;
    CHECK(proto.ParseFromString(data)) << "Failed to parse proto";
    results.push_back(proto);
  }
  return results;
}

// Writes all `protos` to a TFRecord formatted file.
template <typename Proto>
void WriteProtosToTFRecord(const std::vector<Proto>& protos,
                           tensorflow::StringPiece output_path) {
  std::unique_ptr<tensorflow::WritableFile> file;
  TF_CHECK_OK(tensorflow::Env::Default()->NewWritableFile(
      output_path.ToString(), &file));
  tensorflow::io::RecordWriter record_writer(file.get());
  for (const Proto& proto : protos) {
    TF_CHECK_OK(record_writer.WriteRecord(proto.SerializeAsString()));
  }
}

// Creates a vector of ContigInfos with specified `names` and `positions`
// representing pos_in_fast. `names` and `positions` need to have the same
// number of elements.
std::vector<nucleus::genomics::v1::ContigInfo> CreateContigInfos(
    const std::vector<string>& names, const std::vector<int>& positions);

// A matcher to help us do Pointwise double expectations with a provided
// precision via DoubleNear.
MATCHER_P(PointwiseDoubleNear, abs_error, "") {
  using RhsType = decltype(::testing::get<1>(arg));
  ::testing::Matcher<RhsType> matcher =
      ::testing::DoubleNear(::testing::get<1>(arg), abs_error);
  return matcher.Matches(::testing::get<0>(arg));
}

// A matcher to test if a floating point value is finite.
MATCHER(IsFinite, "") { return std::isfinite(arg); }

// Adapter to extract an iterable into a vector for examination in test code
// from a StatusOr<std::shared_ptr<Iterable<Record>>>.
template <class Record>
std::vector<Record> as_vector(
    const StatusOr<std::shared_ptr<Iterable<Record>>>& it) {
  TF_CHECK_OK(it.status());
  return as_vector(it.ValueOrDie());
}

// Adapter to extract an iterable into a vector for examination in test code.
template<class Record>
std::vector<Record> as_vector(const std::shared_ptr<Iterable<Record>>& it) {
  std::vector<Record> records;
  for (const StatusOr<Record*> value_status : it) {
    records.push_back(*value_status.ValueOrDie());
  }
  return records;
}

// Creates a test Read.
//
// The read has reference_name chr, start of start, aligned_sequence of bases,
// and cigar element parsed from cigar_elements, which is vector of standard
// CIGAR element string values like {"5M", "2I", "3M"} which is 5 bp matches,
// 2 bp insertion, and 3 bp matches. The read has base qualities set to 30 and
// a mapping quality of 90.
::nucleus::genomics::v1::Read MakeRead(
    const string& chr, int start, const string& bases,
    const std::vector<string>& cigar_elements);

}  // namespace nucleus

#endif  // THIRD_PARTY_NUCLEUS_TESTING_TEST_UTILS_H_
