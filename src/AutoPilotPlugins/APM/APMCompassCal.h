/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMCompassCal_H
#define APMCompassCal_H

#include <QObject>
#include <QThread>
#include <QVector3D>

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(APMCompassCalLog)

class CalWorkerThread : public QThread
{
    Q_OBJECT

public:
    CalWorkerThread(Vehicle* vehicle, QObject* parent = nullptr);

    // Cancel currently in progress calibration
    void cancel(void) { _cancel = true; }

    // Overrides from QThread
    void run(void) Q_DECL_FINAL;

    static const unsigned max_mags = 3;

    bool                    rgCompassAvailable[max_mags];
    QMutex                  lastScaledImuMutex;
    mavlink_raw_imu_t       lastRawImu;
    mavlink_scaled_imu_t    rgLastScaledImu[max_mags];

    static const char*      rgCompassParams[3][4];

signals:
    void vehicleTextMessage(int vehicleId, int compId, int severity, QString text);

private:
    void _emitVehicleTextMessage(const QString& message);

    // The routines below are based on the PX4 flight stack compass cal routines. Hence
    // the PX4 Flight Stack coding style to maintain some level of code movement.

    static const float mag_sphere_radius;
    static const unsigned int calibration_sides;			///< The total number of sides
    static const unsigned int calibration_total_points;     ///< The total points per magnetometer
    static const unsigned int calibraton_duration_seconds;  ///< The total duration the routine is allowed to take

    // The order of these cannot change since the calibration calculations depend on them in this order
    enum detect_orientation_return {
        DETECT_ORIENTATION_TAIL_DOWN,
        DETECT_ORIENTATION_NOSE_DOWN,
        DETECT_ORIENTATION_LEFT,
        DETECT_ORIENTATION_RIGHT,
        DETECT_ORIENTATION_UPSIDE_DOWN,
        DETECT_ORIENTATION_RIGHTSIDE_UP,
        DETECT_ORIENTATION_ERROR
    };
    static const unsigned detect_orientation_side_count = 6;

    // Data passed to calibration worker routine
    typedef struct  {
        unsigned        done_count;
        unsigned int	calibration_points_perside;
        unsigned int	calibration_interval_perside_seconds;
        uint64_t        calibration_interval_perside_useconds;
        unsigned int	calibration_counter_total[max_mags];
        bool            side_data_collected[detect_orientation_side_count];
        float*          x[max_mags];
        float*          y[max_mags];
        float*          z[max_mags];
    } mag_worker_data_t;

    enum calibrate_return {
        calibrate_return_ok,
        calibrate_return_error,
        calibrate_return_cancelled
    };

    /**
     * Least-squares fit of a sphere to a set of points.
     *
     * Fits a sphere to a set of points on the sphere surface.
     *
     * @param x point coordinates on the X axis
     * @param y point coordinates on the Y axis
     * @param z point coordinates on the Z axis
     * @param size number of points
     * @param max_iterations abort if maximum number of iterations have been reached. If unsure, set to 100.
     * @param delta abort if error is below delta. If unsure, set to 0 to run max_iterations times.
     * @param sphere_x coordinate of the sphere center on the X axis
     * @param sphere_y coordinate of the sphere center on the Y axis
     * @param sphere_z coordinate of the sphere center on the Z axis
     * @param sphere_radius sphere radius
     *
     * @return 0 on success, 1 on failure
     */
    int sphere_fit_least_squares(const float x[], const float y[], const float z[],
                     unsigned int size, unsigned int max_iterations, float delta, float *sphere_x, float *sphere_y, float *sphere_z,
                     float *sphere_radius);

    /// Wait for vehicle to become still and detect it's orientation
    ///	@return Returns detect_orientation_return according to orientation of still vehicle
    enum detect_orientation_return detect_orientation(void);

    /// Returns the human readable string representation of the orientation
    ///	@param orientation Orientation to return string for, "error" if buffer is too small
    const char* detect_orientation_str(enum detect_orientation_return orientation);

    /// Perform calibration sequence which require a rest orientation detection prior to calibration.
    ///	@return OK: Calibration succeeded, ERROR: Calibration failed
    calibrate_return calibrate_from_orientation(
                            bool	side_data_collected[detect_orientation_side_count],	///< Sides for which data still needs calibration
                            void*	worker_data);                                       ///< Opaque data passed to worker routine

    calibrate_return calibrate(void);
    calibrate_return mag_calibration_worker(detect_orientation_return orientation, void* data);
    unsigned progress_percentage(mag_worker_data_t* worker_data);

    Vehicle*    _vehicle;
    bool        _cancel;
};

// Used to calibrate APM Stack compass by simulating PX4 Flight Stack firmware compass cal
// on the ground station side of things.
class APMCompassCal : public QObject
{
    Q_OBJECT
    
public:
    APMCompassCal(void);
    ~APMCompassCal();

    void setVehicle(Vehicle* vehicle);
    void startCalibration(void);
    void cancelCalibration(void);
    
signals:
    void vehicleTextMessage(int vehicleId, int compId, int severity, QString text);

private slots:
    void _handleMavlinkRawImu(mavlink_message_t message);
    void _handleMavlinkScaledImu2(mavlink_message_t message);
    void _handleMavlinkScaledImu3(mavlink_message_t message);

private:
    void _setSensorTransmissionSpeed(bool fast);
    void _stopCalibration(void);
    void _emitVehicleTextMessage(const QString& message);

    Vehicle*            _vehicle;
    CalWorkerThread*    _calWorkerThread;
    float               _rgSavedCompassOffsets[3][3];
};

#endif

