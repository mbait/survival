#include "read_ini.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string>

namespace platform
{
namespace
{

std::string to_lower(std::string_view s)
{
	std::string out;
	out.reserve(s.size());
	for (char c : s)
	{
		out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
	}
	return out;
}

std::string_view trim(std::string_view s)
{
	auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
	while (!s.empty() && is_space(static_cast<unsigned char>(s.front())))
	{
		s.remove_prefix(1);
	}
	while (!s.empty() && is_space(static_cast<unsigned char>(s.back())))
	{
		s.remove_suffix(1);
	}
	return s;
}

// Walk the file once; invoke `visit(section_lower, key_lower, raw_value)`
// for every key/value pair. Stops as soon as `visit` returns true.
template <typename Visit> bool walk(const std::filesystem::path& file, Visit&& visit)
{
	std::ifstream in(file);
	if (!in)
	{
		return false;
	}

	std::string current_section_lower;
	std::string line;

	while (std::getline(in, line))
	{
		// Strip an optional UTF-8 BOM from the first line.
		if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
		    static_cast<unsigned char>(line[1]) == 0xBB &&
		    static_cast<unsigned char>(line[2]) == 0xBF)
		{
			line.erase(0, 3);
		}
		// Tolerate CRLF files on a Unix host.
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		std::string_view sv = trim(line);
		if (sv.empty() || sv.front() == ';' || sv.front() == '#')
		{
			continue;
		}

		if (sv.front() == '[' && sv.back() == ']')
		{
			current_section_lower = to_lower(trim(sv.substr(1, sv.size() - 2)));
			continue;
		}

		const auto eq = sv.find('=');
		if (eq == std::string_view::npos)
		{
			continue;
		}
		const auto key = trim(sv.substr(0, eq));
		const auto value = trim(sv.substr(eq + 1));

		if (visit(current_section_lower, to_lower(key), value))
		{
			return true;
		}
	}
	return false;
}

} // namespace

std::string ini_get_string(const std::filesystem::path& file, std::string_view section,
                           std::string_view key, std::string_view fallback)
{
	const std::string section_lower = to_lower(section);
	const std::string key_lower = to_lower(key);
	std::string result(fallback);

	walk(file,
	     [&](const std::string& s, const std::string& k, std::string_view v)
	     {
		     if (s == section_lower && k == key_lower)
		     {
			     result.assign(v);
			     return true;
		     }
		     return false;
	     });

	return result;
}

int ini_get_int(const std::filesystem::path& file, std::string_view section, std::string_view key,
                int fallback)
{
	const std::string raw = ini_get_string(file, section, key, std::string_view {});
	if (raw.empty())
	{
		return fallback;
	}
	int out = 0;
	const auto* first = raw.data();
	const auto* last = raw.data() + raw.size();
	const auto [ptr, ec] = std::from_chars(first, last, out);
	if (ec != std::errc {} || ptr == first)
	{
		return fallback;
	}
	return out;
}

} // namespace platform
