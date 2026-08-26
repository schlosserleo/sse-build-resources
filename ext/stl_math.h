#pragma once

#include <algorithm>

namespace stl
{
	// normalize a_value into [0, 1] over [a_min, a_max], clamped
	[[nodiscard]] inline constexpr float normalize_safe_clamp(
		float a_value,
		float a_min,
		float a_max) noexcept
	{
		if (!(a_max > a_min))
		{
			return 0.0f;
		}

		return std::clamp((a_value - a_min) / (a_max - a_min), 0.0f, 1.0f);
	}

	template <class TO, class FROM>
	[[nodiscard]] inline TO unrestricted_cast(FROM a_from) noexcept
	{
		union
		{
			FROM f;
			TO   t;
		} u{ a_from };

		return u.t;
	}
}
