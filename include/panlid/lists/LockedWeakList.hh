#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace panlid::lists {

template <typename T> class LockedWeakList {
  public:
    void Add(const std::weak_ptr<T>& item) {
        std::scoped_lock lock{mutex_};
        items_.push_back(item);
    }

    bool Remove(const std::weak_ptr<T>& item) {
        std::scoped_lock lock{mutex_};

        const auto it = std::ranges::find_if(
            items_, [&item](const auto& existing) { return sameOwner(existing, item); });

        if (it == items_.end()) {
            return false;
        }

        items_.erase(it);
        return true;
    }

    [[nodiscard]] bool Contains(const std::weak_ptr<T>& item) const {
        std::scoped_lock lock{mutex_};

        return std::ranges::any_of(
            items_, [&item](const auto& existing) { return sameOwner(existing, item); });
    }

    [[nodiscard]] std::vector<std::shared_ptr<T>> GetSnapshot() {
        std::scoped_lock lock{mutex_};

        std::vector<std::shared_ptr<T>> alive;
        alive.reserve(items_.size());

        std::erase_if(items_, [&alive](const std::weak_ptr<T>& item) {
            auto strong = item.lock();
            if (!strong) {
                return true;
            }

            alive.push_back(std::move(strong));
            return false;
        });

        return alive;
    }

  private:
    [[nodiscard]] static bool sameOwner(const std::weak_ptr<T>& a, const std::weak_ptr<T>& b) {
        return !a.owner_before(b) && !b.owner_before(a);
    }

    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<T>> items_;
};

} // namespace panlid::lists
