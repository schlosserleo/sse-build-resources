#pragma once

#include <Windows.h>
#include <fstream>
#include <shlwapi.h>
#include <stdio.h>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "Shlwapi.lib")

#include <tsl/sparse_map.h>

class VersionDb
{
public:
	VersionDb() { clear(); }
	~VersionDb() = default;

private:
	using map_type = tsl::sparse_map<unsigned long long, unsigned long long>;

	map_type _data;
	//std::map<unsigned long long, unsigned long long> _rdata;
	int _ver[4];
	std::string _verStr;
	//std::string _moduleName;
	unsigned long long _base = 0;

	template <typename T>
	static T read(std::ifstream& file)
	{
		T v;
		file.read((char*)&v, sizeof(T));
		return v;
	}

	static void* ToPointer(unsigned long long v)
	{
		return (void*)v;
	}

	static unsigned long long FromPointer(void* ptr)
	{
		return (unsigned long long)ptr;
	}

	static bool ParseVersionFromString(const char* ptr, int& major, int& minor, int& revision, int& build)
	{
		return sscanf_s(
				   ptr,
				   "%d.%d.%d.%d",
				   &major,
				   &minor,
				   &revision,
				   &build) == 4 &&
		       ((major != 1 && major != 0) || minor != 0 || revision != 0 || build != 0);
	}

public:
	//const std::string& GetModuleName() const { return _moduleName; }
	const std::string& GetLoadedVersionString() const { return _verStr; }

	const map_type& GetOffsetMap() const
	{
		return _data;
	}

	void* FindAddressById(unsigned long long id) const
	{
		unsigned long long b = _base;
		if (b == 0)
			return NULL;

		unsigned long long offset = 0;
		if (!FindOffsetById(id, offset))
			return NULL;

		return reinterpret_cast<void*>(b + offset);
	}

	bool FindOffsetById(unsigned long long id, unsigned long long& result) const
	{
		auto itr = _data.find(id);
		if (itr != _data.end())
		{
			result = itr->second;
			return true;
		}
		return false;
	}

	bool GetExecutableVersion(int& major, int& minor, int& revision, int& build) const
	{
		TCHAR szVersionFile[MAX_PATH];
		GetModuleFileName(NULL, szVersionFile, MAX_PATH);

		DWORD verHandle = 0;
		//UINT size = 0;
		//LPBYTE lpBuffer = NULL;
		DWORD verSize = GetFileVersionInfoSize(szVersionFile, &verHandle);

		if (verSize != NULL)
		{
			std::unique_ptr<CHAR[]> verData = std::make_unique<CHAR[]>(verSize);

			if (GetFileVersionInfo(szVersionFile, verHandle, verSize, verData.get()))
			{
				{
					char* vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData.get(), "\\StringFileInfo\\040904B0\\ProductVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							return true;
						}
					}
				}

				{
					char* vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData.get(), "\\StringFileInfo\\040904B0\\FileVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	void GetLoadedVersion(int& major, int& minor, int& revision, int& build) const
	{
		major = _ver[0];
		minor = _ver[1];
		revision = _ver[2];
		build = _ver[3];
	}

	void clear()
	{
		_data.clear();
		for (auto& e : _ver)
		{
			e = 0;
		}
		_base = 0;
	}

	bool Load()
	{
		int major, minor, revision, build;

		if (!GetExecutableVersion(major, minor, revision, build))
			return false;

		return Load(2, major, minor, revision, build);
	}

	bool Load(int a_format, int major, int minor, int revision, int build)
	{
		clear();

		_data.reserve(800000);

		char fileName[256];
		_snprintf_s(
			fileName,
			_TRUNCATE,
			a_format >= 2 ?
                "Data\\SKSE\\Plugins\\versionlib-%d-%d-%d-%d.bin" :
                "Data\\SKSE\\Plugins\\version-%d-%d-%d-%d.bin",
			major,
			minor,
			revision,
			build);

		std::ifstream file(fileName, std::ios::binary);
		if (!file.good())
			return false;

		int format = read<int>(file);

		if (format != a_format)
			return false;

		if (format == 5)
		{
			return LoadV5(file);
		}

		for (int i = 0; i < 4; i++)
		{
			_ver[i] = read<int>(file);
		}

		{
			char verName[64];
			_snprintf_s(verName, _TRUNCATE, "%d.%d.%d.%d", _ver[0], _ver[1], _ver[2], _ver[3]);
			_verStr = verName;
		}

		int tnLen = read<int>(file);

		if (tnLen < 0 || tnLen >= 0x10000)
			return false;

		if (tnLen > 0)
		{
			file.seekg(tnLen, file.cur);
		}

		_base = (unsigned long long)GetModuleHandleA(NULL);

		int ptrSize = read<int>(file);

		int addrCount = read<int>(file);

		unsigned char type, low, high;
		unsigned char b1, b2;
		unsigned short w1, w2;
		unsigned int d1, d2;
		unsigned long long q1, q2;
		unsigned long long pvid = 0;
		unsigned long long poffset = 0;
		unsigned long long tpoffset;

		for (int i = 0; i < addrCount; i++)
		{
			type = read<unsigned char>(file);
			low = type & 0xF;
			high = type >> 4;

			switch (low)
			{
			case 0:
				q1 = read<unsigned long long>(file);
				break;
			case 1:
				q1 = pvid + 1;
				break;
			case 2:
				b1 = read<unsigned char>(file);
				q1 = pvid + b1;
				break;
			case 3:
				b1 = read<unsigned char>(file);
				q1 = pvid - b1;
				break;
			case 4:
				w1 = read<unsigned short>(file);
				q1 = pvid + w1;
				break;
			case 5:
				w1 = read<unsigned short>(file);
				q1 = pvid - w1;
				break;
			case 6:
				w1 = read<unsigned short>(file);
				q1 = w1;
				break;
			case 7:
				d1 = read<unsigned int>(file);
				q1 = d1;
				break;
			default:
				{
					clear();
					return false;
				}
			}

			tpoffset = (high & 8) != 0 ? (poffset / (unsigned long long)ptrSize) : poffset;

			switch (high & 7)
			{
			case 0:
				q2 = read<unsigned long long>(file);
				break;
			case 1:
				q2 = tpoffset + 1;
				break;
			case 2:
				b2 = read<unsigned char>(file);
				q2 = tpoffset + b2;
				break;
			case 3:
				b2 = read<unsigned char>(file);
				q2 = tpoffset - b2;
				break;
			case 4:
				w2 = read<unsigned short>(file);
				q2 = tpoffset + w2;
				break;
			case 5:
				w2 = read<unsigned short>(file);
				q2 = tpoffset - w2;
				break;
			case 6:
				w2 = read<unsigned short>(file);
				q2 = w2;
				break;
			case 7:
				d2 = read<unsigned int>(file);
				q2 = d2;
				break;
			default:
				throw std::exception();
			}

			if ((high & 8) != 0)
				q2 *= (unsigned long long)ptrSize;

			_data[q1] = q2;

			poffset = q2;
			pvid = q1;
		}

		return true;
	}

	// format 5 (1.7.99+): flat table -> u32 version[4], char name[64],
	// u32 ptrSize, u32 reserved, u32 count, u32 offsets[count] indexed by id
	bool LoadV5(std::ifstream& file)
	{
		for (int i = 0; i < 4; i++)
		{
			_ver[i] = read<int>(file);
		}

		{
			char verName[64];
			_snprintf_s(verName, _TRUNCATE, "%d.%d.%d.%d", _ver[0], _ver[1], _ver[2], _ver[3]);
			_verStr = verName;
		}

		char moduleName[64];
		file.read(moduleName, sizeof(moduleName));

		_base = (unsigned long long)GetModuleHandleA(NULL);

		int ptrSize = read<int>(file);
		(void)ptrSize;

		int reserved = read<int>(file);
		(void)reserved;

		int addrCount = read<int>(file);

		if (addrCount < 0)
		{
			clear();
			return false;
		}

		for (int i = 0; i < addrCount; i++)
		{
			unsigned int offset = read<unsigned int>(file);

			if (!file.good())
			{
				clear();
				return false;
			}

			if (offset != 0)
			{
				_data[(unsigned long long)i] = offset;
			}
		}

		return true;
	}

	bool Dump(const std::string& path)
	{
		std::ofstream f = std::ofstream(path.c_str());
		if (!f.good())
			return false;

		for (auto itr = _data.begin(); itr != _data.end(); itr++)
		{
			f << std::dec;
			f << itr->first;
			f << '\t';
			f << std::hex;
			f << itr->second;
			f << '\n';
		}

		return true;
	}
};
