/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Arkadiusz Szulakiewicz <a.szulakiewicz@globallogic.com>

#pragma once

#include <QGeoCoordinate>
#include <QObject>

class QGeoCoordinate;

//! What3Words partial API implementation for converstion between GPS coordinates and What3Words addresses.
//! addresses
/*!
  What3Words API is available at https://docs.what3words.com/api/v3/
*/
class What3Words : public QObject
{
    Q_OBJECT

public:
    //! A constructor.
    What3Words(QObject *parent = Q_NULLPTR);

    //! A destructor.
    virtual ~What3Words() override;

    //! Method for converting What3Words address to GPS coordinate
    /*!
      \param address to be converted to GeoCoordinate
      \param geoCoordinate the second argument.
      \return returns true if conversion succeeded, false otherwise
    */
    bool convertToGeoCoordinate(const QString &address, QGeoCoordinate &geoCoordinate);

    //! Method for converting GPS coordinate to What3Words address
    /*!
      \sa testMe()
      \param geoCoordinate to convert from
      \param what3wordsAddress to convert to
      \return returns true if conversion succeeded, false otherwise
    */
    bool convertToWhat3WordsAddress(const QGeoCoordinate &geoCoordinate, QString &what3wordsAddress);

    //! Method for setting API key required to access What3Words API
    /*!
      \param apiKey API key
    */
    void setApiKey(const QString &apiKey);


    //! Method for accessing last error returned by API (if any) in human-readable way
    /*!
      \return apiKey ApiKey.
    */
    QString lastError();

private:
    class What3WordsImpl;
    QScopedPointer<What3WordsImpl> impl_;
};

