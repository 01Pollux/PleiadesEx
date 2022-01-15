#pragma once

#include <map>
#include <functional>
#include <list>
#include <px/interfaces/EventManager.hpp>

class EventManager : public px::IEventManager
{
public:
	struct EventCallbackWrapper
	{
		px::EventCallback Callback;
		px::EventID Id;
		auto operator==(const px::EventID id) const noexcept { return this->Id == id; }
	};
	using map_type = std::map<std::string, std::list<EventCallbackWrapper>>;

	// Inherited via IEventManager
	px::EventID AddListener(const char* event_name, const px::EventCallback& callback) override;

	void RemoveListener(const char* event_name, px::EventID callback) override;

	void AddEvent(const char* event_name) override;

	void RemoveEvent(const char* event_name) override;

	bool StartEvent(const char* event_name) override;

	void ExecuteEvent(const char* event_name, px::bitbuf&) override;

private:
	map_type m_EventCallbacks;
};

PX_NAMESPACE_BEGIN();
inline EventManager event_manager;
PX_NAMESPACE_END();
