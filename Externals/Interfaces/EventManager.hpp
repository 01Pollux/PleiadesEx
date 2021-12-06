#pragma once

#include "InterfacesSys.hpp"
#include "User/EnumClass.hpp"

#include <variant>
#include <map>


_STD_BEGIN;

template<typename>
class function;

_STD_END;


SG_NAMESPACE_BEGIN;

static constexpr const char* Interface_EventManager = "IEventManager";

class EventData
{
public:
	using variant = std::variant<
		bool,
		int8_t,
		int16_t,
		int32_t,
		void*,
		float,
		double,
		std::string
	>;
	using map_type = std::map<std::string, variant>;

	EventData() = default;

	EventData(std::initializer_list<map_type::value_type> list) : DataMap(list) { };
	template<typename _Iter>
	EventData(_Iter first, _Iter last) : DataMap(first, last) { };

	void set_bool(const char* key, bool val) { emplace(key, static_cast<int8_t>(val)); }
	void set_int8(const char* key, int8_t val) { emplace(key, val); }
	void set_int16(const char* key, int16_t val) { emplace(key, val); }
	void set_int32(const char* key, int32_t val) { emplace(key, val); }
	void set_uint(const char* key, uint32_t val) { emplace(key, static_cast<int32_t>(val)); }
	void set_int(const char* key, int val) { emplace(key, val); }
	void set_ptr(const char* key, void* val) { emplace(key, val); }
	void set_float(const char* key, float val) { emplace(key, val); }
	void set_double(const char* key, double val) { emplace(key, val); }
	void set_string(const char* key, const std::string& val) { emplace(key, val); }
	void set_string(const char* key, std::string&& val) { emplace(key, std::move(val)); }

	template<typename _Ty>
	_Ty& alloc(const char* key) { return DataMap[key].emplace<_Ty>(); }

	variant* find(const char* key) { const auto iter = DataMap.find(key); return iter == DataMap.end() ? nullptr : &iter->second; }
	const variant* find(const char* key) const { const auto iter = DataMap.find(key); return iter == DataMap.end() ? nullptr : &iter->second; }

	auto begin()		  noexcept { return DataMap.begin(); }
	auto rbegin()		  noexcept { return DataMap.rbegin(); }
	auto begin()	const noexcept { return DataMap.begin(); }
	auto rbegin()	const noexcept { return DataMap.rbegin(); }
	auto cbegin()	const noexcept { return DataMap.cbegin(); }
	auto crbegin()	const noexcept { return DataMap.crbegin(); }

	auto end()			  noexcept { return DataMap.end(); }
	auto rend()			  noexcept { return DataMap.rend(); }
	auto end()		const noexcept { return DataMap.end(); }
	auto rend()		const noexcept { return DataMap.rend(); }
	auto cend()		const noexcept { return DataMap.cend(); }
	auto crend()	const noexcept { return DataMap.crend(); }

	size_t size()	const noexcept { return DataMap.size(); }
	bool contains(const char* key) const
	{
		return DataMap.contains(key);
	}
	void erase(const char* key) { DataMap.erase(key); }

	bool				get_bool(const char* key)	const { return static_cast<bool>(std::get<int8_t>(*find(key))); }
	int8_t				get_int8(const char* key)	const { return std::get<int8_t>(*find(key)); }
	int16_t				get_int16(const char* key)	const { return std::get<int16_t>(*find(key)); }
	int32_t				get_int32(const char* key)	const { return std::get<int32_t>(*find(key)); }
	uint32_t			get_uint(const char* key)	const { return static_cast<uint32_t>(std::get<int32_t>(*find(key))); }
	int					get_int(const char* key)	const { return std::get<int32_t>(*find(key)); }
	const void* get_ptr(const char* key)	const { return std::get<void*>(*find(key)); }
	float				get_float(const char* key)	const { return std::get<float>(*find(key)); }
	double				get_double(const char* key) const { return std::get<double>(*find(key)); }
	const std::string& get_string(const char* key)	const { return std::get<std::string>(*find(key)); }

private:
	template<typename _Ty>	void emplace(const char* key, const _Ty& val) { DataMap[key].emplace<_Ty>(val); }
	template<typename _Ty>	void emplace(const char* key, _Ty&& val) { DataMap[key] = std::forward<_Ty>(val); }

	map_type DataMap;
};

using EventCallback = std::function<void(EventData*)>;
using EventID = uint32_t;

class IEventManager : public IInterface
{
public:
	/// <summary>
	/// Add event listener, returns 0 if the event doesn't exists
	/// </summary>
	virtual EventID AddListener(const char* event_name, const EventCallback& callback) abstract;

	/// <summary>
	/// Remove an existsing event listener by name
	/// </summary>
	virtual void RemoveListener(const char* event_name, EventID id) abstract;

	/// <summary>
	/// Register an event
	/// </summary>
	virtual void AddEvent(const char* event_name) abstract;

	/// <summary>
	/// Delete an existsing event
	/// </summary>
	virtual void RemoveEvent(const char* event_name) abstract;

	/// <summary>
	/// Begin event's callback
	/// </summary>
	/// <returns>true if the event exists and there is atleast 1 listener</returns>
	virtual bool StartEvent(const char* event_name) abstract;

	/// <summary>
	/// End event's callback
	/// </summary>
	virtual void ExecuteEvent(const char* event_name, EventData*) abstract;
};

SG_NAMESPACE_END;