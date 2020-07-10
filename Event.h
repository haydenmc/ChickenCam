/**
 * @file Event.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @brief A simple event subscription helper
 */

#pragma once

#include <functional>
#include <stdexcept>
#include <vector>

template<typename ...Args>
class Event
{
private:
    std::vector<std::function<void(Args...)>> subscribedCallbacks;

public:
    /* Public methods */
    void Fire(Args... args)
    {
        for (auto& callback : this->subscribedCallbacks)
        {
            callback(args...);
        }
    }

    void Subscribe(const std::function<void(Args...)>& callback)
    {
        this->subscribedCallbacks.push_back(callback);
    }

    // TODO: Unsubscribe via event tokens (then we can use a map! :D)
};