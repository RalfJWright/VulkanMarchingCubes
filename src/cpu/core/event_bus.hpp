#pragma once

#include <any>
#include <typeindex>
#include <functional>
#include <unordered_map>

namespace tmx {
  struct EventBus {
	public:
	 EventBus() = default;
	~EventBus() = default;

	template<typename EventType, typename CallerType>
	void add(CallerType *caller, void (CallerType::* callerFn)(const std::any &event)) {
	  callbacks[typeid(EventType)] = [caller, callerFn](const std::any &event) { (caller->*callerFn)(event); };
	}

	template<typename EventType>
	void notify(const EventType &event) {
	  (callbacks[typeid(EventType)])(event);
	}

	private:
	std::unordered_map< std::type_index, std::function<void(const std::any &)> > callbacks;
  };
}