#ifndef PLATFORM_READ_INI_H
#define PLATFORM_READ_INI_H

#include <filesystem>
#include <string>
#include <string_view>

// Minimal stand-in for Win32 GetPrivateProfileString / GetPrivateProfileInt.
//
// Matches the original semantics that the legacy main.cpp depended on:
//   - section and key match case-insensitively (Win32 behaviour),
//   - missing file, missing section, or missing key returns the fallback,
//   - lines beginning with ';' or '#' are treated as comments,
//   - whitespace around '=' and around values is trimmed,
//   - a UTF-8 BOM at the start of the file is tolerated.
//
// We deliberately keep the API tiny — the game reads exactly six keys.
namespace platform {

std::string ini_get_string(const std::filesystem::path& file,
                           std::string_view section,
                           std::string_view key,
                           std::string_view fallback);

int ini_get_int(const std::filesystem::path& file,
                std::string_view section,
                std::string_view key,
                int fallback);

}  // namespace platform

#endif  // PLATFORM_READ_INI_H
