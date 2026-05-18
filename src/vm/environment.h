#pragma once

#include <string>
#include <string_view>

namespace torture::vm {

std::string currentEnvironmentFingerprint();
std::string environmentSummary();
bool matchesCurrentEnvironment(std::string_view fingerprint);
std::string shortFingerprint(std::string_view fingerprint);

} // namespace torture::vm
