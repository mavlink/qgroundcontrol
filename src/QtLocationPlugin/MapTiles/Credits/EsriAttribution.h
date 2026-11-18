#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

class QNetworkAccessManager;
class QNetworkReply;

/// The credits that Esri asks for beside "Powered by Esri": the sources of what a map shows, which depend on the
/// area and on the zoom level. Esri publishes them for a map service as a list of contributors, each with the areas
/// and zoom levels that it is credited for and a score for each area.
///
/// A list is fetched when it is first asked about and kept for the run. A fetch that fails, or that gets an answer
/// which is no such list, is made again for a later question, once the retry delay has passed. A list that the
/// server does not have (HTTP 404) is not asked for again.
///
/// For the main thread only.
class EsriAttribution : public QObject
{
    Q_OBJECT

public:
    /// \a networkManager is for tests: null takes the network manager of the map engine
    explicit EsriAttribution(QNetworkAccessManager* networkManager = nullptr, QObject* parent = nullptr);

    /// The contributors that the list at \a listUrl credits for an area at the tile zoom level \a zoom: the one
    /// with the best score there first, joined with ", ", each name once. Contributors of the same score are in the
    /// order of the list. Empty until the list has arrived, which listReceived() tells, and for an empty address.
    ///
    /// The area lies between the latitudes \a south and \a north and reaches from the longitude \a west eastwards to
    /// \a east, in degrees. Longitudes beyond 180 degrees are wrapped, and an \a east that is less than \a west is
    /// east of the antimeridian: 170 to 190 and 170 to -170 are the same area across it. An area of 360 degrees or
    /// more has every longitude. An area that touches one of a contributor is credited to it.
    QString credits(const QUrl& listUrl, double south, double west, double north, double east, int zoom);

    /// For tests, which cannot wait for the delay after which a failed fetch is made again
    void setRetryDelay(std::chrono::milliseconds delay) { _retryDelay = delay; }

signals:
    /// The list at \a listUrl has arrived: credits() answers from it
    void listReceived(const QUrl& listUrl);

private:
    /// A rectangle of latitudes and longitudes and the zoom levels at which a contributor is credited there
    struct Area
    {
        double south = 0.;
        double west = 0.;
        double north = 0.;
        double east = 0.;
        double score = 0.;
        int zoomMin = 0;
        int zoomMax = 0;
        int name = 0;  ///< Index in List::names
    };

    struct List
    {
        enum class State
        {
            Fetching,
            Received,
            Failed,       ///< Asked for again once the retry delay has passed
            Unavailable,  ///< The server has no such list
        };

        State state = State::Fetching;
        QElapsedTimer sinceFailure;
        QStringList names;
        QList<Area> areas;  ///< Best score first, so that the names of a scan are in the order of the credits
    };

    void _fetch(const QUrl& listUrl, List& list);
    void _replyFinished(QNetworkReply* reply, const QUrl& listUrl);
    static void _setFailed(List& list);
    /// Fills \a list from \a body. False for a body that is not a list of contributors.
    static bool _parse(QByteArray body, List& list);

    QPointer<QNetworkAccessManager> _networkManager;
    QHash<QUrl, List> _lists;
    std::chrono::milliseconds _retryDelay = kRetryDelay;

    static constexpr std::chrono::milliseconds kRetryDelay = std::chrono::minutes(1);
};
