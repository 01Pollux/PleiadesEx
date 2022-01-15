
#include <filesystem>
#include <fstream>

#include <boost/lexical_cast.hpp>

#include <px/interfaces/PluginSys.hpp>

#include "Library/Manager.hpp"
#include "Library/Module.hpp"

#include "Logs/Logger.hpp"
#include "GameData.hpp"


GameData::GameData(px::IPlugin* pPlugin) :
	m_Plugin(pPlugin),
	m_Paths{ std::format("{}.{}", LibraryManager::CommonTag, px::lib_manager.GetHostName()) }
{
	if (pPlugin)
		m_Paths.emplace_back(std::format("{0}/{1}/{1}.{2}", LibraryManager::PluginsDir, pPlugin ? pPlugin->GetFileName() : LibraryManager::MainName, px::lib_manager.GetHostName()));
}

void GameData::PushFiles(const std::vector<std::string>& files)
{
	const std::string_view& host_name = px::lib_manager.GetHostName();
	m_Paths.reserve(m_Paths.size() + files.size());
	const char* const file_name = this->GetPluginName();

	for (auto& file : files)
	{
		m_Paths.emplace_back(
			std::format(
				"{0}/{1}{2}.{3}",
				LibraryManager::PluginsDir,
				file_name,
				file,
				host_name
			)
		);
	}
}


px::IntPtr GameData::ReadSignature(const std::vector<std::string>& keys, const std::string& signame)
{
	try
	{
		for (const std::string& file_name : GetPaths("signatures"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			auto sig_info{ nlohmann::json::parse(file, nullptr, true, true) };
			auto key = JumpTo(sig_info, keys);

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
		PX_LOG_ERROR(
			PX_MESSAGE("Exception reported while reading signature."),
			PX_LOGARG("Plugin", this->GetPluginName()),
			PX_LOGARG("Keys", keys),
			PX_LOGARG("Signature", signame),
			PX_LOGARG("Exception", ex.what())
		);
	}

	return nullptr;
}

px::IntPtr GameData::ReadAddress(const std::vector<std::string>& keys, const std::string& addrname)
{
	try
	{
		for (const std::string& file_name : GetPaths("addresses"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			auto addr_info{ nlohmann::json::parse(file, nullptr, true, true) };
			auto key = JumpTo(addr_info, keys);

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
		PX_LOG_ERROR(
			PX_MESSAGE("Exception reported while reading address."),
			PX_LOGARG("Plugin", this->GetPluginName()),
			PX_LOGARG("Address", addrname),
			PX_LOGARG("Keys", keys),
			PX_LOGARG("Exception", ex.what())
		);
	}

	return nullptr;
}

px::IntPtr GameData::ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, px::IntPtr pThis)
{
	px::IntPtr res;
	auto val = LoadOffset(keys, func_name, false);

	if (val.has_value())
	{
		px::IntPtr vt = pThis.read<void*>();
		res = vt.read<void*>(val.value() * sizeof(void*));
	}

	return res;
}

std::optional<int> GameData::ReadOffset(const std::vector<std::string>& keys, const std::string& name)
{
	return LoadOffset(keys, name, true);
}


nlohmann::json GameData::ReadDetour(const std::vector<std::string>& keys, const std::string& detourname)
{
	try
	{
		for (const std::string& file_name : GetPaths("detours"))
		{
			std::ifstream file(file_name);
			if (!file)
				continue;

			auto detour_info{ nlohmann::json::parse(file, nullptr, true, true) };
			auto key = JumpTo(detour_info, keys);

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
		PX_LOG_ERROR(
			PX_MESSAGE("Exception reported while reading detour."),
			PX_LOGARG("Plugin", this->GetPluginName()),
			PX_LOGARG("Detour", detourname),
			PX_LOGARG("Keys", keys),
			PX_LOGARG("Exception", ex.what())
		);
	}

	return nlohmann::json{ };
}


const nlohmann::json* GameData::JumpTo(const nlohmann::json& main, const std::vector<std::string>& keys)
{
	auto data = &main;

	const bool bad_key = false;
	for (const auto& key : keys)
	{
		if (!data->contains(key))
			return nullptr;
		else data = &(data->at(key));
	}

	return data;
}

px::IntPtr GameData::LoadSignature(const nlohmann::json& info, bool is_signature)
{
	if (!info.contains("library"))
	{
		throw std::runtime_error("Missing library's name.");
	}

	const std::string& lib_name = info["library"].get_ref<const std::string&>();
	std::unique_ptr<LibraryImpl> lib(static_cast<LibraryImpl*>(px::lib_manager.ReadLibrary(lib_name.c_str())));

	if (!lib)
	{
		throw std::runtime_error("Failed to load library for signature.");
	}

	auto& sig = info["windows"];

	px::IntPtr ptr = is_signature ?
		lib->FindBySignature(sig["pattern"]) :
		sig["address"].is_number_integer() ?
			(lib->GetModule() + sig["address"].get<int>()) :nullptr;

	if (!ptr)
	{
		throw std::runtime_error("Failed to find signature.");
	}

	if (sig.contains("extra"))
	{
		auto& extra = sig["extra"];
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

			auto offset_info{ nlohmann::json::parse(file, nullptr, true, true) };
			auto key = JumpTo(offset_info, keys);

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
		PX_LOG_ERROR(
			PX_MESSAGE(is_offset ? "Exception reported while reading offsets." : "Exception reported while reading virtuals."),
			PX_LOGARG("Plugin", this->GetPluginName()),
			PX_LOGARG("Name", name),
			PX_LOGARG("Keys", keys),
			PX_LOGARG("Exception", ex.what())
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

const char* GameData::GetPluginName() const noexcept
{
	return m_Plugin ? m_Plugin->GetFileName().c_str() : LibraryManager::MainName;
}
