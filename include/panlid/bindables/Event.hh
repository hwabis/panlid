#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace panlid::bindables {

template <typename... Args> class Event {
  public:
    using Handler = std::function<void(const Args&...)>;

    Event& operator+=(Handler handler) {
        auto next = handlers_ == nullptr ? std::make_shared<std::vector<Handler>>()
                                         : std::make_shared<std::vector<Handler>>(*handlers_);
        next->push_back(std::move(handler));
        handlers_ = std::move(next);
        return *this;
    }

    void Clear() { handlers_.reset(); }

    void Invoke(const Args&... args) const {
        const auto snapshot = handlers_;
        if (snapshot == nullptr) {
            return;
        }

        for (const auto& handler : *snapshot) {
            handler(args...);
        }
    }

  private:
    std::shared_ptr<const std::vector<Handler>> handlers_;
};

} // namespace panlid::bindables
