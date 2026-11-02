#pragma once

#include <tuple>
#include <type_traits>

#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::schema {
template <FixedString Name, typename Type>
struct Parameter {
  using ValueType             = Type;
  static constexpr auto kName = Name;
};

/// reference to a Lua function stored in the Lua registry;
/// the owner must release it with Engine::unref_function()
/// `Parameters` (list of Parameter) describes arguments passed to the function
template <typename... Parameters>
struct LuaFunction {
  using Arguments = std::tuple<typename Parameters::ValueType...>;
  int ref;
};

template <typename T>
struct IsLuaFunction : std::false_type {};
template <typename... Parameters>
struct IsLuaFunction<LuaFunction<Parameters...>> : std::true_type {};
}  // namespace yabe::core::setting::schema
