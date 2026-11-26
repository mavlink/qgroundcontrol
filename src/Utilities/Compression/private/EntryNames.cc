#include "EntryNames.h"

#include <QtCore/QList>
#include <QtCore/QStringList>

namespace LibArchive {

namespace {

/// Whether a component, without its extension, names a device on Windows, which then opens the device instead of a
/// file. Windows ignores spaces before the extension
bool isWindowsDeviceName(QStringView component)
{
    QStringView stem = component.left(component.indexOf(QLatin1Char('.')));
    while (stem.endsWith(QLatin1Char(' '))) {
        stem.chop(1);
    }
    for (const QLatin1StringView device :
         {QLatin1StringView("CON"), QLatin1StringView("PRN"), QLatin1StringView("AUX"), QLatin1StringView("NUL"),
          QLatin1StringView("CONIN$"), QLatin1StringView("CONOUT$")}) {
        if (stem.compare(device, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    // COM and LPT with a digit, also a superscript one
    if ((stem.size() != 4) || !(stem.startsWith(QLatin1StringView("COM"), Qt::CaseInsensitive) ||
                                stem.startsWith(QLatin1StringView("LPT"), Qt::CaseInsensitive))) {
        return false;
    }
    const char16_t digit = stem.at(3).unicode();
    return ((digit >= u'0') && (digit <= u'9')) || (digit == u'\u00B9') || (digit == u'\u00B2') || (digit == u'\u00B3');
}

}  // namespace

QString nonPortableNameReason(QStringView name)
{
    if ((name.size() > kMaxEntryNameBytes) || (name.toUtf8().size() > kMaxEntryNameBytes)) {
        return QStringLiteral("more than 4096 bytes");
    }

    constexpr qsizetype kMaxComponentBytes = 255;
    for (const QStringView component : name.tokenize(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        if ((component == QLatin1StringView(".")) || (component == QLatin1StringView(".."))) {
            continue;
        }
        for (const QChar character : component) {
            if ((character.unicode() < 0x20) || QStringView(u"<>:\"|?*\\").contains(character)) {
                return QStringLiteral("a character that Windows does not allow");
            }
        }
        if (component.endsWith(QLatin1Char('.')) || component.endsWith(QLatin1Char(' '))) {
            return QStringLiteral("a part that ends with a dot or a space");
        }
        if (isWindowsDeviceName(component)) {
            return QStringLiteral("a device name of Windows");
        }
        if (component.toUtf8().size() > kMaxComponentBytes) {
            return QStringLiteral("a part longer than 255 bytes");
        }
    }
    return {};
}

QString nameCollisionKey(QStringView name)
{
    return name.toString().normalized(QString::NormalizationForm_C).toCaseFolded();
}

QString EntrySpellings::claim(QStringView relativePath)
{
    const QList<QStringView> components = relativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // Follow the components already claimed
    quint32 parent = 0;
    qsizetype known = 0;
    for (; known < components.size(); ++known) {
        const auto claimed = _components.constFind({parent, nameCollisionKey(components.at(known))});
        if (claimed == _components.constEnd()) {
            break;
        }
        if (claimed->spelling != components.at(known)) {
            QStringList earlier;
            for (qsizetype i = 0; i < known; ++i) {
                earlier.append(components.at(i).toString());
            }
            earlier.append(claimed->spelling);
            return earlier.join(QLatin1Char('/'));
        }
        parent = claimed->id;
    }

    // Below a new component, every one is new
    for (; known < components.size(); ++known) {
        QString key = nameCollisionKey(components.at(known));
        QString spelling = components.at(known).toString();
        _storedCharacters += key.size() + spelling.size();
        _components.insert({parent, std::move(key)}, {++_lastId, std::move(spelling)});
        parent = _lastId;
    }
    return {};
}

}  // namespace LibArchive
