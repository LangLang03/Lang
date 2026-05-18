#pragma once

#include <string>
#include <string_view>

namespace torture {

std::string sha256Hex(std::string_view input);

} // namespace torture
