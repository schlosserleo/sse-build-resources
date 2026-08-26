#pragma once

#include "PerfCounter.h"
#include "versiondb.h"

#include <memory>

class IAL
{
public:
	static void Release();

	inline static constexpr bool IsLoaded() noexcept
	{
		return m_Instance.m_isLoaded;
	}

	inline static constexpr float GetLoadTime() noexcept
	{
		return IPerfCounter::delta(
			m_Instance.m_tLoadStart,
			m_Instance.m_tLoadEnd);
	}

	inline static constexpr long long GetLoadStart() noexcept
	{
		return m_Instance.m_tLoadStart;
	}

	inline static constexpr long long GetLoadEnd() noexcept
	{
		return m_Instance.m_tLoadEnd;
	}

	inline static constexpr bool HasBadQuery() noexcept
	{
		return m_Instance.m_hasBadQuery;
	}

	inline static constexpr unsigned long long GetLastBadQueryID() noexcept
	{
		return m_Instance.m_lastBadQueryID;
	}

	inline static std::size_t Size()
	{
		return m_Instance.m_database->GetOffsetMap().size();
	}

	inline static constexpr auto ver() noexcept
	{
		return m_Instance.m_ver;
	}

	template <class T = std::uintptr_t>
	inline static constexpr T Addr(
		unsigned long long a_id_se,
		unsigned long long a_id_ae,
		std::ptrdiff_t a_offset_se,
		std::ptrdiff_t a_offset_ae)
	{
		auto res = AddrImpl(
			a_id_se,
			a_id_ae,
			a_offset_se,
			a_offset_ae);

		if constexpr (std::is_same_v<T, std::uintptr_t>)
		{
			return res;
		}
		else
		{
			return reinterpret_cast<T>(res);
		}
	}

	// resolve without setting the bad-query flag; returns null for ids
	// absent from the database (e.g. classes removed in newer runtimes)
	template <class T = std::uintptr_t>
	inline static T AddrSoft(
		unsigned long long a_id_se,
		unsigned long long a_id_ae)
	{
		auto id = m_Instance.m_isAE ?
                      a_id_ae :
                      a_id_se;

		std::uintptr_t res = 0;

		if (id != 0)
		{
			res = reinterpret_cast<std::uintptr_t>(
				m_Instance.m_database->FindAddressById(id));
		}

		if constexpr (std::is_same_v<T, std::uintptr_t>)
		{
			return res;
		}
		else
		{
			return reinterpret_cast<T>(res);
		}
	}

	template <class T = std::uintptr_t>
	inline static constexpr T Addr(
		unsigned long long a_id_se,
		unsigned long long a_id_ae)
	{
		auto res = AddrImpl(
			a_id_se,
			a_id_ae);

		if constexpr (std::is_same_v<T, std::uintptr_t>)
		{
			return res;
		}
		else
		{
			return reinterpret_cast<T>(res);
		}
	}

	/*static std::uintptr_t Offset(
		unsigned long long a_id_se,
		unsigned long long a_id_ae)
	{
		auto id = m_Instance.m_isAE ?
                      a_id_ae :
                      a_id_se;

		unsigned long long r;
		if (!m_Instance.m_database.FindOffsetById(id, r))
		{
			m_Instance.m_hasBadQuery = true;
			return std::uintptr_t(0);
		}
		return static_cast<std::uintptr_t>(r);
	}*/

	template <class T>
	class Address
	{
	public:
		Address() = delete;

		Address(
			unsigned long long a_id_se,
			unsigned long long a_id_ae,
			std::ptrdiff_t a_offset_se,
			std::ptrdiff_t a_offset_ae) :
			m_offset(IAL::AddrImpl(
				a_id_se,
				a_id_ae,
				a_offset_se,
				a_offset_ae))
		{
		}

		Address(
			unsigned long long a_id_se,
			unsigned long long a_id_ae) :
			m_offset(IAL::AddrImpl(
				a_id_se,
				a_id_ae))
		{
		}

		inline constexpr operator T() const noexcept
		{
			if constexpr (std::is_same_v<T, std::uintptr_t>)
			{
				return m_offset;
			}
			else
			{
				return reinterpret_cast<T>(
					static_cast<std::uintptr_t>(m_offset));
			}
		}

		template <class Tp = std::uintptr_t>
		inline constexpr Tp As() const noexcept
		{
			if constexpr (std::is_same_v<T, Tp>)
			{
				return m_offset;
			}
			else
			{
				return reinterpret_cast<Tp>(
					static_cast<std::uintptr_t>(m_offset));
			}
		}

	private:
		std::uintptr_t m_offset;
	};

	static_assert(sizeof(Address<void*>) == 0x8);

	static inline constexpr bool IsAE() noexcept
	{
		return m_Instance.m_isAE;
	}

private:
	SKMP_NOINLINE static std::uintptr_t AddrImpl(
		unsigned long long a_id_se,
		unsigned long long a_id_ae)
	{
		auto id = m_Instance.m_isAE ?
                      a_id_ae :
                      a_id_se;

		if (id == 0)
		{
			return 0;
		}

		auto addr = m_Instance.m_database->FindAddressById(id);
		if (!addr)
		{
			m_Instance.m_hasBadQuery = true;
			m_Instance.m_lastBadQueryID = id;
			return 0;
		}

		return reinterpret_cast<std::uintptr_t>(addr);
	}

	SKMP_NOINLINE static std::uintptr_t AddrImpl(
		unsigned long long a_id_se,
		unsigned long long a_id_ae,
		std::ptrdiff_t a_offset_se,
		std::ptrdiff_t a_offset_ae)
	{
		unsigned long long id;
		std::ptrdiff_t offset;

		if (m_Instance.m_isAE)
		{
			id = a_id_ae;
			offset = a_offset_ae;
		}
		else
		{
			id = a_id_se;
			offset = a_offset_se;
		}

		if (id == 0)
		{
			return 0;
		}

		auto addr = m_Instance.m_database->FindAddressById(id);
		if (!addr)
		{
			m_Instance.m_hasBadQuery = true;
			m_Instance.m_lastBadQueryID = id;
			return 0;
		}

		return reinterpret_cast<std::uintptr_t>(addr) + offset;
	}

	bool IAL::get_ver(int (&a_parts)[4], std::uint64_t& a_out);

	IAL();
	~IAL() = default;

	bool m_isLoaded{ false };
	bool m_hasBadQuery{ false };
	unsigned long long m_lastBadQueryID{ 0 };
	long long m_tLoadStart{ 0 };
	long long m_tLoadEnd{ 0 };

	bool m_isAE{ false };

	std::uint64_t m_ver{ 0 };

	std::unique_ptr<VersionDb> m_database;

	static IAL m_Instance;
};
