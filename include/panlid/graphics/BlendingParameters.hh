#pragma once

#include "panlid/graphics/BlendingEquation.hh"
#include "panlid/graphics/BlendingType.hh"

namespace panlid::graphics {

/// Contains information about how an IDrawable should be blended into its destination.
struct BlendingParameters {
    /// The blending factor for the source color of the blend.
    BlendingType Source{};

    /// The blending factor for the destination color of the blend.
    BlendingType Destination{};

    /// The blending factor for the source alpha of the blend.
    BlendingType SourceAlpha{};

    /// The blending factor for the destination alpha of the blend.
    BlendingType DestinationAlpha{};

    /// Gets or sets the BlendingEquation to use for the RGB components of the blend.
    BlendingEquation RgbEquation{};

    /// Gets or sets the BlendingEquation to use for the alpha component of the blend.
    BlendingEquation AlphaEquation{};

    [[nodiscard]] constexpr static BlendingParameters GetNone() {
        return {
            .Source = BlendingType::One,
            .Destination = BlendingType::Zero,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::Zero,
            .RgbEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    [[nodiscard]] constexpr static BlendingParameters GetInherit() {
        return {
            .Source = BlendingType::Inherit,
            .Destination = BlendingType::Inherit,
            .SourceAlpha = BlendingType::Inherit,
            .DestinationAlpha = BlendingType::Inherit,
            .RgbEquation = BlendingEquation::Inherit,
            .AlphaEquation = BlendingEquation::Inherit,
        };
    }

    [[nodiscard]] constexpr static BlendingParameters GetMixture() {
        return {
            .Source = BlendingType::SrcAlpha,
            .Destination = BlendingType::OneMinusSrcAlpha,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::One,
            .RgbEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    [[nodiscard]] constexpr static BlendingParameters GetAdditive() {
        return {
            .Source = BlendingType::SrcAlpha,
            .Destination = BlendingType::One,
            .SourceAlpha = BlendingType::One,
            .DestinationAlpha = BlendingType::One,
            .RgbEquation = BlendingEquation::Add,
            .AlphaEquation = BlendingEquation::Add,
        };
    }

    /// Copy all properties that are marked as inherited from a parent BlendingParameters object.
    void CopyFromParent(const BlendingParameters& parent) {
        if (Source == BlendingType::Inherit) {
            Source = parent.Source;
        }
        if (Destination == BlendingType::Inherit) {
            Destination = parent.Destination;
        }
        if (SourceAlpha == BlendingType::Inherit) {
            SourceAlpha = parent.SourceAlpha;
        }
        if (DestinationAlpha == BlendingType::Inherit) {
            DestinationAlpha = parent.DestinationAlpha;
        }
        if (RgbEquation == BlendingEquation::Inherit) {
            RgbEquation = parent.RgbEquation;
        }
        if (AlphaEquation == BlendingEquation::Inherit) {
            AlphaEquation = parent.AlphaEquation;
        }
    }

    /// Any properties marked as inherited will have their blending mode changed to the default
    /// type. This can occur when a root element is set to inherited.
    void ApplyDefaultToInherited() {
        if (Source == BlendingType::Inherit) {
            Source = BlendingType::SrcAlpha;
        }
        if (Destination == BlendingType::Inherit) {
            Destination = BlendingType::OneMinusSrcAlpha;
        }
        if (SourceAlpha == BlendingType::Inherit) {
            SourceAlpha = BlendingType::One;
        }
        if (DestinationAlpha == BlendingType::Inherit) {
            DestinationAlpha = BlendingType::One;
        }
        if (RgbEquation == BlendingEquation::Inherit) {
            RgbEquation = BlendingEquation::Add;
        }
        if (AlphaEquation == BlendingEquation::Inherit) {
            AlphaEquation = BlendingEquation::Add;
        }
    }

    constexpr bool operator==(const BlendingParameters& other) const = default;

    [[nodiscard]] constexpr bool IsDisabled() const {
        return Source == BlendingType::One && Destination == BlendingType::Zero &&
               SourceAlpha == BlendingType::One && DestinationAlpha == BlendingType::Zero &&
               RgbEquation == BlendingEquation::Add && AlphaEquation == BlendingEquation::Add;
    }
};

} // namespace panlid::graphics
