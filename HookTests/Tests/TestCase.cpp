#include "TestCase.hpp"
#include "imgui/imgui.h"


SG_NAMESPACE_BEGIN;

void ITestCase::Register(IGameData* gamedata)
{
	for (auto entry : m_Tests)
		entry->OnRegister(gamedata);
}

void ITestCase::Unregister()
{
	for (auto entry : m_Tests)
		entry->OnUnregister();
}

void ITestCase::Render()
{
	for (const auto section : { "ThisCall" })
	{
		if (!ImGui::CollapsingHeader(section))
			continue;

		for (auto entry : m_Tests)
		{
			if (entry->SectionName != section)
				continue;

			ImGui::TextUnformatted(entry->TextLabel);
			ImGui::SameLine();
			ImGui::PushID(entry);
			if (ImGui::Button("Run"))
				entry->OnRun();
			ImGui::PopID();
		}
	}
}

SG_NAMESPACE_END;