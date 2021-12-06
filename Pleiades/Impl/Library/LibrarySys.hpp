#pragma once

#include "Interfaces/LibrarySys.hpp"
#include "User/IntPtr.hpp"
#include <boost/system.hpp>
#include <asmjit/asmjit.h>


SG_NAMESPACE_BEGIN;

class LibraryImpl : public ILibrary
{
	friend class LibraryManager;
public:
	LibraryImpl(IntPtr ptr, bool should_free) noexcept :
		m_ModuleHandle(ptr),
		m_ShouldFreeModule(should_free)
	{ };

public:
	~LibraryImpl() override;

	IntPtr FindByName(const std::string& name) override;

	IntPtr FindBySignature(const std::string& signature) override;

	IntPtr FindBySignature(const SG::ILibrary::SignaturePredicate& signature) override;

	IntPtr FindByString(const std::string& str) override;

public:
	IntPtr GetModule() noexcept { return m_ModuleHandle; }

private:
	IntPtr	m_ModuleHandle;
	bool	m_ShouldFreeModule;
};


class LibraryManager : public ILibraryManager
{
public:
	LibraryManager() noexcept;

public:
	// Inherited via ILibraryManager
	ILibrary* ReadLibrary(const char* module_name) override;

	ILibrary* OpenLibrary(const char* module_name) override;

	ILibrary* OpenLibrary(const wchar_t* module_name) override;

	bool GoToDirectory(PlDirType type, const char* dir, char* path, size_t max_len) override;

	IGameData* OpenGameData(IPlugin* pPlugin) override;

	const std::string& GetHostName() override;

	asmjit::JitRuntime* GetRuntime() override;

	std::string GetLastError() override;

	Profiler::Manager* GetProfiler() override;

public:
	void BuildDirectories();

	void SetHostName(const std::string& name);

private:
	std::string m_HostName;
	std::unique_ptr<asmjit::JitRuntime> m_Runtime;
};

extern LibraryManager lib_manager;

SG_NAMESPACE_END;