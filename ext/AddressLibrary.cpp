#include "AddressLibrary.h"
#include "PerfCounter.h"

#pragma warning(disable: 4073)
#pragma init_seg(lib)

IAL IAL::m_Instance;

bool IAL::get_ver(
	int (&a_parts)[4],
	std::uint64_t& a_out)
{
	std::uint64_t result{ 0 };

	if (!m_database->GetExecutableVersion(
			a_parts[0],
			a_parts[1],
			a_parts[2],
			a_parts[3]))
	{
		return false;
	}

	for (int i = 0; i < 4; i++)
	{
		if (a_parts[i] > 0xFFFF)
			return false;

		result <<= 16;
		result |= a_parts[i];
	}

	a_out = result;

	return true;
}

IAL::IAL() :
	m_database(std::make_unique<VersionDb>())
{
	PerfCounter pc;

	int parts[4]{ 0 };

	if (!get_ver(parts, m_ver))
	{
		return;
	}

	m_isAE = m_ver > 0x0001000500610000;

	m_tLoadStart = pc.Query();

	const int format = m_ver >= 0x0001000700000000 ? 5 : m_isAE ? 2 : 1;

	m_isLoaded = m_database->Load(
		format,
		parts[0],
		parts[1],
		parts[2],
		parts[3]);

	// tolerate databases losslessly re-encoded to the old format
	// (some third-party packages ship format-2 conversions)
	if (!m_isLoaded && format == 5)
	{
		m_isLoaded = m_database->Load(
			2,
			parts[0],
			parts[1],
			parts[2],
			parts[3]);
	}

	m_tLoadEnd = pc.Query();
}

void IAL::Release()
{
	m_Instance.m_isLoaded = false;
	m_Instance.m_database.reset();
}
