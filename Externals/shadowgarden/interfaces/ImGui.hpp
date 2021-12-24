#pragma once

#include <array>
#include <imgui/imgui.h>
#include <nlohmann/Json.hpp>
#include "InterfacesSys.hpp"

SG_NAMESPACE_BEGIN;

template<typename _Ty>
struct Config
{
	using value_type = _Ty;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	/// <summary>
	/// Construct a Config value in 'vars'
	/// </summary>
	/// <param name="key_name">section name in 'vars'</param>
	/// <param name="name">variable's name that should be displayed in ImGui, key's name is 'name'</param>
	/// <param name="defval">variable's value, key's name is 'value'</param>
	/// <param name="description">variable's description, optioanl and should be used 'ImGui::DrawHelp', key's name is 'description'</param>
	constexpr explicit Config(const char* key_name = "", const value_type& defval = value_type{}, const char* description = nullptr) :
		CfgName(key_name), Description(description), Value(defval) { }

	void set_key(const char* name) noexcept { CfgName = name; }
	_NODISCARD const char* key() const noexcept { return CfgName; }

	_NODISCARD bool has_description() const noexcept { return Description != nullptr; }
	_NODISCARD const char* description() const noexcept { return Description; }

	_NODISCARD const_reference get() const noexcept { return Value; }
	_NODISCARD reference get() noexcept { return Value; }

	_NODISCARD operator const_reference() const noexcept { return get(); }
	_NODISCARD operator reference() noexcept { return get(); }

	template<typename _OTypeInfo>
	const_reference operator=(const Config<_OTypeInfo>& o) { return (Value = static_cast<const_reference>(o.Value)); }
	template<typename _Ty>
	const_reference operator=(const _Ty& o) { return (Value = static_cast<const_reference>(o)); }

	template<typename _Ty = value_type> _NODISCARD auto operator<=>(const _Ty& o) const noexcept { return Value <=> static_cast<const_reference>(o); }
	template<typename _Ty = value_type> _NODISCARD bool operator!=(const _Ty& o) const noexcept { return Value != static_cast<const_reference>(o); }

	template<typename _Ty = value_type> _NODISCARD auto operator<=>(const Config<_Ty>& prop) const noexcept { return Value <=> static_cast<const_reference>(prop.Value); }
	template<typename _Ty = value_type> _NODISCARD bool operator!=(const Config<_Ty>& prop) const noexcept { return Value != static_cast<const_reference>(prop.Value); }

	_NODISCARD auto operator+() const noexcept { return +this->Value; }
	_NODISCARD auto operator-() const noexcept { return -this->Value; }
	_NODISCARD auto operator~() const noexcept { return ~this->Value; }

	auto operator++() { return ++Value; }
	auto operator--() { return --Value; }
	auto operator++(int) { return Value++; }
	auto operator--(int) { return Value--; }

public:

#define SG_CONFIG_OPEARTOR(SYMBOL)												\
	reference operator##SYMBOL##=(const Config& prop) noexcept					\
	{																			\
		Value SYMBOL##= prop.Value;												\
		return *this;															\
	}																			\
	template<typename _Ty>														\
	reference operator##SYMBOL##=(const _Ty& o)	noexcept						\
	{																			\
		Value SYMBOL##= static_cast<const_reference>(o);						\
		return *this;															\
	}																			\
	_NODISCARD value_type operator##SYMBOL(const Config& prop) const noexcept	\
	{																			\
		return Value SYMBOL prop.Value;											\
	}																			\
	template<typename _Ty>														\
	_NODISCARD value_type operator##SYMBOL(const _Ty& o) const noexcept			\
	{																			\
		return Value SYMBOL static_cast<const_reference>(o);					\
	}

	SG_CONFIG_OPEARTOR(+);
	SG_CONFIG_OPEARTOR(-);
	SG_CONFIG_OPEARTOR(/ );
	SG_CONFIG_OPEARTOR(*);
	SG_CONFIG_OPEARTOR(%);
	SG_CONFIG_OPEARTOR(&);
	SG_CONFIG_OPEARTOR(| );
	SG_CONFIG_OPEARTOR(^);
	SG_CONFIG_OPEARTOR(<<);
	SG_CONFIG_OPEARTOR(>>);

#undef SG_CONFIG_OPEARTOR

public:
	_NODISCARD auto data() const noexcept
	{
		if constexpr (requires(const _Ty& v) { v.data(); })
			return Value.data();
		else
			return &Value;
	}

	_NODISCARD auto data() noexcept
	{
		if constexpr (requires(const _Ty& v) { v.data(); })
			return Value.data();
		else
			return &Value;
	}

	_NODISCARD auto begin() const noexcept
	{
		if constexpr (requires(const _Ty& v) { std::begin(v); })
			return Value.begin();
		else
			return &Value;
	}

	_NODISCARD auto begin() noexcept
	{
		if constexpr (requires(const _Ty& v) { std::begin(v); })
			return Value.begin();
		else
			return &Value;
	}

	_NODISCARD auto end() const noexcept
	{
		if constexpr (requires(const _Ty& v) { std::end(v); })
			return Value.end();
		else
			return begin() + sizeof(_Ty);
	}

	_NODISCARD auto end() noexcept
	{
		if constexpr (requires(const _Ty& v) { std::end(v); })
			return Value.end();
		else
			return begin() + sizeof(_Ty);
	}

public:
	pointer operator->() noexcept { return &Value; }
	constexpr const_pointer operator->() const noexcept { return &Value; }

	template<typename _IdxType = size_t>
	_NODISCARD const auto& operator[](const _IdxType& _idx) const noexcept
	{
		return Value[_idx];
	}

	template<typename _IdxType = size_t>
	_NODISCARD auto& operator[](const _IdxType& _idx) noexcept
	{
		return Value[_idx];
	}

	void to_json(Json& js) const
	{
		Json& sec{ js[key()] };
		constexpr bool has_to_json = requires(const _Ty& v, Json& js) { v.to_json(js); };
		constexpr bool has_get = requires(const _Ty& v) { v.get(); };
		Json& value = sec["value"];

		if constexpr (has_to_json)
		{
			Value.to_json(value);
		}
		else if constexpr (has_get)
		{
			value = Value.get();
		}
		else
		{
			sec["value"] = Value;
		}
		if (has_description())
			sec["description"] = Description;
	}

	void from_json(const Json& js)
	{
		const auto iter = js.find(key());
		if (iter != js.end())
		{
			constexpr bool has_from_json = requires(_Ty& v, const Json& js) { v.from_json(js); };
			constexpr bool has_get = requires(_Ty& v) { v.get(); };
			const Json& value = (*iter)["value"];

			if constexpr (has_from_json)
			{
				Value.from_json(value);
			}
			else if constexpr (has_get)
			{
				value.get_to(Value.get());
			}
			else
			{
				value.get_to(Value);
			}
		}
	}
	
private:
	const char* Description;
	const char* CfgName;
	_Ty Value;
};


class ConfigState
{
public:
	bool has_changed() const noexcept { return m_Changed; }
	operator bool() const noexcept { return has_changed(); }

	void set() noexcept { m_Changed = true; }
	void add(bool state) noexcept { m_Changed |= state; }
	void reset() noexcept { m_Changed = false; }

private:
	bool m_Changed{ };
};


#define SG_CONFIG_GET_ENUM()							\
static constexpr auto json_get_keyvalues() noexcept


#define SG_CONFIG_IMPL_GET_ENUM(Var)					\
void to_json(Json& js) const							\
{														\
	for (auto& [name, value] : json_get_keyvalues())	\
	{													\
		if (Var == value)								\
		{												\
			js = name;									\
			break;										\
		}												\
	}													\
}														\
														\
void from_json(const Json& js)							\
{														\
	for (auto& [name, value] : json_get_keyvalues())	\
	{													\
		if (js == name)									\
		{												\
			Var = value;								\
			break;										\
		}												\
	}													\
}

/*
class MyTestClass
{
public:
	enum class type : char
	{
		e0,
		e1,
		e2
	};

	constexpr MyTestClass(type type = type::e0) noexcept : m_Type(type) { }
	constexpr operator type() const noexcept { return m_Type; }

	SG_CONFIG_GET_ENUM()
	{
		return std::array{
			std::pair{ "var0", type::e0 },
			std::pair{ "var1", type::e1 },
			std::pair{ "var2 outline", type::e2 },
		};
	}
	SG_CONFIG_IMPL_GET_ENUM();

private:
	type m_Type;
}
*/


using ImGuiPluginCallback = std::function<bool()>;
using ImGuiPlCallbackId = uint32_t;

static constexpr const char* Interface_ImGuiLoader = "IImGuiLoader";

class IImGuiLoader : public IInterface
{
public:
	/// <summary>
	/// Initialize ImGuiContext in Dll's codespace.
	/// Must be called in 
	/// </summary>
	virtual ImGuiContext* GetContext() noexcept abstract;

	/// <summary>
	/// Register callback for ImGui drawing.
	/// </summary>
	/// <param name="is_propmgr">true for handling callback in 'Property Manager', else it will be called after main drawing as independent menu</param>
	virtual ImGuiPlCallbackId AddCallback(IPlugin* plugin, const char* name, const ImGuiPluginCallback& callback) abstract;

	/// <summary>
	/// Unregister callback by callback id.
	/// </summary>
	virtual void RemoveCallback(const ImGuiPlCallbackId id) abstract;

	/// <summary>
	/// Unregister callback by plugin.
	/// </summary>
	virtual void RemoveCallback(const SG::IPlugin* plugin) noexcept  abstract;

	/// <summary>
	/// Tests whether or not the main menu is on
	/// </summary>
	virtual bool IsMenuOn() abstract;

	/// <summary>
	/// Find a font from 'Pleiades/Fonts/*.ttf' 
	/// </summary>
	/// <returns>a pointer to ImFont texture, nullptr if the font wasn't loaded</returns>
	virtual ImFont* FindFont(const char* font_name) abstract;
};


SG_NAMESPACE_END; 

// Helpers for SG::Config, etc
namespace ImGui
{
	inline void DrawHelp(const char* text)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();

			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(text);
			ImGui::PopTextWrapPos();

			ImGui::EndTooltip();
		}
	}

	template<typename _Ty>
	inline void Help(const SG::Config<_Ty>& var)
	{
		if (var.has_description())
			DrawHelp(var.description());
	}

	inline void SameLineHelp(const char* text)
	{
		ImGui::SameLine();
		DrawHelp(text);
	}

	template<typename _Ty>
	inline void SameLineHelp(const SG::Config<_Ty>& var)
	{
		if (var.has_description())
		{
			ImGui::SameLine();
			DrawHelp(var.description());
		}
	}
	
	template<bool _x255 = true, typename _Ty, size_t _Size>
	inline bool ColorEdit(const char* name, std::array<_Ty, _Size>& color, ImGuiColorEditFlags flags = 0)
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		float actual_color[4]{
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			(_Size >= 4 ? (static_cast<float>(color[3]) / to_ratio) : 1.f)
		};

		if constexpr (_Size < 4)
			flags |= ImGuiColorEditFlags_NoAlpha;

		if (ImGui::ColorEdit4(name, actual_color, flags))
		{
			for (size_t i = 0; i < _Size; i++)
				color[i] = static_cast<_Ty>(actual_color[i] * to_ratio);
			return true;
		}

		return false;
	}

	template<bool _x255 = true, typename _Ty, size_t _Size>
	inline bool ColorPicker(const char* name, std::array<_Ty, _Size>& color, ImGuiColorEditFlags flags = 0)
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		float actual_color[4]{
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			(_Size >= 4 ? (static_cast<float>(color[3]) / to_ratio) : 1.f)
		};

		if constexpr (_Size >= 4)
			flags |= ImGuiColorEditFlags_NoAlpha;

		if (ImGui::ColorPicker4(name, actual_color, flags))
		{
			for (size_t i = 0; i < _Size; i++)
				color[i] = static_cast<_Ty>(actual_color[i] * to_ratio);
			return true;
		}

		return false;
	}

	template<bool _x255 = true, typename _Ty>
	inline bool ColorButton(const char* name, std::array<_Ty, 4>& color, ImGuiColorEditFlags flags = 0, ImVec2 size = { })
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		ImVec4 actual_color(
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			static_cast<float>(color[3]) / to_ratio
		);

		if (ImGui::ColorButton(name, actual_color, flags, size))
		{
			float* c(&actual_color.x);
			for (size_t i = 0; i < 4; i++)
				color[i] = static_cast<_Ty>(c[i] * to_ratio);
			return true;
		}

		return false;
	}

	template<bool _x255 = true, typename _Ty, size_t _Size>
	inline bool ColorEdit(const char* name, _Ty (&color)[_Size], ImGuiColorEditFlags flags = 0)
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		float actual_color[4]{
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			(_Size >= 4 ? (static_cast<float>(color[3]) / to_ratio) : 1.f)
		};

		if constexpr (_Size < 4)
			flags |= ImGuiColorEditFlags_NoAlpha;

		if (ImGui::ColorEdit4(name, actual_color, flags))
		{
			for (size_t i = 0; i < _Size; i++)
				color[i] = static_cast<_Ty>(actual_color[i] * to_ratio);
			return true;
		}

		return false;
	}

	template<bool _x255 = true, typename _Ty, size_t _Size>
	inline bool ColorPicker(const char* name, _Ty (&color)[_Size], ImGuiColorEditFlags flags = 0)
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		float actual_color[4]{
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			(_Size >= 4 ? (static_cast<float>(color[3]) / to_ratio) : 1.f)
		};

		if constexpr (_Size >= 4)
			flags |= ImGuiColorEditFlags_NoAlpha;

		if (ImGui::ColorPicker4(name, actual_color, flags))
		{
			for (size_t i = 0; i < _Size; i++)
				color[i] = static_cast<_Ty>(actual_color[i] * to_ratio);
			return true;
		}

		return false;
	}

	template<bool _x255 = true, typename _Ty, size_t _Size>
	inline bool ColorButton(const char* name, _Ty (&color)[_Size], ImGuiColorEditFlags flags = 0, ImVec2 size = { })
	{
		constexpr float to_ratio = _x255 ? 255.f : 1.f;
		ImVec4 actual_color(
			static_cast<float>(color[0]) / to_ratio,
			static_cast<float>(color[1]) / to_ratio,
			static_cast<float>(color[2]) / to_ratio,
			(_Size >= 4 ? (static_cast<float>(color[3]) / to_ratio) : 1.f)
		);

		if (ImGui::ColorButton(name, actual_color, flags, size))
		{
			float* c(&actual_color.x);
			for (size_t i = 0; i < 4; i++)
				color[i] = static_cast<_Ty>(c[i] * to_ratio);
			return true;
		}

		return false;
	}
}