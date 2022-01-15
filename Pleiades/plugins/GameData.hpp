#pragma once

#include <optional>

#include <nlohmann/Json.hpp>
#include <px/interfaces/GameData.hpp>

#include "Manager.hpp"

class GameData : public px::IGameData
{
public:
	GameData(px::IPlugin* pPlugin);

public:
	// Inherited via IGameData
	void PushFiles(const std::vector<std::string>& files) override;

	px::IntPtr ReadSignature(const std::vector<std::string>& keys, const std::string& signame) override;

	px::IntPtr ReadAddress(const std::vector<std::string>& keys, const std::string& addrname) override;

	px::IntPtr ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, px::IntPtr pThis) override;

	std::optional<int> ReadOffset(const std::vector<std::string>& keys, const std::string& name) override;

public:
	nlohmann::json ReadDetour(const std::vector<std::string>& keys, const std::string& signame);

private:
	const nlohmann::json* JumpTo(const nlohmann::json& main, const std::vector<std::string>& keys);

	px::IntPtr LoadSignature(const nlohmann::json& info, bool is_signature);

	std::optional<int> LoadOffset(const std::vector<std::string>& keys, const std::string& name, bool is_offset);

	std::vector<std::string> GetPaths(const std::string_view& key) const;

	const char* GetPluginName() const noexcept;

private:
	px::IPlugin* m_Plugin;
	std::vector<std::string> m_Paths;
};
