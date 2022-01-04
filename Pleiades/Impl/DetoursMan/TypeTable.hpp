#pragma once

#include <unordered_map>
#include <nlohmann/Json.hpp>
#include <px/defines.hpp>


PX_NAMESPACE_BEGIN();

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

		asmjit::TypeId get_detault(const std::string& type_name) const
		{
			return default_table()[type_name].get<asmjit::TypeId>();
		}

		const nlohmann::json& custom_table() const noexcept { return m_TypeInfos["Custom"]; }
		const nlohmann::json& default_table() const noexcept { return m_TypeInfos["Defaults"]; }

		/// <summary>
		/// collect the size of types
		/// </summary>
		size_t get_size(const std::vector<asmjit::TypeId>&) const noexcept;

		/// <summary>
		/// load type's underlying types into a vector
		/// </summary>
		/// <param name="type_name"></param>
		/// <returns></returns>
		std::vector<asmjit::TypeId> load_type(const nlohmann::json& type_name) const;

	private:
		nlohmann::json m_TypeInfos;
	};
}

PX_NAMESPACE_END();