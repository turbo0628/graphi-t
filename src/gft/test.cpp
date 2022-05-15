#include <exception>
#include "gft/test.hpp"
#include "gft/log.hpp"

namespace liong {

namespace test {

TestRegistry TEST_REG;


TestRegistry::TestRegistry() {}

TestRegistry& TestRegistry::get_inst() {
  static std::unique_ptr<TestRegistry> inner;
  if (inner == nullptr) {
    inner = std::make_unique<TestRegistry>();
  }
  return *inner;
}

int TestRegistry::reg(
  const std::string& name,
  std::function<void()>&& func
) {
  tests.emplace(name, Entry { func });
  return 0;
}

TestReport TestRegistry::run_all() {
  const auto& tests = get_inst().tests;

  TestReport out {};
  if (tests.empty()) {
    log::info("no test to run");
    return out;
  } else {
    log::info("scheduling ", tests.size(), " tests");
  }

  for (const auto& pair : tests) {
    log::info("[", pair.first, "]");
    log::push_indent();
    try {
      pair.second.f();
    } catch (const std::exception& e) {
      log::error("unit test '", pair.first, "' threw an exception");
      log::error(e.what());
      ++out.nfail;
      continue;
    } catch (...) {
      log::error("unit test '", pair.first, "' threw an illiterate exception");
      ++out.nfail;
      continue;
    }
    log::pop_indent();
    ++out.nsucc;
  }
  return out;
}

} // namespace test

} // namespace liong
