#include "Interfaces/InterfacesSys.hpp"
#include "Defines.hpp"


class PluginSample : public SG::IPluginImpl
{
public:
	bool OnPluginLoad2(SG::IPluginManager* ifacemgr) override
	{
		ImGui::SetCurrentContext(SG::ImGuiLoader->GetContext());
		m_ImGuiCallback = SG::ImGuiLoader->AddCallback(
			this,
			"Plugin Sample",
			[] ()
			{
				static bool dummy{ };
				return ImGui::Checkbox("Dummy", &dummy);
			}
		);
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnPluginLoad2"),
			SG_LOGARG("This", std::bit_cast<uintptr_t>(this))
		);
		return true;
	}

	void OnPluginUnload() override
	{
		std::vector<std::string> something{
			"Hah",
			"01",
			"Px"
		};
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnPluginUnload"),
			SG_LOGARG("something", something)
		);
	}

	void OnAllPluginsLoaded() override
	{
		ImGui::SetCurrentContext(SG::ImGuiLoader->GetContext());
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnAllPluginsLoaded")
		);
	}

	void OnPluginPauseChange(bool pausing) noexcept override
	{
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnPluginPauseChange"),
			SG_LOGARG("pausing", pausing)
		);
	}

	virtual void OnSaveConfig(Json& cfg) override
	{
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnPluginPauseChange"),
			SG_LOGARG("vars", cfg)
		);
	}

	virtual void OnReloadConfig(const Json& cfg) override
	{
		SG_LOG_MESSAGE(
			SG_MESSAGE("PluginSample::OnPluginPauseChange"),
			SG_LOGARG("vars", cfg)
		);
	}

private:
	SG::ImGuiPlCallbackId m_ImGuiCallback;
};

// Important for exporting plugin
SG_DLL_EXPORT(PluginSample);