#include <iostream>
#include <print>
#include <string_view>

#include "core/setting/setting.hpp"
#include "lua-meta-file.hpp"
#include "reference-json.hpp"

// usage: generate-setting-meta-file [--format=lua|json]
//   lua  (default): meta file for LuaLS
//   json          : reference for the documentation website
int main(int argc, char** argv) {
  std::string_view format = "lua";
  for (int i = 1; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const std::string_view     arg           = argv[i];
    constexpr std::string_view kFormatOption = "--format=";
    if (arg.starts_with(kFormatOption)) {
      format = arg.substr(kFormatOption.size());
    } else {
      std::println(std::cerr, "unknown argument: {}", arg);
      return 1;
    }
  }

  const yabe::core::setting::Setting setting;
  if (format == "lua") {
    std::cout << yabe::tools::generate_lua_meta_file(setting.reference());
  } else if (format == "json") {
    std::cout << yabe::tools::generate_reference_json(setting.reference());
  } else {
    std::println(
        std::cerr, "unknown format: {} (expected lua or json)", format);
    return 1;
  }
  return 0;
}
