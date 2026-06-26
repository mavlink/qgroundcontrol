#include "GenericMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QUrl>

// Percent-encode a user-supplied token before substituting it into a URL so it
// cannot inject query/path separators.
static QString encodeToken(const QString& token)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(token));
}

QString CustomURLMapProvider::_getURL(int x, int y, int zoom) const
{
    QString url = factString(appSettings()->customURL());
    (void) url.replace("{x}", QString::number(x));
    (void) url.replace("{y}", QString::number(y));
    static const QRegularExpression zoomRegExp("\\{(z|zoom)\\}");
    (void) url.replace(zoomRegExp, QString::number(zoom));

    // SSRF guard: a user-settings URL must be http(s) and must not target the
    // loopback or link-local ranges. Callers treat an empty URL as "no tile".
    const QUrl parsed(url, QUrl::StrictMode);
    const QString scheme = parsed.scheme().toLower();
    if (!parsed.isValid() || ((scheme != QStringLiteral("http")) && (scheme != QStringLiteral("https")))) {
        return QString();
    }
    const QString host = parsed.host().toLower();
    if (host.isEmpty() || (host == QStringLiteral("localhost")) || host.startsWith(QStringLiteral("127.")) ||
        host.startsWith(QStringLiteral("169.254.")) || (host == QStringLiteral("::1"))) {
        return QString();
    }

    return url;
}

QString TemplateMapProvider::_getURL(int x, int y, int zoom) const
{
    const int tileY = _config.flipY ? ((1 << zoom) - 1 - y) : y;

    QString url = _config.urlTemplate;
    if (_config.serverCount > 0) {
        url = url.arg(_getServerNum(x, y, _config.serverCount));
    }
    if (!_config.mapTypeId.isEmpty()) {
        url = url.arg(_config.mapTypeId);
    }
    url = url.arg(zoom);
    if (_config.axisOrder == MapProviderConfig::ZYX) {
        url = url.arg(tileY).arg(x);
    } else {
        url = url.arg(x).arg(tileY);
    }
    if (_config.appendImageFormat) {
        url = url.arg(_imageFormat);
    }

    return url;
}

QString LINZBasemapMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString apiKey = factString(appSettings()->linzToken());

    QString url = _mapUrl.arg(zoom).arg(x).arg(y).arg(_imageFormat);

    if (!apiKey.isEmpty()) {
        url += QStringLiteral("?api=%1").arg(encodeToken(apiKey));
    }

    return url;
}

QString OpenAIPMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString apiKey = factString(appSettings()->openaipToken());

    QString url = _mapUrl.arg(zoom).arg(x).arg(y);

    if (!apiKey.isEmpty()) {
        url += QStringLiteral("?apiKey=%1").arg(encodeToken(apiKey));
    }

    return url;
}

QString VWorldMapProvider::_getURL(int x, int y, int zoom) const
{
    if ((zoom < 5) || (zoom > 19)) {
        return QString();
    }

    const int gap = zoom - 6;
    const qint64 span = static_cast<qint64>(1) << gap;

    const qint64 x_min = 53 * span;
    const qint64 x_max = (55 * span) + (2 * gap - 1);
    if ((x < x_min) || (x > x_max)) {
        return QString();
    }

    const qint64 y_min = 22 * span;
    const qint64 y_max = (26 * span) + (2 * gap - 1);
    if ((y < y_min) || (y > y_max)) {
        return QString();
    }

    const QString VWorldMapToken = encodeToken(factString(appSettings()->vworldToken()));
    return _mapUrl.arg(VWorldMapToken, _mapTypeId).arg(zoom).arg(y).arg(x).arg(_imageFormat);
}
