#pragma once

#include <optional>

#include <nlohmann/Json.hpp>
#include <px/interfaces/GameData.hpp>

#include "LibrarySys.hpp"

PX_NAMESPACE_BEGIN();
	
class GameData : public IGameData
{
public:
	GameData(IPlugin* pPlugin);

public:
	// Inherited via IGameData
	void PushFiles(const std::vector<std::string>& files) override;

	IntPtr ReadSignature(const std::vector<std::string>& keys, const std::string& signame) override;

	IntPtr ReadAddress(const std::vector<std::string>& keys, const std::string& addrname) override;

	IntPtr ReadVirtual(const std::vector<std::string>& keys, const std::string& func_name, IntPtr pThis) override;

	std::optional<int> ReadOffset(const std::vector<std::string>& keys, const std::string& name) override;

public:
	nlohmann::json ReadDetour(const std::vector<std::string>& keys, const std::string& signame);

private:
	const nlohmann::json* JumpTo(const nlohmann::json& main, const std::vector<std::string>& keys);

	IntPtr LoadSignature(const nlohmann::json& info, bool is_signature);

	std::optional<int> LoadOffset(const std::vector<std::string>& keys, const std::string& name, bool is_offset);

	std::vector<std::string> GetPaths(const std::string_view& key) const;

	const char* GetPluginName() const noexcept;

private:
	IPlugin*				 m_Plugin;
	std::vector<std::string> m_Paths;
};

PX_NAMESPACE_END();