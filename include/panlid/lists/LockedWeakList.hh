#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace panlid::lists {

template <typename T> class LockedWeakList {
  public:
    void Add(const std::shared_ptr<T>& item) {
        std::lock_guard lock(mutex_);
        list_.push_back(item);
    }

    void Add(std::weak_ptr<T> weakReference) {
        std::lock_guard lock(mutex_);
        list_.push_back(std::move(weakReference));
    }

    bool Remove(const std::shared_ptr<T>& item) {
        std::lock_guard lock(mutex_);
        return removeLocked(item);
    }

    bool Remove(const std::weak_ptr<T>& weakReference) {
        std::lock_guard lock(mutex_);
        auto locked = weakReference.lock();
        if (!locked) {
            return false;
        }
        return removeLocked(locked);
    }

    void RemoveAt(std::size_t index) {
        std::lock_guard lock(mutex_);
        if (index < list_.size()) {
            list_.erase(list_.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }

    [[nodiscard]] bool Contains(const std::shared_ptr<T>& item) const {
        std::lock_guard lock(mutex_);
        return std::ranges::any_of(list_, [&](const std::weak_ptr<T>& w) {
            auto locked = w.lock();
            return locked && locked == item;
        });
    }

    [[nodiscard]] bool Contains(const std::weak_ptr<T>& weakReference) const {
        std::lock_guard lock(mutex_);
        auto target = weakReference.lock();
        if (!target) {
            return false;
        }
        return std::ranges::any_of(list_, [&](const std::weak_ptr<T>& w) {
            auto locked = w.lock();
            return locked && locked == target;
        });
    }

    void Clear() {
        std::lock_guard lock(mutex_);
        list_.clear();
    }

    [[nodiscard]] std::vector<std::shared_ptr<T>> ValidItems() const {
        std::lock_guard lock(mutex_);
        std::vector<std::shared_ptr<T>> result;
        result.reserve(list_.size());
        for (const auto& weakRef : list_) {
            if (auto locked = weakRef.lock()) {
                result.push_back(std::move(locked));
            }
        }
        return result;
    }

  private:
    [[nodiscard]] bool removeLocked(const std::shared_ptr<T>& item) {
        auto it = std::ranges::find_if(list_, [&](const std::weak_ptr<T>& w) {
            auto locked = w.lock();
            return locked && locked == item;
        });
        if (it == list_.end()) {
            return false;
        }
        list_.erase(it);
        return true;
    }

    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<T>> list_;
};

} // namespace panlid::lists
