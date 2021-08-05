// # bin2c
// @PENGUINLIONG
#include "assert.hpp"
#include "util.hpp"

using namespace liong;

int main(int argc, const char** argv) {
  assert(argc == 3);

  auto src_path = argv[1];
  std::vector<uint8_t> buf;
  auto bin = util::load_file(src_path);

  std::stringstream ss {};
  ss << "// This is a generated file; changes may be overwritten.\n"
    "const uint8_t data[] = {";
  for (auto x : buf) { ss << x << ","; }
  ss << "};";

  auto dst_path = argv[2];
  auto c = ss.str();
  util::save_file(dst_path, c.data(), c.size());

  return 0;
}
