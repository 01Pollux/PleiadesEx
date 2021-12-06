
#include <filesystem>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/predef.h>

#include "GameData.hpp"
#include "Impl/Library/LibrarySys.hpp"
#include "Interfaces/PluginSys.hpp"
#include "Impl/Interfaces/Logger.hpp"


SG_NAMESPACE_BEGIN;
	
GameData::GameData(IPlugin* pPlugin) :
	m_PluginFileName(pPlugin ? pPlugin->GetFileName() : ILibraryManager::MainName),
	m_Paths{ std::format("{}.{}", ILibraryManager::CommonTag, SG::lib_manager.GetHostName()) }
{
	if (pPlugin)
		m_Paths.emplace_back(std::format("{0}/{1}/{1}.{2}", ILibraryManager::PluginsDir, m_PluginFileName, SG::lib_manager.GetHostName()));
}

void GameData::PushFiles(std::initializer_list<const char*> files)
{
	const std::string& host_name = SG::lib_manager.GetHostName();
	m_Paths.reserve(m_Paths.size() + files.size());
	for (auto file : files)
	{
		m_Paths.emplace_back(
			std::format(
				"{0}/{1}{2}.{3}",
				LibraryManager::PluginsDir,
				m_PluginFileName,
				file,
				host_name
			)
		);
	}
}


IntPtr GameData::ReadSignature(const std::vector<std::string>& keys, const std::string& signame)
{
	try
	{
		for (const std::string& file_name : GetPaths("signatures"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			const Json sig_info = Json::parse(file, nullptr, true, true);
			const Json* key = JumpTo(sig_info, keys);

			if (key && key->is_object())
			{
				if (signame.empty())
				{
					if (key->is_object())
						return LoadSignature(*key, true);
				}
				else if (const auto iter = key->find(signame); iter != key->end() && iter->is_object())
				{
					return LoadSignature(*iter, true);
				}
			}
		}

		throw std::runtime_error("Failed to find keys.");
	}
	catch (const std::exception& ex)
	{
		SG_LOG_ERROR(
			SG_MESSAGE("Exception reported while reading signature."),
			SG_LOGARG("Plugin", m_PluginFileName),
			SG_LOGARG("Keys", keys),
			SG_LOGARG("Signature", signame),
			SG_LOGARG("Exception", ex.what())
		);
	}

	return IntPtr::Null();
}

IntPtr GameData::ReadAddress(const std::vector<std::string>& keys, const std::string& addrname)
{
	try
	{
		for (const std::string& file_name : GetPaths("addresses"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			const Json addr_info = Json::parse(file, nullptr, true, true);
			const Json* key = JumpTo(addr_info, keys);

			if (key && key->is_object())
			{
				if (addrname.empty())
				{
					if (key->is_number_integer())
						return LoadSignature(*key, false);
				}
				else if (const auto iter = key->find(addrname); iter != key->end() && iter->is_number_integer())
				{
					return LoadSignature(*iter, false);
				}
			}
		}

		throw std::runtime_error("Failed to find keys.");
	}
	catch (const std::exception& ex)
	{
		SG_LOG_ERROR(
			SG_MESSAGE("Exception reported while reading address."),
			SG_LOGARG("Plugin", m_PluginFileName),
			SG_LOGARG("Address", addrname),
			SG_LOGARG("Keys", keys),
			SG_LOGARG("Exception", ex.what())
		);
	}

	return IntPtr::Null();
}

IntPtr GameData::ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, IntPtr pThis)
{
	IntPtr res;
	auto val = LoadOffset(keys, func_name, false);

	if (val.has_value())
	{
		IntPtr vt = pThis.read<void*>();
		res = vt.read<void*>(val.value() * sizeof(void*));
	}

	return res;
}

std::optional<int> GameData::ReadOffset(const std::vector<std::string>& keys, const std::string& name)
{
	return LoadOffset(keys, name, true);
}


Json GameData::ReadDetour(const std::vector<std::string>& keys, const std::string& detourname)
{
	try
	{
		for (const std::string& file_name : GetPaths("detours"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			const auto detour = Json::parse(file, nullptr, true, true);
			const Json* key = JumpTo(detour, keys);

			if (key && key->is_object())
			{
				if (detourname.empty())
				{
					if (key->is_object())
						return *key;
				}
				else if (const auto iter = key->find(detourname); iter != key->end() && iter->is_object())
				{
					return *iter;
				}
			}
		}

		throw std::runtime_error("Failed to find keys.");
	}
	catch (const std::exception& ex)
	{
		SG_LOG_ERROR(
			SG_MESSAGE("Exception reported while reading detour."),
			SG_LOGARG("Plugin", m_PluginFileName),
			SG_LOGARG("Detour", detourname),
			SG_LOGARG("Keys", keys),
			SG_LOGARG("Exception", ex.what())
		);
	}

	return Json{ };
}


const Json* GameData::JumpTo(const Json& main, const std::vector<std::string>& keys)
{
	const Json* data(&main);

	const bool bad_key = false;
	for (const auto key : keys)
	{
		if (!data->contains(key))
			return nullptr;
		else data = &(data->at(key));
	}

	return data;
}

IntPtr GameData::LoadSignature(const Json& info, bool is_signature)
{
	if (!info.contains("library"))
	{
		throw std::runtime_error("Missing library's name.");
	}

	const std::string& lib_name = info["library"].get_ref<const std::string&>();
	std::unique_ptr<LibraryImpl> lib(static_cast<LibraryImpl*>(SG::lib_manager.ReadLibrary(lib_name.c_str())));

	if (!lib)
	{
		throw std::runtime_error("Failed to load library for signature.");
	}

	constexpr const char* platform = BOOST_OS_WINDOWS ? "windows" : BOOST_OS_LINUX ? "linux" : "any";
	static_assert(platform != "any");

	const Json& sig = info[platform];

	IntPtr ptr = is_signature ?
		lib->FindBySignature(sig["pattern"]) :
		sig["address"].is_number_integer() ?
		IntPtr(lib->GetModule() + sig["address"].get<int>()) :
		nullptr;

	if (!ptr)
	{
		throw std::runtime_error("Failed to find signature.");
	}

	if (sig.contains("extra"))
	{
		const Json& extra = sig["extra"];
		if (extra.is_array())
		{
			const size_t size = extra.size();
			for (size_t i = 0; i < extra.size(); i++)
			{
				auto& str = extra.at(i).get_ref<const std::string&>();
				if (str.size() < 2)
					continue;
				switch (str[0])
				{
				case 'r':
				{
					for (size_t j = boost::lexical_cast<int>(str.c_str() + 1); j > 0; j--)
						ptr = ptr.read<void*>();
					break;
				}
				case 'a':
				{
					ptr += boost::lexical_cast<int>(str.c_str() + 1);
					break;
				}
				default: continue;
				}
			}
		}
	}

	return ptr;
}

std::optional<int> GameData::LoadOffset(const std::vector<std::string>& keys, const std::string& name, bool is_offset)
{
	try
	{
		for (const std::string& file_name : GetPaths(is_offset ? "offsets" : "virtuals"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			const Json sig_data = Json::parse(file, nullptr, true, true);
			const Json* key = JumpTo(sig_data, keys);

			if (key)
			{
				if (name.empty())
				{
					if (key->is_number_integer())
						return key->get<int>();
				}
				else if (const auto iter = key->find(name); iter != key->end() && iter->is_number_integer())
				{
					return iter->get<int>();
				}
			}
		}

		throw std::runtime_error("Failed to find keys.");
	}
	catch (const std::exception& ex)
	{
		SG_LOG_ERROR(
			SG_MESSAGE(is_offset ? "Exception reported while reading offsets." : "Exception reported while reading virtuals."),
			SG_LOGARG("Plugin", m_PluginFileName),
			SG_LOGARG("Name", name),
			SG_LOGARG("Keys", keys),
			SG_LOGARG("Exception", ex.what())
		);
	}
	return std::nullopt;
}

std::vector<std::string> GameData::GetPaths(const std::string_view& key) const
{
	std::vector<std::string> paths;
	paths.reserve(m_Paths.size());

	for (const std::string& path : m_Paths)
	{
		paths.emplace_back(std::format("{}.{}.json", path, key));
	}

	return paths;
}

SG_NAMESPACE_END;