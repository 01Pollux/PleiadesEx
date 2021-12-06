#pragma once

#include "Interfaces/InterfacesSys.hpp"
#include "Interfaces/PluginSys.hpp"

namespace SG = ShadowGarden;
namespace ShadowGarden
{
	class IPluginImpl : public IPlugin
	{
	public:
		IPluginImpl() noexcept;
		// Inherited via IPlugin
		bool OnPluginLoad(SG::IPluginManager* ifacemgr) final;

	public:
		/// <summary>
		/// Called when plugin is first loaded
		/// </summary>
		/// <returns>
		/// true if plugin is all set to be loaded, false otherwise
		/// </returns>
		virtual bool OnPluginLoad2(SG::IPluginManager* ifacemgr) { return false; }
	};

	extern IPlugin*			ThisPlugin;
}


#ifdef SG_USING_PL_MANAGER
namespace ShadowGarden { extern IPluginManager* PluginManager; }
#endif

#ifdef SG_USING_LIBRARY
#include "Interfaces/LibrarySys.hpp"
namespace ShadowGarden { extern ILibraryManager* LibManager; }
#endif

#ifdef SG_USING_LOGGER
#include "Interfaces/Logger.hpp"
namespace ShadowGarden { extern ILogger* Logger; }
#endif

#ifdef SG_USING_EVENT_MANAGER
#include "Interfaces/EventManager.hpp"
namespace ShadowGarden { extern IEventManager* EventManager; }
#endif

#ifdef SG_USING_DETOUR_MANAGER
#include "Interfaces/HookArgs.hpp"
#include "Interfaces/HooksManager.hpp"
namespace ShadowGarden { extern IDetoursManager* DetourManager; }
#endif

#ifdef SG_USING_IMGUI
#include "Interfaces/ImGui.hpp"
namespace ShadowGarden { extern IImGuiLoader* ImGuiLoader; }
#endif

#ifdef SG_USING_PROFILER
#include "User/Profiler.hpp"
#endif


namespace ShadowGarden
{
	template<typename _Ty>
	inline void GetInterface_NoFail(const char* name, IPluginManager* pFactory, _Ty& iface)
	{
		pFactory->RequestInterface(name, std::bit_cast<IInterface**>(&iface));
	}
	
	template<typename _Ty>
	inline bool GetInterface(const char* name, IPluginManager* pFactory, _Ty& iface)
	{
		if (!pFactory->RequestInterface(name, std::bit_cast<IInterface**>(&iface)))
		{
#ifdef SG_USING_LOGGER
			SG_LOG_FATAL({
				SG_MESSAGE("Failed to find interface."),
				SG_LOGARG("Name", name)
			});
#endif
			return false;
		}
		return true;
	}
}
