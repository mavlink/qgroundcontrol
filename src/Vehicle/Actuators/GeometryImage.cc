/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeometryImage.h"

#include <QDir>

#include <array>

using namespace GeometryImage;


static void generateTestGeometries(VehicleGeometryImageProvider& provider)
{
#if 0 // enable this to generate a set of test geometry images on startup in the current working directory
    const QString imagePrefix = "test_geometry_";
    static bool generatedOnce = false;
    if (generatedOnce) {
        return;
    }
    generatedOnce = true;

    qWarning() << "Generating Test Geometry Images";

    const QList<ActuatorGeometry> geometries[] = {
        { // quad
            {ActuatorGeometry::Type::Motor, 1, QVector3D{1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-1, -1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{1, -1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-1, 1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
        },
        { // off-center quad
            {ActuatorGeometry::Type::Motor, 1, QVector3D{1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-0.5, -0.5, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{1, -0.5, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-0.5, 1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
        },
        { // weird penta
            {ActuatorGeometry::Type::Motor, 1, QVector3D{2, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-0.5, -0.5, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{2, -0.5, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-0.5, 1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 5, QVector3D{0.5, 1.5, 0}, ActuatorGeometry::SpinDirection::ClockWise},
        },
        { // octo
            {ActuatorGeometry::Type::Motor, 1, QVector3D{1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-1, -1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{1, -1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 5, QVector3D{1.4, 0, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 6, QVector3D{-1.4, 0, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 7, QVector3D{0, -1.4, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 8, QVector3D{0, 1.4, 0}, ActuatorGeometry::SpinDirection::ClockWise},
        },
        { // octo coax
            {ActuatorGeometry::Type::Motor, 1, QVector3D{1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-1, -1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{1, -1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-1, 1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 5, QVector3D{1, 1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 6, QVector3D{-1, -1, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 7, QVector3D{1, -1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 8, QVector3D{-1, 1, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
        },
        { // hex
            {ActuatorGeometry::Type::Motor, 1, QVector3D{1, 0.7, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 2, QVector3D{-1, -0.7, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 3, QVector3D{1, -0.7, 0}, ActuatorGeometry::SpinDirection::ClockWise},
            {ActuatorGeometry::Type::Motor, 4, QVector3D{-1, 0.7, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 5, QVector3D{0, -1.2, 0}, ActuatorGeometry::SpinDirection::CounterClockWise},
            {ActuatorGeometry::Type::Motor, 6, QVector3D{0, 1.2, 0}, ActuatorGeometry::SpinDirection::ClockWise},
        }
    };

    const QSize sizes[] = {
            { 160, 160 },
            { 120 * 2, 120 },
            { 160 * 2, 160 }, // default size
            { 250 * 2, 250 },
    };

    for (unsigned sizeIdx = 0; sizeIdx < sizeof(sizes) / sizeof(sizes[0]); ++sizeIdx) {
        const QSize& size = sizes[sizeIdx];
        for (unsigned geometryIdx = 0; geometryIdx < sizeof(geometries) / sizeof(geometries[0]); ++geometryIdx) {
            provider.actuators() = geometries[geometryIdx];
            QPixmap pixmap = provider.requestPixmap("", nullptr, size);

            QString imageFileName = QDir(QDir::currentPath()).filePath(imagePrefix + QString::number(sizeIdx)+"_"
                    +QString::number(geometryIdx)+".png");
            qWarning() << "Generating image" << imageFileName;
            QFile file(imageFileName);
            file.open(QIODevice::WriteOnly);
            pixmap.save(&file, "PNG");
        }
    }

#endif
}

VehicleGeometryImageProvider::VehicleGeometryImageProvider()
: QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    generateTestGeometries(*this);
}

void VehicleGeometryImageProvider::drawAxisIndicator(QPainter& p, const QPointF& origin, float fontSize, const QColor& color)
{
    const float lineLength = fontSize * 2.f;
    const float arrowWidth = 6.f;
    const float arrowHeight = 8.f;

    p.setPen(QPen{color, 1.5f});
    p.setBrush(color);

    QFont font = p.font();
    font.setPixelSize(fontSize);
    p.setFont(font);

    auto drawArrow = [&](const QPointF& start, const QPointF& end) {
        float lineLength = QLineF{start, end}.length();
        p.save();
        p.translate(end);
        float angle = atan2f(end.y()-start.y(), end.x()-start.x());
        p.rotate(angle * (180.f / M_PI) + 90.f);
        p.drawLine(QPointF{0, arrowHeight/2}, QPointF{0, lineLength});
        QPointF arrow[3] = {
            QPointF{0.f - arrowWidth/2.f, arrowHeight},
            QPointF{0.f, 0.f},
            QPointF{0.f + arrowWidth/2.f, arrowHeight},
        };
        p.drawConvexPolygon(arrow, sizeof(arrow) / sizeof(arrow[0]));
        p.restore();
    };

    // upwards
    {
        QPointF endPos{origin.x(), origin.y() - lineLength};
        drawArrow(origin, endPos);

        p.save();
        p.translate(endPos);
        QRectF textRect{-lineLength, -lineLength, lineLength * 2.f, lineLength};
        p.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, "x");
        p.restore();
    }

    // rightwards
    {
        QPointF endPos{origin.x() + lineLength, origin.y()};
        drawArrow(origin, endPos);

        p.save();
        p.translate(endPos);
        QRectF textRect{0, -lineLength / 2, lineLength, lineLength};
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignBaseline, " y");
        p.restore();
    }
}

QPixmap VehicleGeometryImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    int width = requestedSize.width();
    int height = requestedSize.height();
    if (size)
        *size = QSize(width, height);

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    _actuatorImagePositions.clear();

    // get the dimensions
    QVector3D min{1e10f, 1e10f, 1e10f};
    QVector3D max{-1e10f, -1e10f, -1e10f};
    for (const auto& actuator : _actuators) {
        for (int i = 0; i < 3; ++i) {
            if (actuator.position[i] < min[i]) {
                min[i] = actuator.position[i];
            }
            if (actuator.position[i] > max[i]) {
                max[i] = actuator.position[i];
            }
        }
    }

    if (_actuators.size() <= 1 || max.x() - min.x() < 0.0001f || max.y() - min.y() < 0.0001f ) {
        return pixmap;
    }

    // separate actuators, check for coax (on top of each other)
    QList<ActuatorGeometry> actuators;
    QList<ActuatorGeometry> coaxActuators;
    for (const auto& actuator : _actuators) {
        if (actuator.type == ActuatorGeometry::Type::Motor) {
            bool isCoax = false;
            for (const auto& actuatorBefore : _actuators) {
                if (actuatorBefore.type == ActuatorGeometry::Type::Motor) {
                    if (&actuatorBefore == &actuator) {
                        break;
                    }
                    QVector2D diff = actuatorBefore.position.toVector2D() - actuator.position.toVector2D();
                    if (diff.length() < 0.03f) {
                        coaxActuators.append(actuator);
                        isCoax = true;
                        break;
                    }
                }
            }
            if (!isCoax) {
                actuators.append(actuator);
            }
        }
    }

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const float axisIndicatorSize = 15.f; // font size
    const float margin = 5.f; // from image borders

    // scaling & center offset
    float usableWidth = width - 2.f * margin;
    float usableHeight = height - 2.f * margin;
    float extraOffsetX = 0.f;
    // if there's not enough space on the left for the axis to ensure there's no overlap, reduce the usable size
    if (width < height + axisIndicatorSize * 4.f) {
        usableWidth -= axisIndicatorSize;
        usableHeight -= axisIndicatorSize;
        extraOffsetX = axisIndicatorSize;
    }

    const float rotorDiameter = std::min(usableWidth, usableHeight) * (_actuators.length() <= 6 ? 0.31f : 0.25f);
    const float fontSize = rotorDiameter * 0.4f;
    const float extraYMargin = coaxActuators.length() > 0 ? fontSize * 1.1f : 0.f;

    const float scaleX = (usableWidth - rotorDiameter) / (max.y() - min.y());
    const float scaleY = (usableHeight - extraYMargin - rotorDiameter) / (max.x() - min.x());
    const float scale = std::min(scaleX, scaleY);
    const float offsetX = margin + extraOffsetX + usableWidth / 2.f - (max.y() + min.y()) / 2.f * scale;
    const float offsetY = margin + (usableHeight - extraYMargin) / 2.f + (max.x() + min.x()) / 2.f * scale;

    // style
    const QColor clockWiseColor{ 21, 158, 31, 200 };
    const QColor counterClockWiseColor{ 78, 195, 232, 200 };
    const QColor frameArrowColor{ 255, 68, 43, 200 };
    const QColor frameColor{ 150, 150, 150 };
    const float frameWidth{ 6.f };
    const float rotorFontSize{ rotorDiameter * 0.4f };
    const QColor rotorHighlightColor{ frameArrowColor };
    const QColor fontColor{ _palette.text() };

    auto iterateMotors = [scale, offsetX, offsetY](const QList<ActuatorGeometry> &actuators,
            std::function<void(const ActuatorGeometry&, QPointF)> draw) {
                for (const auto& actuator : actuators) {
                    if (actuator.type == ActuatorGeometry::Type::Motor) {
                        QPointF pos{
                            offsetX + actuator.position.y()*scale,
                            offsetY - actuator.position.x()*scale
                        };
                        draw(actuator, pos);
                    }
                }
            };

    // draw line from center to actuators first
    iterateMotors(actuators, [&](const ActuatorGeometry& actuator, QPointF pos) {
        p.setPen(QPen{frameColor, frameWidth});
        p.drawLine(QPointF{offsetX, offsetY}, pos);
    });

    // frame body
    p.setPen(frameColor);
    p.setBrush(QBrush{frameColor});
    float centerSize = rotorDiameter * 0.8f;
    p.drawRoundedRect(QRectF{offsetX - centerSize / 2.f, offsetY - centerSize / 2.f, centerSize, centerSize}, frameWidth, frameWidth);
    p.setPen(frameArrowColor);
    p.setBrush(frameArrowColor);
    float arrowWidth = rotorDiameter / 4.f;
    float arrowHeight = rotorDiameter / 2.f;
    QPointF arrow[3] = {
            QPointF{offsetX - arrowWidth / 2.f, offsetY + arrowHeight / 2.f},
            QPointF{offsetX,                    offsetY - arrowHeight / 2.f},
            QPointF{offsetX + arrowWidth / 2.f, offsetY + arrowHeight / 2.f},
    };
    p.drawConvexPolygon(arrow, sizeof(arrow) / sizeof(arrow[0]));

    drawAxisIndicator(p, QPointF{axisIndicatorSize / 2.f, height - axisIndicatorSize / 2.f}, axisIndicatorSize, fontColor);

    auto drawMotor = [&](const ActuatorGeometry& actuator, QPointF pos, float yPosOffset, bool labelAtBottom) {
        p.setPen(Qt::NoPen);
        QColor arrowColor;
        if (actuator.spinDirection == ActuatorGeometry::SpinDirection::ClockWise) {
            p.setBrush(QBrush{clockWiseColor});
            arrowColor = clockWiseColor;
        } else {
            p.setBrush(QBrush{counterClockWiseColor});
            arrowColor = counterClockWiseColor;
        }
        arrowColor.setAlpha(255);
        if (_palette.globalTheme() == QGCPalette::Light) {
            arrowColor = arrowColor.darker(200);
        } else {
            arrowColor = arrowColor.lighter(130);
        }

        pos.setY(pos.y() + yPosOffset);

        p.drawEllipse(pos, rotorDiameter/2, rotorDiameter/2);

        QRectF textRect;
        if (labelAtBottom) {
            textRect = QRectF{pos.x()-rotorDiameter/2.f, pos.y()+rotorDiameter/2.f-yPosOffset, rotorDiameter, yPosOffset};
        } else {
            textRect = QRectF{pos.x()-rotorDiameter/2.f, pos.y()-rotorDiameter/2.f, rotorDiameter, rotorDiameter};
        }
        if (actuator.renderOptions.highlight) {
            p.setPen(rotorHighlightColor);
            p.setBrush(rotorHighlightColor);
            float radius;
            if (labelAtBottom) {
                radius = rotorFontSize / 2.f;
            } else {
                radius = rotorDiameter / 2.f;
            }
            p.drawEllipse(textRect.center(), radius, radius);
            _actuatorImagePositions.append(ImagePosition{actuator.type, actuator.index, textRect.center(), radius});
        }
        p.setPen(fontColor);
        QFont font = p.font();
        font.setPixelSize(rotorFontSize);
        p.setFont(font);
        p.drawText(textRect, Qt::AlignCenter, QString::number(actuator.index + actuator.labelIndexOffset));

        // spin direction arrows
        int angle = 50;// angle for the whole arc
        float arrowWidth = frameWidth;
        float arrowHeight = frameWidth * 1.25f;
        float arrowPosition = rotorDiameter / 2.f;
        p.setPen(QPen{arrowColor, 2.5f});
        p.setBrush(arrowColor);
        std::array<int, 2> angleOffsets;
        if (labelAtBottom) {
            angleOffsets = {30, 180-30}; // bottom right + left sides
        } else {
            angleOffsets = {0, 180}; // right + left sides
        }
        for (int angleOffset : angleOffsets) {
            p.save();
            p.translate(pos);
            float ySign = 1.f;
            if (actuator.spinDirection == ActuatorGeometry::SpinDirection::ClockWise) {
                p.rotate(angle / 2.f + angleOffset);
                ySign = -1.f;
            } else {
                p.rotate(-angle / 2.f + angleOffset);
            }
            QRectF arrowRect{-arrowPosition, -arrowPosition, arrowPosition * 2.f, arrowPosition * 2.f};
            p.drawArc(arrowRect, 0, -ySign * 16 * angle);
            QPointF arrow[3] = {
                QPointF{arrowPosition - arrowWidth/2.f, ySign*arrowHeight/2.f},
                QPointF{arrowPosition,                  -ySign*arrowHeight/2.f},
                QPointF{arrowPosition + arrowWidth/2.f, ySign*arrowHeight/2.f},
            };
            p.drawConvexPolygon(arrow, sizeof(arrow) / sizeof(arrow[0]));
            p.restore();
        }
    };

    // draw coax motors
    iterateMotors(coaxActuators, [&](const ActuatorGeometry& actuator, QPointF pos) {
        drawMotor(actuator, pos, extraYMargin, true);
    });

    // draw the rest of the motors
    iterateMotors(actuators, [&](const ActuatorGeometry& actuator, QPointF pos) {
        drawMotor(actuator, pos, 0.f, false);
    });

    return pixmap;
}

VehicleGeometryImageProvider* VehicleGeometryImageProvider::instance()
{
    static VehicleGeometryImageProvider* instance = nullptr;
    // The instance is managed & deleted by the QML engine
    if (!instance)
        instance = new VehicleGeometryImageProvider();
    return instance;
}

int VehicleGeometryImageProvider::getHighlightedMotorIndexAtPos(const QPointF &position)
{
    int foundIdx = -1;
    for (int i = 0; i < _actuatorImagePositions.size(); ++i) {
        if (_actuatorImagePositions[i].type == ActuatorGeometry::Type::Motor) {
            float radius = _actuatorImagePositions[i].radius;
            if (QLineF{_actuatorImagePositions[i].position, position}.length() < radius) {
                // in case of multiple matches (overlaps), be safe and do not return any match
                if (foundIdx != -1) {
                    return -1;
                }
                foundIdx = i;
            }
        }
    }
    if (foundIdx >= 0) {
        return _actuatorImagePositions[foundIdx].index;
    }
    return -1;
}
