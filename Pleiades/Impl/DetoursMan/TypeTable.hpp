#pragma once

#include <unordered_map>
#include <nlohmann/Json.hpp>
#include "SGDefines.hpp"


SG_NAMESPACE_BEGIN;

namespace DetourDetail
{
	class TypeTable
	{
	public:
		TypeTable();

		bool is_default(const std::string& type_name) const
		{
			default_table().contains(type_name);
		}
		bool is_custom(const std::string& type_name) const
		{
			custom_table().contains(type_name);
		}

		uint32_t get_detault(const std::string& type_name) const
		{
			return m_TypeInfos["Defaults"][type_name].get<uint32_t>();
		}

		const Json& custom_table() const noexcept { return m_TypeInfos["Custom"]; }
		const Json& default_table() const noexcept { return m_TypeInfos["Defaults"]; }

		/// <summary>
		/// collect the size of types
		/// </summary>
		size_t get_size(const std::vector<uint32_t>&) const noexcept;

		/// <summary>
		/// load type's underlying types into a vector
		/// </summary>
		/// <param name="type_name"></param>
		/// <returns></returns>
		std::vector<uint32_t> load_type(const Json& type_name) const;

	private:
		Json m_TypeInfos;
	};
}

SG_NAMESPACE_END;