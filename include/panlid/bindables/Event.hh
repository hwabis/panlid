#pragma once

#include <functional>
#include <vector>

namespace panlid::bindables {

template <typename T> class Event {
  public:
    using Callback = std::function<void(const T&)>;

    void operator+=(Callback cb) { callbacks_.push_back(std::move(cb)); }

    void Invoke(const T& value) const {
        auto current = callbacks_;
        for (const auto& cb : current) {
            if (cb) {
                cb(value);
            }
        }
    }

    void Clear() { callbacks_.clear(); }

  private:
    std::vector<Callback> callbacks_;
};

} // namespace panlid::bindables
