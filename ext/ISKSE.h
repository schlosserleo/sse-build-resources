#pragma once

#include "IMisc.h"
#include "IOS.h"

#include <common/IDebugLog.h>
#include <skse64/PluginAPI.h>
#include <skse64_common/BranchTrampoline.h>
#include <skse64_common/skse_version.h>

#include <ShlObj.h>

enum class SKSEInterfaceFlags : std::uint8_t
{
	kMessaging = 1ui8 << 0,
	kTask = 1ui8 << 1,
	kPapyrus = 1ui8 << 2,
	kSerialization = 1ui8 << 3,
	kObjectInterface = 1ui8 << 4,
	kScaleform = 1ui8 << 5,
	kTrampoline = 1ui8 << 6,
};

DEFINE_ENUM_CLASS_BITWISE(SKSEInterfaceFlags);

enum class TrampolineID : std::uint8_t
{
	kBranch,
	kLocal
};

template <SKSEInterfaceFlags _InterfaceFlags, std::size_t _TrampolineBranch = 0, std::size_t _TrampolineLocal = 0>
class ISKSEBase
{
	struct TrampolineUsageInfo
	{
		std::size_t used;
		std::size_t total;
	};

public:
	ISKSEBase() = default;

	ISKSEBase(const ISKSEBase&) = delete;
	ISKSEBase(ISKSEBase&&) = delete;
	ISKSEBase& operator=(const ISKSEBase&) = delete;
	ISKSEBase& operator=(ISKSEBase&&) = delete;

	[[nodiscard]] bool Query(const SKSEInterface* a_skse, PluginInfo* a_info);
	bool OpenLog(std::uint32_t a_version = 0);
	[[nodiscard]] bool CreateTrampolines(const SKSEInterface* a_skse);
	[[nodiscard]] bool QueryInterfaces(const SKSEInterface* a_skse);

	template <class T, class interface_type = stl::strip_type<T>>
	[[nodiscard]] inline constexpr interface_type* GetInterface() const
	{
		return reinterpret_cast<interface_type*>(m_interfaces[interface_type::INTERFACE_TYPE]);
	}

	[[nodiscard]] inline constexpr BranchTrampoline& GetTrampoline(TrampolineID a_id)
	{
		return m_trampolines[stl::underlying(a_id)];
	}

	[[nodiscard]] inline std::size_t GetTrampolineSize(TrampolineID a_id) const
	{
		return m_trampolines[stl::underlying(a_id)].Size();
	}

	[[nodiscard]] inline std::size_t GetTrampolineRemain(TrampolineID a_id) const
	{
		return m_trampolines[stl::underlying(a_id)].Remain();
	}

	[[nodiscard]] auto GetTrampolineUsage(TrampolineID a_id) const noexcept
	{
		auto total = GetTrampolineSize(a_id);
		auto rem = GetTrampolineRemain(a_id);

		return TrampolineUsageInfo{ total - rem, total };
	}

	[[nodiscard]] inline constexpr PluginHandle GetPluginHandle() const noexcept
	{
		return m_pluginHandle;
	}

	[[nodiscard]] inline constexpr std::uint32_t GetRuntimeVersion() const noexcept
	{
		return m_runtimeVersion;
	}

	[[nodiscard]] inline constexpr HMODULE ModuleHandle() const noexcept
	{
		return m_moduleHandle;
	}

	inline void SetModuleHandle(HMODULE a_handle)
	{
		m_moduleHandle = a_handle;
	}

	inline void SetPluginHandle(PluginHandle a_handle)
	{
		m_pluginHandle = a_handle;
	}

protected:
	virtual void OnLogOpen() = 0;
	virtual const char* GetLogPath(std::uint32_t a_version) const = 0;
	virtual const char* GetPluginName() const = 0;
	virtual std::uint32_t GetPluginVersion() const = 0;
	virtual bool CheckRuntimeVersion(std::uint32_t a_version) const = 0;
	virtual bool CheckInterfaceVersion(
		std::uint32_t a_interfaceID,
		std::uint32_t a_interfaceVersion,
		std::uint32_t a_compiledInterfaceVersion) const;

private:
	template <class T, class interface_type = stl::strip_type<T>>
	bool QueryInterface(const SKSEInterface* a_skse);

	template <TrampolineID _Trampoline>
	bool InitializeTrampoline(std::size_t a_size);

	static std::size_t GetAlignedTrampolineSize(size_t maxSize);

	HMODULE m_moduleHandle{ nullptr };
	PluginHandle m_pluginHandle{ kPluginHandle_Invalid };
	std::uint32_t m_runtimeVersion{ 0 };

	void* m_interfaces[SKSEInterfaceType::kInterface_Max]{ nullptr };
	BranchTrampoline m_trampolines[2];
};

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::CheckInterfaceVersion(std::uint32_t a_interfaceID, std::uint32_t a_interfaceVersion, std::uint32_t a_compiledInterfaceVersion) const
{
	return a_interfaceVersion >= a_compiledInterfaceVersion;
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
template <
	class T,
	class interface_type>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::QueryInterface(const SKSEInterface* a_skse)
{
	auto iface = reinterpret_cast<interface_type*>(a_skse->QueryInterface(interface_type::INTERFACE_TYPE));
	if (iface == nullptr)
	{
		if (interface_type::INTERFACE_TYPE == SKSETrampolineInterface::INTERFACE_TYPE)
		{
			gLog.Warning("Trampoline interface not found");
			return true;
		}
		else
		{
			gLog.FatalError(
				"Could not get interface id %d",
				interface_type::INTERFACE_TYPE);

			return false;
		}
	}

	if (!CheckInterfaceVersion(
			interface_type::INTERFACE_TYPE,
			iface->interfaceVersion,
			interface_type::kInterfaceVersion))
	{
		gLog.FatalError(
			"Interface %d too old (%d expected %d)",
			interface_type::INTERFACE_TYPE,
			iface->interfaceVersion,
			interface_type::kInterfaceVersion);

		return false;
	}
	else
	{
		m_interfaces[interface_type::INTERFACE_TYPE] = reinterpret_cast<void*>(iface);

		return true;
	}
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
std::size_t ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::GetAlignedTrampolineSize(size_t a_maxSize)
{
	auto alignTo = GetAllocGranularity();
	if (alignTo == 0)
	{
		return a_maxSize;
	}

	auto r = a_maxSize % alignTo;
	return r ? a_maxSize + (alignTo - r) : a_maxSize;
}

template <SKSEInterfaceFlags _InterfaceFlags, std::size_t _TrampolineBranch, std::size_t _TrampolineLocal>
template <TrampolineID _Trampoline>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::InitializeTrampoline(std::size_t a_size)
{
	auto& trampoline = GetTrampoline(_Trampoline);

	auto iface = GetInterface<SKSETrampolineInterface>();

	if (iface)
	{
		void* base(nullptr);

		if constexpr (_Trampoline == TrampolineID::kBranch)
		{
			base = iface->AllocateFromBranchPool(m_pluginHandle, a_size);
		}
		else if constexpr (_Trampoline == TrampolineID::kLocal)
		{
			base = iface->AllocateFromLocalPool(m_pluginHandle, a_size);
		}

		if (base != nullptr)
		{
			trampoline.SetBase(a_size, base);
			return true;
		}
	}

	return trampoline.Create(GetAlignedTrampolineSize(a_size));
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::Query(const SKSEInterface* a_skse, PluginInfo* a_info)
{
	OpenLog(a_skse->runtimeVersion);

	a_info->infoVersion = PluginInfo::kInfoVersion;
	a_info->name = GetPluginName();
	a_info->version = GetPluginVersion();

	if (a_skse->isEditor)
	{
		gLog.FatalError("Loaded in editor, marking as incompatible");
		return false;
	}

	if (!CheckRuntimeVersion(a_skse->runtimeVersion))
	{
		gLog.FatalError(
			"Unsupported runtime version %d.%d.%d.%d",
			GET_EXE_VERSION_MAJOR(a_skse->runtimeVersion),
			GET_EXE_VERSION_MINOR(a_skse->runtimeVersion),
			GET_EXE_VERSION_BUILD(a_skse->runtimeVersion),
			GET_EXE_VERSION_SUB(a_skse->runtimeVersion));

		return false;
	}

	m_pluginHandle = a_skse->GetPluginHandle();

	return true;
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::OpenLog(std::uint32_t a_version)
{
	m_runtimeVersion = a_version;

	auto logPath = GetLogPath(a_version);
	if (logPath && !gLog.IsOpen())
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, logPath);
		gLog.SetLogLevel(LogLevel::Debug);

		OnLogOpen();

		return gLog.IsOpen();
	}
	else
	{
		return false;
	}
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::QueryInterfaces(const SKSEInterface* a_skse)
{
	for (auto& e : m_interfaces)
	{
		e = nullptr;
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kMessaging) == SKSEInterfaceFlags::kMessaging)
	{
		if (!QueryInterface<SKSEMessagingInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kTask) == SKSEInterfaceFlags::kTask)
	{
		if (!QueryInterface<SKSETaskInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kPapyrus) == SKSEInterfaceFlags::kPapyrus)
	{
		if (!QueryInterface<SKSEPapyrusInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kSerialization) == SKSEInterfaceFlags::kSerialization)
	{
		if (!QueryInterface<SKSESerializationInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kObjectInterface) == SKSEInterfaceFlags::kObjectInterface)
	{
		if (!QueryInterface<SKSEObjectInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kScaleform) == SKSEInterfaceFlags::kScaleform)
	{
		if (!QueryInterface<SKSEScaleformInterface>(a_skse))
		{
			return false;
		}
	}

	if constexpr ((_InterfaceFlags & SKSEInterfaceFlags::kTrampoline) == SKSEInterfaceFlags::kTrampoline)
	{
		if (!QueryInterface<SKSETrampolineInterface>(a_skse))
		{
			return false;
		}
	}

	return true;
}

template <
	SKSEInterfaceFlags _InterfaceFlags,
	std::size_t _TrampolineBranch,
	std::size_t _TrampolineLocal>
bool ISKSEBase<
	_InterfaceFlags,
	_TrampolineBranch,
	_TrampolineLocal>::CreateTrampolines(const SKSEInterface* a_skse)
{
	if constexpr (_TrampolineBranch > 0)
	{
		if (!InitializeTrampoline<TrampolineID::kBranch>(_TrampolineBranch))
		{
			gLog.FatalError("Could not create branch trampoline");
			return false;
		}
	}

	if constexpr (_TrampolineLocal > 0)
	{
		if (!InitializeTrampoline<TrampolineID::kLocal>(_TrampolineLocal))
		{
			gLog.FatalError("Could not create codegen trampoline");
			return false;
		}
	}

	return true;
}