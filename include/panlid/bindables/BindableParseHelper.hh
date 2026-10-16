#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace panlid::bindables {

template <typename T> inline constexpr bool kAlwaysFalse = false;

template <typename T> inline constexpr bool kIsOptional = false;

template <typename U> inline constexpr bool kIsOptional<std::optional<U>> = true;

template <typename T> T ParseValue(std::string_view text) {
    if constexpr (std::same_as<T, std::string>) {
        return std::string{text};
    } else if constexpr (kIsOptional<T>) {
        if (text.empty()) {
            return std::nullopt;
        }
        return ParseValue<typename T::value_type>(text);
    } else if constexpr (std::is_assignable_v<T&, std::nullptr_t>) {
        if (!text.empty()) {
            throw std::invalid_argument(
                std::format("Cannot parse \"{}\" into a pointer type.", text));
        }
        return T{};
    } else if constexpr (std::same_as<T, bool>) {
        const auto matches = [text](std::string_view expected) {
            return std::ranges::equal(text, expected, [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == b;
            });
        };

        if (matches("true")) {
            return true;
        }
        if (matches("false")) {
            return false;
        }
        throw std::invalid_argument(std::format("Cannot parse \"{}\" as a bool.", text));
    } else if constexpr (std::is_enum_v<T>) {
        if (const auto value = magic_enum::enum_cast<T>(text)) {
            return *value;
        }
        throw std::invalid_argument(std::format("\"{}\" is not a valid enum name.", text));
    } else if constexpr (std::is_arithmetic_v<T>) {
        T value{};
        const auto* first = std::to_address(text.begin());
        const auto* last = std::to_address(text.end());
        const auto [end, error] = std::from_chars(first, last, value);

        if (error == std::errc::result_out_of_range) {
            throw std::out_of_range(std::format("\"{}\" is out of range.", text));
        }
        if (error != std::errc{} || end != last) {
            throw std::invalid_argument(std::format("Cannot parse \"{}\" as a number.", text));
        }
        return value;
    } else {
        static_assert(kAlwaysFalse<T>, "Bindable<T>::Parse is not supported for this T.");
    }
}

} // namespace panlid::bindables
