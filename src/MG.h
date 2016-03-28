/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Helper functions
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */


#ifndef _MG_H_
#define _MG_H_

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <cmath>

namespace MG
{
const static int MAX_FLIGHT_TIME = 60 * 60 * 24 * 21;

class VERSION
{
public:

    static const int MAJOR = 1;
    static const int MINOR = 01;
};

class SYSTEM
{
public:

    static const int ID = 127;
    static const int COMPID = 0;
    static int getID() {
        return SYSTEM::ID;
    }
};

class UNITS
{
public:

    /** The distance units supported for display by the groundstation **/
    enum DistanceUnit {
        METER,
        CENTIMETER,
        MILLIMETER,
        INCH,
        FEET,
        MILE
    };

    /**
     * @brief Convert a distance in meters into another distance unit system
     *
     * @param in The distance to convert
     * @param newUnit The new unit to convert to
     *
     * @return The converted distance
     */
    static double convertFromMeter(double in, DistanceUnit newUnit) {
        double result = in;

        // Conversion table in meter
        static const double inInch = 39.3700787;
        static const double inFeet = 3.2808399;
        static const double inMile = 0.000000621371192;
        static const double inCentimeter = 100;
        static const double inMillimeter = 1000;

        switch (newUnit) {
        case METER:
            result = in;
            break;
        case CENTIMETER:
            result = in * inCentimeter;
            break;
        case MILLIMETER:
            result = in * inMillimeter;
            break;
        case INCH:
            result = in * inInch;
            break;
        case FEET:
            result = in * inFeet;
            break;
        case MILE:
            result = in * inMile;
            break;
        }

        return result;
    }

    /**
     * @brief Convert between two distance units
     *
     * This convenience function allows to convert between arbitrary distance units
     *
     * @param in The input distance
     * @param inputUnit The input unit
     * @param outputUnit The output unit
     *
     * @return The converted distance
     */
    static double convert(double in, DistanceUnit inputUnit, DistanceUnit outputUnit) {
        double meters = convertToMeter(in, inputUnit);
        return convertFromMeter(meters, outputUnit);
    }

    /**
     * @brief Convert a distance to the meter unit
     *
     * @param in The distance to convert
     * @param inputUnit The unit the distance is represented in
     *
     * @return The converted distance
     */
    static double convertToMeter(double in, DistanceUnit inputUnit) {

        double result = in;


        // Conversion table in meter
        static const double inInch = 39.3700787;
        static const double inFeet = 3.2808399;
        static const double inMile = 0.000000621371192;
        static const double inCentimeter = 100;
        static const double inMillimeter = 1000;

        // Don't convert if new unit is same unit

        switch (inputUnit) {
        case METER:
            result = in;
            break;
        case CENTIMETER:
            result = in / inCentimeter;
            break;
        case MILLIMETER:
            result = in / inMillimeter;
            break;
        case INCH:
            result = in / inInch;
            break;
        case FEET:
            result = in / inFeet;
            break;
        case MILE:
            result = in / inMile;
            break;
        }

        return result;
    }

};

class DISPLAY
{

public:

    DISPLAY() {
        // Initialize static class display with notebook display default value
        //pixelSize = 0.224f;
        //setPixelSize(0.224f);
    }

    ~DISPLAY() {

    }

    /**
     * @brief Get the size of a single pixel
     *
     * This value can be used to generate user interfaces with
     * a size in physical units, for example a gauge which is
     * always 50.8 mm (2") in diameter, regardless of the screen.
     *
     * @return The horizontal and vertical size of a pixel-square
     */
    static double getPixelSize() {
        return 0.224f;
    }

    /**
     * @brief Set the size of a single pixel
     *
     * @param size The horizontal and vertical size of a pixel-square
     */
    static void setPixelSize(double size) {
        pixelSize = size;
    }

    /**
     * @brief Set the size of a single pixel
     *
     * This method calculates the pixel size from the vertical and horizontal
     * resolution and the screen diameter. The diameter is in mm (as this unit
     * is a SI unit). One inch = 25.4 mm
     *
     * @param horizontalResolution The horizontal screen resolution, e.g. 1280.
     * @param verticalResolution The vertical screen resolution, e.g. 800.
     * @param screenDiameter The screen diameter in mm, e.g. 13.3" = 338 mm.
     */
    static void setPixelSize(int horizontalResolution, int verticalResolution, double screenDiameter) {
        pixelSize = screenDiameter / sqrt(static_cast<float>(horizontalResolution*horizontalResolution + verticalResolution*verticalResolution));
    }

private:
    /** The size of a single pixel **/
    static double pixelSize;
};

class STAT
{
    /** The time interval for the last few moments in milliseconds **/
    static const int SHORT_TERM_INTERVAL = 300;
    /** The time interval for the last moment in milliseconds **/
    static const int CURRENT_INTERVAL = 50;
};

class TIME
{

public:

    //static const QString ICONDIR = "./icons";

    /**
     * @brief Convenience method to get the milliseconds time stamp for now
     *
     * The timestamp is created at the instant of calling this method. It is
     * defined as the number of milliseconds since unix epoch, which is
     * 1.1.1970, 00:00 UTC.
     *
     * @return The number of milliseconds elapsed since unix epoch
     * @deprecated Will the replaced by time helper class
     **/
    static quint64 getGroundTimeNow() {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();
        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 milliseconds = time.toTime_t() * static_cast<quint64>(1000);
        return static_cast<quint64>(milliseconds + time.time().msec());
    }

    /**
     * @brief Convenience method to get the milliseconds time stamp for now
     *
     * The timestamp is created at the instant of calling this method. It is
     * defined as the number of milliseconds since unix epoch, which is
     * 1.1.1970, 00:00 UTC.
     *
     * @return The number of milliseconds elapsed since unix epoch
     * @deprecated Will the replaced by time helper class
     **/
    static quint64 getGroundTimeNowUsecs() {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();
        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);
        return static_cast<quint64>(microseconds + (time.time().msec()*1000));
    }

    /*tatic quint64 getMissionTimeUsecs()
    {
        ;
    }*/

    /**
     * Convert milliseconds to an QDateTime object. This method converts the amount of
     * milliseconds since 1.1.1970, 00:00 UTC (unix epoch) to a QDateTime date object.
     *
     * @param msecs The milliseconds since unix epoch (in Qt unsigned 64bit integer type quint64)
     * @return The QDateTime object set to corresponding date and time
     * @deprecated Will the replaced by time helper class
     **/
    static QDateTime msecToQDateTime(quint64 msecs) {
        QDateTime time = QDateTime();
        /* Set date and time depending on the seconds since unix epoch,
             * integer division truncates the milliseconds */
        time.setTime_t(msecs / 1000);
        /* Add the milliseconds, modulo returns the milliseconds part */
        return time.addMSecs(msecs % 1000);
    }

};
}

#endif // _MG_H_
