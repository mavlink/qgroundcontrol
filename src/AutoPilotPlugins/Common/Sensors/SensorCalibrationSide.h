#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

/// Common enumeration for 6-side sensor calibration orientations.
/// Used by both APM and PX4 sensor calibration workflows.
class SensorCalibrationSide : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Enum container only")

public:
    /// The 6 sides used for orientation-based calibration
    enum class Side {
        Down = 0,       ///< Vehicle resting normally (down side facing down)
        Up,             ///< Vehicle upside down (down side facing up)
        Left,           ///< Vehicle on left side
        Right,          ///< Vehicle on right side
        Front,          ///< Vehicle nose down
        Back,           ///< Vehicle tail down (nose up)
        Count           ///< Number of sides (not a valid side)
    };
    Q_ENUM(Side)

    /// Bitmask values for each side (for visibility/calibration masks)
    enum SideMask {
        MaskBack  = (1 << 0),  ///< Tail down
        MaskFront = (1 << 1),  ///< Nose down
        MaskLeft  = (1 << 2),
        MaskRight = (1 << 3),
        MaskUp    = (1 << 4),  ///< Upside down
        MaskDown  = (1 << 5),  ///< Normal orientation
        MaskAll   = 0x3F       ///< All 6 sides
    };
    Q_ENUM(SideMask)

    /// Convert side enum to bitmask
    static int sideToMask(Side side) {
        switch (side) {
        case Side::Down:  return MaskDown;
        case Side::Up:    return MaskUp;
        case Side::Left:  return MaskLeft;
        case Side::Right: return MaskRight;
        case Side::Front: return MaskFront;
        case Side::Back:  return MaskBack;
        default:          return 0;
        }
    }

    /// Convert bitmask to side enum (returns first set bit)
    static Side maskToSide(int mask) {
        if (mask & MaskDown)  return Side::Down;
        if (mask & MaskUp)    return Side::Up;
        if (mask & MaskLeft)  return Side::Left;
        if (mask & MaskRight) return Side::Right;
        if (mask & MaskFront) return Side::Front;
        if (mask & MaskBack)  return Side::Back;
        return Side::Down;
    }

    /// Parse side name from text (common format used by PX4)
    /// @param text Side name: "down", "up", "left", "right", "front", "back"
    /// @return Corresponding Side enum value
    static Side parseSideText(const QString& text) {
        if (text == "down") return Side::Down;
        if (text == "up")   return Side::Up;
        if (text == "left") return Side::Left;
        if (text == "right") return Side::Right;
        if (text == "front") return Side::Front;
        if (text == "back") return Side::Back;
        return Side::Down;
    }

    /// Get display name for a side
    static QString sideName(Side side) {
        switch (side) {
        case Side::Down:  return QStringLiteral("Down");
        case Side::Up:    return QStringLiteral("Up");
        case Side::Left:  return QStringLiteral("Left");
        case Side::Right: return QStringLiteral("Right");
        case Side::Front: return QStringLiteral("Front");
        case Side::Back:  return QStringLiteral("Back");
        default:          return QStringLiteral("Unknown");
        }
    }
};

using CalibrationSide = SensorCalibrationSide::Side;
using CalibrationSideMask = SensorCalibrationSide::SideMask;
