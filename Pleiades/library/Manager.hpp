#pragma once

#include <boost/system.hpp>
#include <asmjit/asmjit.h>
#include <px/interfaces/LibrarySys.hpp>
#include <px/IntPtr.hpp>

class LibraryManager : public px::ILibraryManager
{
public:
	// Inherited via ILibraryManager
	virtual px::ILibrary* ReadLibrary(const std::string_view module_name);

	virtual px::ILibrary* OpenLibrary(const std::wstring_view module_name, bool manual_map);

	virtual std::string GoToDirectory(px::PlDirType type, std::string_view extra_path = "");

	virtual px::IGameData* OpenGameData(px::IPlugin* pPlugin);

	virtual std::string_view GetHostName();

	virtual asmjit::JitRuntime* GetRuntime();

	virtual std::string GetLastError();

	virtual px::profiler::manager* GetProfiler();

public:
	LibraryManager();

	void BuildDirectories();
	void SetHostName(const std::string& name)
	{
		m_HostName.assign(name);
	}

private:
	std::string m_HostName;
	std::unique_ptr<asmjit::JitRuntime> m_Runtime;
};

PX_NAMESPACE_BEGIN();
inline LibraryManager lib_manager;
PX_NAMESPACE_END();
