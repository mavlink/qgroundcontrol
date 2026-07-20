#pragma once

#include <QtCore/QObject>
#include <QtGui/QFont>
#include <QtQmlIntegration/QtQmlIntegration>

class SystemFonts : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QFont fixedFont READ fixedFont CONSTANT FINAL)

public:
    explicit SystemFonts(QObject* parent = nullptr);
    ~SystemFonts() override = default;

    static QFont fixedFont();
};
