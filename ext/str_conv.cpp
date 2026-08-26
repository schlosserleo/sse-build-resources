#include "str_conv.h"

#include <Windows.h>

namespace str_conv
{
	std::wstring str_to_wstr(const std::string& a_str)
	{
		if (a_str.empty())
		{
			return {};
		}

		auto len = ::MultiByteToWideChar(CP_UTF8, 0, a_str.data(), static_cast<int>(a_str.size()), nullptr, 0);
		if (len <= 0)
		{
			return {};
		}

		std::wstring result(static_cast<std::size_t>(len), L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, a_str.data(), static_cast<int>(a_str.size()), result.data(), len);

		return result;
	}

	std::string wstr_to_str(const std::wstring& a_str)
	{
		if (a_str.empty())
		{
			return {};
		}

		auto len = ::WideCharToMultiByte(CP_UTF8, 0, a_str.data(), static_cast<int>(a_str.size()), nullptr, 0, nullptr, nullptr);
		if (len <= 0)
		{
			return {};
		}

		std::string result(static_cast<std::size_t>(len), '\0');
		::WideCharToMultiByte(CP_UTF8, 0, a_str.data(), static_cast<int>(a_str.size()), result.data(), len, nullptr, nullptr);

		return result;
	}
}
