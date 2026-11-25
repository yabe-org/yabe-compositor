#pragma once

#include <string>
#include <vector>

#include "core/setting/reference.hpp"

namespace yabe::tools {
/// type name used in the LuaLS meta file
std::string lua_type_name(const core::setting::reference::Type& type);

/// comma-separated `name: type` used in the LuaLS meta file
std::string lua_parameter_list(
    const std::vector<core::setting::reference::Parameter>& parameters);

/// generates the meta file for LuaLS
std::string generate_lua_meta_file(
    const core::setting::reference::Reference& reference);
}  // namespace yabe::tools
