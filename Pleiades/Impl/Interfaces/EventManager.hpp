#pragma once

#include <px/interfaces/EventManager.hpp>

PX_NAMESPACE_BEGIN();

struct EventCallbackWrapper;

class EventManager : public IEventManager
{
public:
	using map_type = std::map<std::string, std::list<EventCallbackWrapper>>;

	// Inherited via IEventManager
	EventID AddListener(const char* event_name, const EventCallback& callback) override;

	void RemoveListener(const char* event_name, EventID callback) override;

	void AddEvent(const char* event_name) override;

	void RemoveEvent(const char* event_name) override;

	bool StartEvent(const char* event_name) override;

	void ExecuteEvent(const char* event_name, EventData*) override;

private:
	map_type m_EventCallbacks;
};

extern EventManager event_manager;

PX_NAMESPACE_END();