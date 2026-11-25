#pragma once

#include <string>

#include "core/setting/reference.hpp"

namespace yabe::tools {
/// generates the reference JSON used by the documentation website
std::string generate_reference_json(
    const core::setting::reference::Reference& reference);
}  // namespace yabe::tools
