#include "SystemFonts.h"

#include <QtGui/QFontDatabase>

SystemFonts::SystemFonts(QObject* parent) : QObject(parent) {}

QFont SystemFonts::fixedFont()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}
