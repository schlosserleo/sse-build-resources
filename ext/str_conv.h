#pragma once

#include <string>

namespace str_conv
{
	[[nodiscard]] std::wstring str_to_wstr(const std::string& a_str);
	[[nodiscard]] std::string  wstr_to_str(const std::wstring& a_str);

	[[nodiscard]] inline std::string to_native(const std::wstring& a_str)
	{
		return wstr_to_str(a_str);
	}
}
