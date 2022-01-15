#pragma once

#include <nlohmann/json_fwd.hpp>

namespace renderer
{
	bool InitializeForDx9(const nlohmann::json& data);
	void ShutdownForDx9();
}