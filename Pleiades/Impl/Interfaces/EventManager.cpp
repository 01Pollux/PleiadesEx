
#include <list>
#include <functional>

#include "EventManager.hpp"


PX_NAMESPACE_BEGIN();
EventManager event_manager;

struct EventCallbackWrapper
{
	EventCallback Callback;
	EventID Id;
	auto operator==(const EventID id) const noexcept { return this->Id == id; }
};

EventID EventManager::AddListener(const char* event_name, const EventCallback& callback)
{
	auto iter = m_EventCallbacks.find(event_name);
	if (iter == m_EventCallbacks.end())
		return 0;

	EventID id = 1;
	for (const auto& entry : iter->second)
	{
		if (entry.Id == id)
			++id;
	}
	iter->second.emplace_back(callback, id);

	return id;
}

void EventManager::RemoveListener(const char* event_name, EventID id)
{
	auto it = m_EventCallbacks.find(event_name);
	if (it != m_EventCallbacks.end())
	{
		auto& list = it->second;
		std::erase(list, id);
	}
}

void EventManager::AddEvent(const char* event_name)
{
	m_EventCallbacks.try_emplace(event_name);
}

void EventManager::RemoveEvent(const char* event_name)
{
	m_EventCallbacks.erase(event_name);
}

bool EventManager::StartEvent(const char* event_name)
{
	auto it = m_EventCallbacks.find(event_name);
	return it == m_EventCallbacks.end() ? false : it->second.size() > 0;
}

void EventManager::ExecuteEvent(const char* event_name, EventData* data)
{
	auto it = m_EventCallbacks.find(event_name);
	if (it != m_EventCallbacks.end())
	{
		for (auto& info : it->second)
			info.Callback(data);
	}
}

PX_NAMESPACE_END();