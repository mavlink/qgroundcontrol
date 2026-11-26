#include "EntryNames.h"

#include <QtCore/QList>
#include <QtCore/QStringList>

#include "QGCFileHelper.h"

namespace LibArchive {

QString nonPortableNameReason(QStringView name)
{
    if ((name.size() > kMaxEntryNameBytes) || (name.toUtf8().size() > kMaxEntryNameBytes)) {
        return QStringLiteral("more than 4096 bytes");
    }

    for (const QStringView component : name.tokenize(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        if ((component == QLatin1StringView(".")) || (component == QLatin1StringView(".."))) {
            continue;
        }
        switch (QGCFileHelper::fileNameIssue(component)) {
            case QGCFileHelper::FileNameIssue::None:
            case QGCFileHelper::FileNameIssue::Empty:
                break;
            case QGCFileHelper::FileNameIssue::ForbiddenCharacter:
                return QStringLiteral("a character that Windows does not allow");
            case QGCFileHelper::FileNameIssue::TrailingDotOrSpace:
                return QStringLiteral("a part that ends with a dot or a space");
            case QGCFileHelper::FileNameIssue::DeviceName:
                return QStringLiteral("a device name of Windows");
            case QGCFileHelper::FileNameIssue::TooLong:
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
