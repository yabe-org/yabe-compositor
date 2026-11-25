#pragma once

namespace yabe::core::setting::schema {
/// identifier of a dispatcher passed to and from Lua; a negative value is
/// invalid. It is not an alias of int so that it is described as a dispatcher
struct DispatcherId {
  // NOLINTNEXTLINE(*-explicit-constructor)
  constexpr DispatcherId(int id) : id(id) {
  }
  int id;
};
}  // namespace yabe::core::setting::schema
