#include <fstream>
#include <asmjit/asmjit.h>
#include "TypeTable.hpp"
#include "Impl/Interfaces/Logger.hpp"
#include "Impl/Library/LibrarySys.hpp"


namespace px::DetourDetail
{
	TypeTable::TypeTable()
	{
		std::ifstream file(std::string(LibraryManager::CommonTag) + ".jit_types.json");
		if (file)
			m_TypeInfos = nlohmann::json::parse(file, nullptr, false, true);

		if (m_TypeInfos.is_discarded())
		{
			PX_LOG_ERROR(
				PX_MESSAGE("Failed to load 'Pleiades.jit_types' file")
			);
		}
	}

	size_t TypeTable::get_size(const std::vector<asmjit::TypeId>& types) const noexcept
	{
		using namespace asmjit::TypeUtils;

		size_t size = 0;
		for (auto type : types)
			size += sizeOf(deabstract(type, deabstractDeltaOfSize(sizeof(void*))));
		return size;
	}

	/// <summary>
	/// load type's underlying types into a vector
	/// types also can reference other custom types and will be loaded respectively
	/// 
	/// eg:
	/// "MyCustomType": [
	///		"F32x1",
	///		"F32x1",
	/// 	"F32x1"
	/// ],
	/// 
	/// will be representing: 
	/// struct MyCustomType
	/// {
	///		float xmmss_0;
	///		float xmmss_1;
	///		float xmmss_2;
	/// }
	/// 
	/// and is the same as:
	/// "MyCustomType": [
	///   {
	///		"type": "F32x1",
	///		"repeat" : 3
	///   }
	/// ]
	/// 
	/// output:
	/// std::vector{ Type::kIdF32x1, Type::kIdF32x1, Type::kIdF32x1 }
	/// 
	/// 
	/// Another example:
	/// "my_type1": [		//	{ int[10] }
	///		{
	///			"type": "I32",
	///			"repeat": 10
	///		}
	///	],
	///	"my_type2": "I32",	//	int
	///	"my_type3": [		//	{ int, int64 }
	///		{
	///			"type": [
	///				{
	///					"type": "I32"
	///				},
	///				{
	///					"type": "I64"
	///				}
	///			]
	///		}
	///	],
	///	"my_type4": [		//	{ int, int64 }[10]
	///		{
	///			"type": [
	///				{
	///					"type": "I32"
	///				},
	///				{
	///					"type": "I64"
	///				}
	///			],
	///		"repeat": 10
	///	}
	///	"my_type5": [		//	{ { int[2] }, int64 }[5]
	///	{
	///		"type": [
	///			{
	///				"type": "I32",
	///				"repeat": 2
	///			},
	///			{
	///				"type": "I64"
	///			}
	///		],
	///		"repeat": 5
	///	}
	///
	/// </summary>
	std::vector<asmjit::TypeId> TypeTable::load_type(const nlohmann::json& type_name) const
	{
		if (type_name.is_null())
			return { };

		const nlohmann::json& default_types = default_table();
		const bool type_is_string = type_name.is_string();

		if (type_is_string)
		{
			if (const auto iter = default_types.find(type_name); iter != default_types.end())
				return { static_cast<asmjit::TypeId>(*iter) };
		}

		const nlohmann::json& custom_types = custom_table();
		const nlohmann::json* custom_type = nullptr;
		if (!type_is_string)
		{
			custom_type = &type_name;
		}
		else
		{
			const auto iter = custom_types.find(type_name);
			if (iter != custom_types.end())
				custom_type = &*iter;
		}

		if (custom_type)
		{
			std::vector<asmjit::TypeId> types;
			for (const auto& arg : *custom_type)
			{
				const bool is_object = arg.is_object();
				if (!is_object && !arg.is_string())
					continue;

				using string_type = const std::string&;
				const nlohmann::json& name_or_arr = is_object ? arg["type"] : arg;

				if (name_or_arr.is_array())
				{
					auto sub_types = load_type(name_or_arr);
					if (!sub_types.empty())
					{
						size_t size_of_array = is_object && arg.contains("repeat") ? arg["repeat"].get<size_t>() : 1;
						if (size_of_array > 1)
						{
							for (; size_of_array > 1; size_of_array--)
							{
								types.insert(
									types.end(),
									sub_types.begin(),
									sub_types.end()
								);
							}
						}

						types.insert(
							types.end(),
							std::make_move_iterator(sub_types.begin()),
							std::make_move_iterator(sub_types.end())
						);

						return types;
					}
					else return { };
				}

				else if (name_or_arr.is_string())
				{
					// if it's generic type
					if (const auto iter = default_types.find(name_or_arr); iter != default_types.end())
					{
						// support for array types
						// instead of copy pasting N elements, "repeat" does it for us
						size_t size_of_array = is_object && arg.contains("repeat") ? arg["repeat"].get<size_t>() : 1;
						for (; size_of_array > 0; size_of_array--)
							types.push_back(static_cast<asmjit::TypeId>(iter->get<int>()));
					}
					// else if it's a custom type
					else if (custom_types.contains(name_or_arr))
					{
						auto sub_types = load_type(name_or_arr);
						if (!sub_types.empty())
						{
							types.insert(
								types.end(),
								std::make_move_iterator(sub_types.begin()),
								std::make_move_iterator(sub_types.end())
							);
						}
					}
					// else the user didn't provide any information
					else return { };
				}
			}
			return types;
		}
		else return { };
	}
}