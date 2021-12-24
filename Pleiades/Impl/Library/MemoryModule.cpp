#include <fstream>
#include <iostream>

#include <shadowgarden/users/Modules.hpp>
#include <shadowgarden/users/String.hpp>

#include "LibrarySys.hpp"
#include "Impl/Plugins/PluginManager.hpp"

SG_NAMESPACE_BEGIN;

LibraryManager::LibraryManager() noexcept :
	m_Runtime(std::make_unique<asmjit::JitRuntime>())
{ }

ILibrary* LibraryManager::ReadLibrary(const char* module_name)
{
#if BOOST_WINDOWS
	HMODULE pMod;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module_name, &pMod))
		return nullptr;
	else
		return new LibraryImpl(pMod, false);
#elif BOOST_LINUX
	void* pMod = dlopen(module_name, RTLD_NOW | RTLD_NOLOAD);
	if (pMod)
	{
		dlclose(pMod);
		return new LibraryImpl(pMod, false);
	}
	else return nullptr;
#else
	return nullptr;
#endif
}

ILibrary* LibraryManager::OpenLibrary(const char* module_name)
{
#if BOOST_WINDOWS
	HMODULE pMod = LoadLibraryA(module_name);
#elif BOOST_LINUX
	void* pMod = dlopen(module_name, RTLD_NOW);
#else
	constexpr void* pMod = nullptr;
#endif
	if (!pMod)
		return nullptr;
	else return new LibraryImpl(pMod, true);
}

ILibrary* LibraryManager::OpenLibrary(const wchar_t* module_name)
{
#if BOOST_WINDOWS
	HMODULE hMod = LoadLibraryW(module_name);
	if (!hMod)
		return nullptr;
	else return new LibraryImpl(hMod, true);
#elif BOOST_LINUX
	return OpenLibrary(StringTransform<std::string>(module_name).c_str());
#else
	return nullptr;
#endif
}

LibraryImpl::~LibraryImpl()
{
	if (!m_ModuleHandle || !m_ShouldFreeModule)
		return;

#if BOOST_WINDOWS
	FreeLibrary(static_cast<HMODULE>(m_ModuleHandle.get()));
#elif BOOST_LINUX
	dlclose(m_ModuleHandle.get());
#endif
}

SG_NAMESPACE_END;