#pragma once

#include <boost/system.hpp>
#include <asmjit/asmjit.h>
#include <px/interfaces/LibrarySys.hpp>
#include <px/IntPtr.hpp>

class LibraryImpl : public px::ILibrary
{
	friend class LibraryManager;
public:
	LibraryImpl(px::IntPtr ptr, bool should_free, bool is_manual_mapped) noexcept :
		m_ModuleHandle(ptr),
		m_ShouldFreeModule(should_free),
		m_IsManualMapped(is_manual_mapped)
	{ };

public:
	~LibraryImpl() override;

	px::IntPtr FindByName(std::string_view name) override;

	px::IntPtr FindBySignature(std::string_view signature) override;

	px::IntPtr FindBySignature(const px::ILibrary::SignaturePredicate& signature) override;

	px::IntPtr FindByString(std::string_view str) override;

public:
	px::IntPtr GetModule() noexcept { return m_ModuleHandle; }

private:
	px::IntPtr	m_ModuleHandle;
	bool	m_ShouldFreeModule : 1;
	bool	m_IsManualMapped : 1;
};
