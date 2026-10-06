#pragma once

#include <algorithm>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace panlid::bindables::detail {

inline void requireFullyParsed(std::size_t used, const std::string& text) {
    if (used != text.size()) {
        throw std::invalid_argument("trailing characters in \"" + text + "\"");
    }
}

template <std::integral T, std::integral U> T narrowTo(U value) {
    if (!std::in_range<T>(value)) {
        throw std::out_of_range("number out of range");
    }
    return static_cast<T>(value);
}

template <std::floating_point T> T stringToFloating(const std::string& text, std::size_t* used) {
    if constexpr (std::same_as<T, float>) {
        return std::stof(text, used);
    } else if constexpr (std::same_as<T, double>) {
        return std::stod(text, used);
    } else {
        return std::stold(text, used);
    }
}

template <std::same_as<std::string> T> T parseText(const std::string& text) { return text; }

template <std::same_as<bool> T> T parseText(const std::string& text) {
    std::string lowered = text;
    std::ranges::transform(lowered, lowered.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "true") {
        return true;
    }
    if (lowered == "false") {
        return false;
    }
    throw std::invalid_argument("not a valid bool: \"" + text + "\"");
}

template <typename T>
    requires std::is_enum_v<T>
T parseText(const std::string& text) {
    if (auto value = magic_enum::enum_cast<T>(text)) {
        return *value;
    }
    throw std::invalid_argument("not a valid enum name: \"" + text + "\"");
}

template <std::floating_point T> T parseText(const std::string& text) {
    std::size_t used = 0;
    const T value = stringToFloating<T>(text, &used);
    requireFullyParsed(used, text);
    return value;
}

template <std::signed_integral T> T parseText(const std::string& text) {
    std::size_t used = 0;
    const long long value = std::stoll(text, &used);
    requireFullyParsed(used, text);
    return narrowTo<T>(value);
}

// bool is technically an unsigned integral, so it has to be excluded here.
template <std::unsigned_integral T>
    requires(!std::same_as<T, bool>)
T parseText(const std::string& text) {
    if (text.find('-') != std::string::npos) {
        throw std::invalid_argument("negative value for unsigned type: \"" + text + "\"");
    }
    std::size_t used = 0;
    const unsigned long long value = std::stoull(text, &used);
    requireFullyParsed(used, text);
    return narrowTo<T>(value);
}

} // namespace panlid::bindables::detail
