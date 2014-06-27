#ifndef _QWT_DATE_SCALE_DRAW_H_
#define _QWT_DATE_SCALE_DRAW_H_ 1

#include "qwt_global.h"
#include "qwt_scale_draw.h"
#include "qwt_date.h"

/*!
  \brief A class for drawing datetime scales

  QwtDateScaleDraw displays values as datetime labels.
  The format of the labels depends on the alignment of
  the major tick labels.

  The default format strings are:

  - Millisecond\n
    "hh:mm:ss:zzz\nddd dd MMM yyyy"
  - Second\n
    "hh:mm:ss\nddd dd MMM yyyy"
  - Minute\n
    "hh:mm\nddd dd MMM yyyy"
  - Hour\n
    "hh:mm\nddd dd MMM yyyy"
  - Day\n
    "ddd dd MMM yyyy"
  - Week\n
    "Www yyyy"
  - Month\n
    "MMM yyyy"
  - Year\n
    "yyyy"

  The format strings can be modified using setDateFormat()
  or individually for each tick label by overloading dateFormatOfDate(),

  Usually QwtDateScaleDraw is used in combination with
  QwtDateScaleEngine, that calculates scales for datetime
  intervals.

  \sa QwtDateScaleEngine, QwtPlot::setAxisScaleDraw()
*/
class QWT_EXPORT QwtDateScaleDraw: public QwtScaleDraw
{
public:
    QwtDateScaleDraw( Qt::TimeSpec = Qt::LocalTime );
    virtual ~QwtDateScaleDraw();

    void setDateFormat( QwtDate::IntervalType, const QString & );
    QString dateFormat( QwtDate::IntervalType ) const;

    void setTimeSpec( Qt::TimeSpec );
    Qt::TimeSpec timeSpec() const;

    void setUtcOffset( int seconds );
    int utcOffset() const;

    void setWeek0Type( QwtDate::Week0Type );
    QwtDate::Week0Type week0Type() const;

    virtual QwtText label( double ) const;

    QDateTime toDateTime( double ) const;

protected:
    virtual QwtDate::IntervalType 
        intervalType( const QwtScaleDiv & ) const;

    virtual QString dateFormatOfDate( const QDateTime &,
        QwtDate::IntervalType ) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
