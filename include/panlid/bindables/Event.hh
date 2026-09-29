#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace panlid::bindables {

template <typename T> class Event {
  public:
    using Callback = std::function<void(const T&)>;
    using SubscriptionId = std::size_t;

    SubscriptionId operator+=(Callback cb) {
        SubscriptionId id = nextId_++;
        callbacks_.emplace_back(id, std::move(cb));
        return id;
    }

    void Unsubscribe(SubscriptionId id) {
        std::erase_if(callbacks_, [id](const auto& entry) { return entry.first == id; });
    }

    void Invoke(const T& value) const {
        auto current = callbacks_;
        for (const auto& [id, cb] : current) {
            if (cb) {
                cb(value);
            }
        }
    }

    void Clear() { callbacks_.clear(); }

  private:
    std::vector<std::pair<SubscriptionId, Callback>> callbacks_;
    SubscriptionId nextId_ = 0;
};

} // namespace panlid::bindables
