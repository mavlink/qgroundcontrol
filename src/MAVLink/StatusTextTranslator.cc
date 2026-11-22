#include "StatusTextTranslator.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QLocale>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QPair>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QRegularExpressionMatchIterator>
#include <QtCore/QStringList>

QGC_LOGGING_CATEGORY(StatusTextTranslatorLog, "MAVLink.StatusTextTranslator")

namespace {

// AP_Arming sends the same check failure as "PreArm: X" / "Pre-Arm: X" / "Arm: X"
const QStringList kArmingPrefixes = {
    QStringLiteral("PreArm:"),
    QStringLiteral("Pre-Arm:"),
    QStringLiteral("Arm:"),
};

// {N} placeholders inside the corpus templates
const QRegularExpression kPlaceholderRegex(QStringLiteral("\\{(\\d+)\\}"));

typedef QPair<QRegularExpression, QString> RegexEntry_t;

// Corpus is loaded lazily once; the tables are read-only afterwards
struct TranslationTables {
    QMutex mutex;
    bool loaded = false;
    bool valid = false;
    QHash<QString, QString> exact;
    QList<RegexEntry_t> regexes;  // corpus order
};

TranslationTables *translationTables()
{
    static TranslationTables tables;
    return &tables;
}

// Application ui locale, installed by QGCApplication::setLanguage().
QLocale &appLocale()
{
    static QLocale locale;  // default-constructed: follows the system locale until setLanguage() runs
    return locale;
}

void loadTables(TranslationTables &tables)
{
    QFile file(QStringLiteral(":/Mavlink/statustext_zhHans.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(StatusTextTranslatorLog) << "Failed to open STATUSTEXT translation corpus, translation disabled";
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        qCWarning(StatusTextTranslatorLog) << "Invalid STATUSTEXT translation corpus, translation disabled";
        return;
    }

    bool valid = true;

    const QJsonArray entries = doc.array();
    for (const QJsonValue &entryValue : entries) {
        const QJsonObject entry = entryValue.toObject();
        const QJsonObject match = entry.value(QStringLiteral("match")).toObject();
        const QString type = match.value(QStringLiteral("type")).toString();
        const QString matchValue = match.value(QStringLiteral("value")).toString();
        const QString zh = entry.value(QStringLiteral("zh")).toString();
        if (type.isEmpty() || matchValue.isEmpty() || zh.isEmpty()) {
            continue;
        }

        if (type == QStringLiteral("exact")) {
            tables.exact.insert(matchValue, zh);
        } else if (type == QStringLiteral("regex")) {
            const QRegularExpression regex(matchValue);
            if (!regex.isValid()) {
                qCWarning(StatusTextTranslatorLog) << "Invalid regex in STATUSTEXT corpus, translation disabled:"
                                                   << matchValue << regex.errorString();
                valid = false;
                break;
            }
            tables.regexes.append({regex, zh});
        }
    }

    if (!valid) {
        // keep the tables unusable so every lookup passes the raw text through
        tables.exact.clear();
        tables.regexes.clear();
        return;
    }

    tables.valid = true;
    qCDebug(StatusTextTranslatorLog) << "STATUSTEXT translation table loaded:" << tables.exact.count() << "exact,"
                                     << tables.regexes.count() << "regex";
}

const TranslationTables *ensureLoaded()
{
    TranslationTables *const t = translationTables();
    const QMutexLocker locker(&t->mutex);
    if (!t->loaded) {
        loadTables(*t);
        t->loaded = true;
    }
    return t;
}

// the corpus template references capture groups as {0} for group 1, {1} for group 2...
QString replacePlaceholders(const QString &templateText, const QRegularExpressionMatch &match)
{
    QString result;
    qsizetype lastEnd = 0;

    const QRegularExpressionMatchIterator it = kPlaceholderRegex.globalMatch(templateText);
    while (it.hasNext()) {
        const QRegularExpressionMatch placeholder = it.next();
        result += templateText.mid(lastEnd, placeholder.capturedStart() - lastEnd);

        bool ok = false;
        const qsizetype groupIndex = placeholder.captured(1).toLongLong(&ok) + 1;
        if (ok && (groupIndex <= match.lastCapturedIndex())) {
            result += match.captured(groupIndex);
        } else {
            result += placeholder.captured(0);
        }

        lastEnd = placeholder.capturedEnd();
    }

    result += templateText.mid(lastEnd);

    return result;
}

// the corpus regexes are all anchored (^...$), so a match() search is a full match here
QString lookup(const TranslationTables &tables, const QString &text)
{
    const auto it = tables.exact.constFind(text);
    if (it != tables.exact.cend()) {
        return it.value();
    }

    for (const RegexEntry_t &entry : tables.regexes) {
        const QRegularExpressionMatch match = entry.first.match(text);
        if (match.hasMatch()) {
            return replacePlaceholders(entry.second, match);
        }
    }

    return QString();
}

}  // namespace

void StatusTextTranslator::setLocale(const QLocale &locale)
{
    appLocale() = locale;
}

QString StatusTextTranslator::translate(const QString &raw)
{
    if (raw.isEmpty()) {
        return raw;
    }

    // Only apply when the ui language is chinese. The locale is installed
    // explicitly by QGCApplication::setLanguage(); a default-constructed
    // QLocale is not sufficient because setLanguage() skips QLocale::setDefault()
    // for en_US, which would leave the gate on the system language.
    if (appLocale().language() != QLocale::Chinese) {
        return raw;
    }

    const TranslationTables *const t = ensureLoaded();
    if (!t->valid) {
        return raw;
    }

    const QString hit = lookup(*t, raw);
    if (!hit.isNull()) {
        return hit;
    }

    // The same arming check failure is sent with different prefix tokens:
    // retry the other prefixes and put the raw prefix back into the display form.
    for (const QString &prefix : kArmingPrefixes) {
        if (!raw.startsWith(prefix)) {
            continue;
        }

        const QString body = raw.mid(prefix.length());
        for (const QString &other : kArmingPrefixes) {
            if (other == prefix) {
                continue;
            }

            const QString swappedHit = lookup(*t, other + body);
            if (swappedHit.isNull()) {
                continue;
            }

            for (const QString &token : kArmingPrefixes) {
                if (swappedHit.startsWith(token)) {
                    return prefix + swappedHit.mid(token.length());
                }
            }

            return swappedHit;
        }
    }

    return raw;
}
