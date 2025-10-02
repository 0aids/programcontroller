#pragma once
#include <vector>
#include <unordered_map>
#include <functional>
// A wrapper over some of the classes to help facilitate inter-manager functionality.
// A manager will actually define the EVENT_TYPES, which is then used to subscribe to events.
// The manager itself is responsible for ensuring that the events are broadcasted.
class IManager {
  public:
    enum class EVENT_TYPES;

  protected:
    std::unordered_map<EVENT_TYPES,
                       std::vector<std::function<void()>>>
        m_subscribers;
};
