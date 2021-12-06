#pragma once

#include <nlohmann/Json.hpp>
#include <optional>

#include "LibrarySys.hpp"
#include "Interfaces/GameData.hpp"

SG_NAMESPACE_BEGIN;
	
class GameData : public IGameData
{
public:
	GameData(IPlugin* pPlugin);

public:
	// Inherited via IGameData
	void PushFiles(std::initializer_list<const char*> files) override;

	IntPtr ReadSignature(const std::vector<std::string>& keys, const std::string& signame) override;

	IntPtr ReadAddress(const std::vector<std::string>& keys, const std::string& addrname) override;

	IntPtr ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, IntPtr pThis) override;

	std::optional<int> ReadOffset(const std::vector<std::string>& keys, const std::string& name) override;

public:
	Json ReadDetour(const std::vector<std::string>& keys, const std::string& signame);

private:
	const Json* JumpTo(const Json& main, const std::vector<std::string>& keys);

	IntPtr LoadSignature(const Json& info, bool is_signature);

	std::optional<int> LoadOffset(const std::vector<std::string>& keys, const std::string& name, bool is_offset);

	std::vector<std::string> GetPaths(const std::string_view& key) const;

private:
	const std::string& m_PluginFileName;
	std::vector<std::string> m_Paths;
};

SG_NAMESPACE_END;