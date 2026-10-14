#pragma once

#include "panlid/graphics/BlendingEquation.hh"
#include "panlid/graphics/BlendingType.hh"

namespace panlid::graphics {

struct BlendingParameters {
    BlendingType Source{};
    BlendingType Destination{};
    BlendingType SourceAlpha{};
    BlendingType DestinationAlpha{};
    BlendingEquation RGBEquation{};
    BlendingEquation AlphaEquation{};

    [[nodiscard]] static constexpr BlendingParameters None() {
        return {
            .Source = BlendingType::One,
            .Destination = BlendingType::Zero,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::Zero,
            .RGBEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    [[nodiscard]] static constexpr BlendingParameters Inherit() {
        return {
            .Source = BlendingType::Inherit,
            .Destination = BlendingType::Inherit,
            .SourceAlpha = BlendingType::Inherit,
            .DestinationAlpha = BlendingType::Inherit,
            .RGBEquation = BlendingEquation::Inherit,
            .AlphaEquation = BlendingEquation::Inherit,
        };
    }

    [[nodiscard]] static constexpr BlendingParameters Mixture() {
        return {
            .Source = BlendingType::SrcAlpha,
            .Destination = BlendingType::OneMinusSrcAlpha,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::One,
            .RGBEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    [[nodiscard]] static constexpr BlendingParameters Additive() {
        return {
            .Source = BlendingType::SrcAlpha,
            .Destination = BlendingType::One,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::One,
            .RGBEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    // Every field still set to Inherit takes the parent's value.
    constexpr void CopyFromParent(const BlendingParameters& parent) {
        replaceInherited(Source, parent.Source);
        replaceInherited(Destination, parent.Destination);
        replaceInherited(SourceAlpha, parent.SourceAlpha);
        replaceInherited(DestinationAlpha, parent.DestinationAlpha);
        replaceInherited(RGBEquation, parent.RGBEquation);
        replaceInherited(AlphaEquation, parent.AlphaEquation);
    }

    // For the root of a tree, where there is no parent to inherit from.
    constexpr void ApplyDefaultToInherited() {
        replaceInherited(Source, BlendingType::SrcAlpha);
        replaceInherited(Destination, BlendingType::OneMinusSrcAlpha);
        replaceInherited(SourceAlpha, BlendingType::One);
        replaceInherited(DestinationAlpha, BlendingType::One);
        replaceInherited(RGBEquation, BlendingEquation::Add);
        replaceInherited(AlphaEquation, BlendingEquation::Add);
    }

    [[nodiscard]] bool operator==(const BlendingParameters&) const = default;

    [[nodiscard]] constexpr bool IsDisabled() const { return *this == None(); }

  private:
    template <typename T> static constexpr void replaceInherited(T& value, T replacement) {
        if (value == T::Inherit) {
            value = replacement;
        }
    }
};

} // namespace panlid::graphics
