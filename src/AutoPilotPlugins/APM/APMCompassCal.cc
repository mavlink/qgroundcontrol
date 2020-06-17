/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMCompassCal.h"
#include "AutoPilotPlugin.h"
#include "ParameterManager.h"

QGC_LOGGING_CATEGORY(APMCompassCalLog, "APMCompassCalLog")

const float CalWorkerThread::mag_sphere_radius = 0.2f;
const unsigned int CalWorkerThread::calibration_sides = 6;
const unsigned int CalWorkerThread::calibration_total_points = 240;
const unsigned int CalWorkerThread::calibraton_duration_seconds = CalWorkerThread::calibration_sides * 10;

const char* CalWorkerThread::rgCompassParams[3][4] = {
    { "COMPASS_OFS_X", "COMPASS_OFS_Y", "COMPASS_OFS_Z", "COMPASS_DEV_ID" },
    { "COMPASS_OFS2_X", "COMPASS_OFS2_Y", "COMPASS_OFS2_Z", "COMPASS_DEV_ID2" },
    { "COMPASS_OFS3_X", "COMPASS_OFS3_Y", "COMPASS_OFS3_Z", "COMPASS_DEV_ID3" },
};

CalWorkerThread::CalWorkerThread(Vehicle* vehicle, QObject* parent)
    : QThread(parent)
    , _vehicle(vehicle)
    , _cancel(false)
{

}

void CalWorkerThread::run(void)
{
    if (calibrate() == calibrate_return_ok) {
        _emitVehicleTextMessage(QStringLiteral("[cal] progress <100>"));
        _emitVehicleTextMessage(QStringLiteral("[cal] calibration done: mag"));
    }
}

void CalWorkerThread::_emitVehicleTextMessage(const QString& message)
{
    emit vehicleTextMessage(_vehicle->id(), 0, MAV_SEVERITY_INFO, message);
    qCDebug(APMCompassCalLog) << message;
}

unsigned CalWorkerThread::progress_percentage(mag_worker_data_t* worker_data)
{
    return 100 * ((float)worker_data->done_count) / calibration_sides;
}

CalWorkerThread::calibrate_return CalWorkerThread::calibrate(void)
{
    calibrate_return result = calibrate_return_ok;

    mag_worker_data_t worker_data;

    worker_data.done_count = 0;
    worker_data.calibration_points_perside = calibration_total_points / calibration_sides;
    worker_data.calibration_interval_perside_seconds = calibraton_duration_seconds / calibration_sides;
    worker_data.calibration_interval_perside_useconds = worker_data.calibration_interval_perside_seconds * 1000 * 1000;

    // Collect data for all sides
    worker_data.side_data_collected[DETECT_ORIENTATION_RIGHTSIDE_UP] =  false;
    worker_data.side_data_collected[DETECT_ORIENTATION_LEFT] =          false;
    worker_data.side_data_collected[DETECT_ORIENTATION_NOSE_DOWN] =     false;
    worker_data.side_data_collected[DETECT_ORIENTATION_TAIL_DOWN] =     false;
    worker_data.side_data_collected[DETECT_ORIENTATION_UPSIDE_DOWN] =   false;
    worker_data.side_data_collected[DETECT_ORIENTATION_RIGHT] =         false;

    for (size_t cur_mag=0; cur_mag<max_mags; cur_mag++) {
        // Initialize to no memory allocated
        worker_data.x[cur_mag] = nullptr;
        worker_data.y[cur_mag] = nullptr;
        worker_data.z[cur_mag] = nullptr;
        worker_data.calibration_counter_total[cur_mag] = 0;
    }

    const unsigned int calibration_points_maxcount = calibration_sides * worker_data.calibration_points_perside;

    for (size_t cur_mag=0; cur_mag<max_mags; cur_mag++) {
        if (rgCompassAvailable[cur_mag]) {
            worker_data.x[cur_mag] = reinterpret_cast<float *>(malloc(sizeof(float) * calibration_points_maxcount));
            worker_data.y[cur_mag] = reinterpret_cast<float *>(malloc(sizeof(float) * calibration_points_maxcount));
            worker_data.z[cur_mag] = reinterpret_cast<float *>(malloc(sizeof(float) * calibration_points_maxcount));
            if (worker_data.x[cur_mag] == nullptr || worker_data.y[cur_mag] == nullptr || worker_data.z[cur_mag] == nullptr) {
                _emitVehicleTextMessage(QStringLiteral("[cal] ERROR: out of memory"));
                result = calibrate_return_error;
            }
        }
    }

    if (result == calibrate_return_ok) {
        result = calibrate_from_orientation(
                    worker_data.side_data_collected,    // Sides to calibrate
                    &worker_data);                      // Opaque data for calibration worked
    }

    // Calculate calibration values for each mag

    float sphere_x[max_mags];
    float sphere_y[max_mags];
    float sphere_z[max_mags];
    float sphere_radius[max_mags];

    // Sphere fit the data to get calibration values
    if (result == calibrate_return_ok) {
        for (unsigned cur_mag=0; cur_mag<max_mags; cur_mag++) {
            if (rgCompassAvailable[cur_mag]) {
                sphere_fit_least_squares(worker_data.x[cur_mag], worker_data.y[cur_mag], worker_data.z[cur_mag],
                                         worker_data.calibration_counter_total[cur_mag],
                                         100, 0.0f,
                                         &sphere_x[cur_mag], &sphere_y[cur_mag], &sphere_z[cur_mag],
                                         &sphere_radius[cur_mag]);

                if (qIsNaN(sphere_x[cur_mag]) || qIsNaN(sphere_y[cur_mag]) || qIsNaN(sphere_z[cur_mag])) {
                    _emitVehicleTextMessage(QStringLiteral("[cal] ERROR: NaN in sphere fit for mag %1").arg(cur_mag));
                    result = calibrate_return_error;
                }
            }
        }
    }

    // Data points are no longer needed
    for (size_t cur_mag=0; cur_mag<max_mags; cur_mag++) {
        free(worker_data.x[cur_mag]);
        free(worker_data.y[cur_mag]);
        free(worker_data.z[cur_mag]);
    }

    if (result == calibrate_return_ok) {
        for (unsigned cur_mag=0; cur_mag<max_mags; cur_mag++) {
            if (rgCompassAvailable[cur_mag]) {
                _emitVehicleTextMessage(QStringLiteral("[cal] mag #%1 off: x:%2 y:%3 z:%4").arg(cur_mag).arg(-sphere_x[cur_mag]).arg(-sphere_y[cur_mag]).arg(-sphere_z[cur_mag]));

                float sensorId = 0.0f;
                if (cur_mag == 0) {
                    sensorId = 2.0f;
                } else if (cur_mag == 1) {
                    sensorId = 5.0f;
                } else if (cur_mag == 2) {
                    sensorId = 6.0f;
                }
                if (sensorId != 0.0f) {
                    _vehicle->sendMavCommand(_vehicle->defaultComponentId(),
                                             MAV_CMD_PREFLIGHT_SET_SENSOR_OFFSETS,
                                             true, /* showErrors */
                                             sensorId, -sphere_x[cur_mag], -sphere_y[cur_mag], -sphere_z[cur_mag]);
                }
            }
        }
    }

    return result;
}

CalWorkerThread::calibrate_return CalWorkerThread::mag_calibration_worker(detect_orientation_return orientation, void* data)
{
    calibrate_return result = calibrate_return_ok;

    unsigned int calibration_counter_side;

    mag_worker_data_t* worker_data = (mag_worker_data_t*)(data);

    _emitVehicleTextMessage(QStringLiteral("[cal] Rotate vehicle around the detected orientation"));
    _emitVehicleTextMessage(QStringLiteral("[cal] Continue rotation for %1 seconds").arg(worker_data->calibration_interval_perside_seconds));

    uint64_t calibration_deadline = QGC::groundTimeUsecs() + worker_data->calibration_interval_perside_useconds;

    unsigned int loop_interval_usecs = (worker_data->calibration_interval_perside_seconds * 1000000) / worker_data->calibration_points_perside;

    calibration_counter_side = 0;

    while (QGC::groundTimeUsecs() < calibration_deadline && calibration_counter_side < worker_data->calibration_points_perside) {
        if (_cancel) {
            result = calibrate_return_cancelled;
            break;
        }

        for (size_t cur_mag=0; cur_mag<max_mags; cur_mag++) {
            if (!rgCompassAvailable[cur_mag]) {
                continue;
            }

            lastScaledImuMutex.lock();
            mavlink_scaled_imu_t copyLastScaledImu = rgLastScaledImu[cur_mag];
            lastScaledImuMutex.unlock();

            worker_data->x[cur_mag][worker_data->calibration_counter_total[cur_mag]] = copyLastScaledImu.xmag;
            worker_data->y[cur_mag][worker_data->calibration_counter_total[cur_mag]] = copyLastScaledImu.ymag;
            worker_data->z[cur_mag][worker_data->calibration_counter_total[cur_mag]] = copyLastScaledImu.zmag;
            worker_data->calibration_counter_total[cur_mag]++;
        }

        calibration_counter_side++;

        // Progress indicator for side
        _emitVehicleTextMessage(QStringLiteral("[cal] %1 side calibration: progress <%2>").arg(detect_orientation_str(orientation)).arg(progress_percentage(worker_data) +
                                                                                                                                        (unsigned)((100 / calibration_sides) * ((float)calibration_counter_side / (float)worker_data->calibration_points_perside))));

        usleep(loop_interval_usecs);
    }

    if (result == calibrate_return_ok) {
        _emitVehicleTextMessage(QStringLiteral("[cal] %1 side done, rotate to a different side").arg(detect_orientation_str(orientation)));

        worker_data->done_count++;
        _emitVehicleTextMessage(QStringLiteral("[cal] progress <%1>").arg(progress_percentage(worker_data)));
    }

    return result;
}

CalWorkerThread::calibrate_return CalWorkerThread::calibrate_from_orientation(
        bool	side_data_collected[detect_orientation_side_count],
        void*	worker_data)
{
    calibrate_return result = calibrate_return_ok;

    unsigned orientation_failures = 0;

    // Rotate through all requested orientations
    while (true) {
        if (_cancel) {
            result = calibrate_return_cancelled;
            break;
        }

        unsigned int side_complete_count = 0;

        // Update the number of completed sides
        for (unsigned i = 0; i < detect_orientation_side_count; i++) {
            if (side_data_collected[i]) {
                side_complete_count++;
            }
        }

        if (side_complete_count == detect_orientation_side_count) {
            // We have completed all sides, move on
            break;
        }

        /* inform user which orientations are still needed */
        char pendingStr[256];
        pendingStr[0] = 0;

        for (unsigned int cur_orientation=0; cur_orientation<detect_orientation_side_count; cur_orientation++) {
            if (!side_data_collected[cur_orientation]) {
                strcat(pendingStr, " ");
                strcat(pendingStr, detect_orientation_str((enum detect_orientation_return)cur_orientation));
            }
        }
        _emitVehicleTextMessage(QStringLiteral("[cal] pending:%1").arg(pendingStr));

        _emitVehicleTextMessage(QStringLiteral("[cal] hold vehicle still on a pending side"));
        enum detect_orientation_return orient = detect_orientation();

        if (orient == DETECT_ORIENTATION_ERROR) {
            orientation_failures++;
            _emitVehicleTextMessage(QStringLiteral("[cal] detected motion, hold still..."));
            continue;
        }

        /* inform user about already handled side */
        if (side_data_collected[orient]) {
            orientation_failures++;
            _emitVehicleTextMessage(QStringLiteral("%1 side already completed").arg(detect_orientation_str(orient)));
            _emitVehicleTextMessage(QStringLiteral("rotate to a pending side"));
            continue;
        }

        _emitVehicleTextMessage(QStringLiteral("[cal] %1 orientation detected").arg(detect_orientation_str(orient)));
        orientation_failures = 0;

        // Call worker routine
        result = mag_calibration_worker(orient, worker_data);
        if (result != calibrate_return_ok ) {
            break;
        }

        _emitVehicleTextMessage(QStringLiteral("[cal] %1 side done, rotate to a different side").arg(detect_orientation_str(orient)));

        // Note that this side is complete
        side_data_collected[orient] = true;
        usleep(200000);
    }

    return result;
}

enum CalWorkerThread::detect_orientation_return CalWorkerThread::detect_orientation(void)
{
    bool    stillDetected = false;
    quint64 stillDetectTime;

    int16_t lastX = 0;
    int16_t lastY = 0;
    int16_t lastZ = 0;

    while (true) {
        lastScaledImuMutex.lock();
        mavlink_raw_imu_t   copyLastRawImu = lastRawImu;
        lastScaledImuMutex.unlock();

        int16_t xDelta = abs(lastX - copyLastRawImu.xacc);
        int16_t yDelta = abs(lastY - copyLastRawImu.yacc);
        int16_t zDelta = abs(lastZ - copyLastRawImu.zacc);

        lastX = copyLastRawImu.xacc;
        lastY = copyLastRawImu.yacc;
        lastZ = copyLastRawImu.zacc;

        if (xDelta < 100 && yDelta < 100 && zDelta < 100) {
            if (stillDetected) {
                if (QGC::groundTimeMilliseconds() - stillDetectTime > 1000) {
                    break;
                }
            } else {
                stillDetectTime = QGC::groundTimeMilliseconds();
                stillDetected = true;
            }
        } else {
            stillDetected = false;
        }

        if (_cancel) {
            break;
        }

        // FIXME: No timeout for never detect still

        usleep(1000);
    }

    static const uint16_t rawImuOneG = 800;
    static const uint16_t rawImuNoGThreshold = 200;

    if (lastX > rawImuOneG && abs(lastY) < rawImuNoGThreshold && abs(lastZ) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_TAIL_DOWN;        // [ g, 0, 0 ]
    }

    if (lastX < -rawImuOneG && abs(lastY) < rawImuNoGThreshold && abs(lastZ) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_NOSE_DOWN;        // [ -g, 0, 0 ]
    }

    if (lastY > rawImuOneG && abs(lastX) < rawImuNoGThreshold && abs(lastZ) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_LEFT;        // [ 0, g, 0 ]
    }

    if (lastY < -rawImuOneG && abs(lastX) < rawImuNoGThreshold && abs(lastZ) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_RIGHT;        // [ 0, -g, 0 ]
    }

    if (lastZ > rawImuOneG && abs(lastX) < rawImuNoGThreshold && abs(lastY) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_UPSIDE_DOWN;        // [ 0, 0, g ]
    }

    if (lastZ < -rawImuOneG && abs(lastX) < rawImuNoGThreshold && abs(lastY) < rawImuNoGThreshold) {
        return DETECT_ORIENTATION_RIGHTSIDE_UP;        // [ 0, 0, -g ]
    }

    _emitVehicleTextMessage(QStringLiteral("[cal] ERROR: invalid orientation"));

    return DETECT_ORIENTATION_ERROR;	// Can't detect orientation
}

const char* CalWorkerThread::detect_orientation_str(enum detect_orientation_return orientation)
{
    static const char* rgOrientationStrs[] = {
        "back",		// tail down
        "front",	// nose down
        "left",
        "right",
        "up",		// upside-down
        "down",		// right-side up
        "error"
    };

    return rgOrientationStrs[orientation];
}

int CalWorkerThread::sphere_fit_least_squares(const float x[], const float y[], const float z[],
                                              unsigned int size, unsigned int max_iterations, float delta, float *sphere_x, float *sphere_y, float *sphere_z,
                                              float *sphere_radius)
{

    float x_sumplain = 0.0f;
    float x_sumsq = 0.0f;
    float x_sumcube = 0.0f;

    float y_sumplain = 0.0f;
    float y_sumsq = 0.0f;
    float y_sumcube = 0.0f;

    float z_sumplain = 0.0f;
    float z_sumsq = 0.0f;
    float z_sumcube = 0.0f;

    float xy_sum = 0.0f;
    float xz_sum = 0.0f;
    float yz_sum = 0.0f;

    float x2y_sum = 0.0f;
    float x2z_sum = 0.0f;
    float y2x_sum = 0.0f;
    float y2z_sum = 0.0f;
    float z2x_sum = 0.0f;
    float z2y_sum = 0.0f;

    for (unsigned int i = 0; i < size; i++) {

        float x2 = x[i] * x[i];
        float y2 = y[i] * y[i];
        float z2 = z[i] * z[i];

        x_sumplain += x[i];
        x_sumsq += x2;
        x_sumcube += x2 * x[i];

        y_sumplain += y[i];
        y_sumsq += y2;
        y_sumcube += y2 * y[i];

        z_sumplain += z[i];
        z_sumsq += z2;
        z_sumcube += z2 * z[i];

        xy_sum += x[i] * y[i];
        xz_sum += x[i] * z[i];
        yz_sum += y[i] * z[i];

        x2y_sum += x2 * y[i];
        x2z_sum += x2 * z[i];

        y2x_sum += y2 * x[i];
        y2z_sum += y2 * z[i];

        z2x_sum += z2 * x[i];
        z2y_sum += z2 * y[i];
    }

    //
    //Least Squares Fit a sphere A,B,C with radius squared Rsq to 3D data
    //
    //    P is a structure that has been computed with the data earlier.
    //    P.npoints is the number of elements; the length of X,Y,Z are identical.
    //    P's members are logically named.
    //
    //    X[n] is the x component of point n
    //    Y[n] is the y component of point n
    //    Z[n] is the z component of point n
    //
    //    A is the x coordiante of the sphere
    //    B is the y coordiante of the sphere
    //    C is the z coordiante of the sphere
    //    Rsq is the radius squared of the sphere.
    //
    //This method should converge; maybe 5-100 iterations or more.
    //
    float x_sum = x_sumplain / size;        //sum( X[n] )
    float x_sum2 = x_sumsq / size;    //sum( X[n]^2 )
    float x_sum3 = x_sumcube / size;    //sum( X[n]^3 )
    float y_sum = y_sumplain / size;        //sum( Y[n] )
    float y_sum2 = y_sumsq / size;    //sum( Y[n]^2 )
    float y_sum3 = y_sumcube / size;    //sum( Y[n]^3 )
    float z_sum = z_sumplain / size;        //sum( Z[n] )
    float z_sum2 = z_sumsq / size;    //sum( Z[n]^2 )
    float z_sum3 = z_sumcube / size;    //sum( Z[n]^3 )

    float XY = xy_sum / size;        //sum( X[n] * Y[n] )
    float XZ = xz_sum / size;        //sum( X[n] * Z[n] )
    float YZ = yz_sum / size;        //sum( Y[n] * Z[n] )
    float X2Y = x2y_sum / size;    //sum( X[n]^2 * Y[n] )
    float X2Z = x2z_sum / size;    //sum( X[n]^2 * Z[n] )
    float Y2X = y2x_sum / size;    //sum( Y[n]^2 * X[n] )
    float Y2Z = y2z_sum / size;    //sum( Y[n]^2 * Z[n] )
    float Z2X = z2x_sum / size;    //sum( Z[n]^2 * X[n] )
    float Z2Y = z2y_sum / size;    //sum( Z[n]^2 * Y[n] )

    //Reduction of multiplications
    float F0 = x_sum2 + y_sum2 + z_sum2;
    float F1 =  0.5f * F0;
    float F2 = -8.0f * (x_sum3 + Y2X + Z2X);
    float F3 = -8.0f * (X2Y + y_sum3 + Z2Y);
    float F4 = -8.0f * (X2Z + Y2Z + z_sum3);

    //Set initial conditions:
    float A = x_sum;
    float B = y_sum;
    float C = z_sum;

    //First iteration computation:
    float A2 = A * A;
    float B2 = B * B;
    float C2 = C * C;
    float QS = A2 + B2 + C2;
    float QB = -2.0f * (A * x_sum + B * y_sum + C * z_sum);

    //Set initial conditions:
    float Rsq = F0 + QB + QS;

    //First iteration computation:
    float Q0 = 0.5f * (QS - Rsq);
    float Q1 = F1 + Q0;
    float Q2 = 8.0f * (QS - Rsq + QB + F0);
    float aA, aB, aC, nA, nB, nC, dA, dB, dC;

    //Iterate N times, ignore stop condition.
    unsigned int n = 0;

#undef  FLT_EPSILON
#define FLT_EPSILON 1.1920929e-07F  /* 1E-5 */

    while (n < max_iterations) {
        n++;

        //Compute denominator:
        aA = Q2 + 16.0f * (A2 - 2.0f * A * x_sum + x_sum2);
        aB = Q2 + 16.0f * (B2 - 2.0f * B * y_sum + y_sum2);
        aC = Q2 + 16.0f * (C2 - 2.0f * C * z_sum + z_sum2);
        aA = (fabsf(aA) < FLT_EPSILON) ? 1.0f : aA;
        aB = (fabsf(aB) < FLT_EPSILON) ? 1.0f : aB;
        aC = (fabsf(aC) < FLT_EPSILON) ? 1.0f : aC;

        //Compute next iteration
        nA = A - ((F2 + 16.0f * (B * XY + C * XZ + x_sum * (-A2 - Q0) + A * (x_sum2 + Q1 - C * z_sum - B * y_sum))) / aA);
        nB = B - ((F3 + 16.0f * (A * XY + C * YZ + y_sum * (-B2 - Q0) + B * (y_sum2 + Q1 - A * x_sum - C * z_sum))) / aB);
        nC = C - ((F4 + 16.0f * (A * XZ + B * YZ + z_sum * (-C2 - Q0) + C * (z_sum2 + Q1 - A * x_sum - B * y_sum))) / aC);

        //Check for stop condition
        dA = (nA - A);
        dB = (nB - B);
        dC = (nC - C);

        if ((dA * dA + dB * dB + dC * dC) <= delta) { break; }

        //Compute next iteration's values
        A = nA;
        B = nB;
        C = nC;
        A2 = A * A;
        B2 = B * B;
        C2 = C * C;
        QS = A2 + B2 + C2;
        QB = -2.0f * (A * x_sum + B * y_sum + C * z_sum);
        Rsq = F0 + QB + QS;
        Q0 = 0.5f * (QS - Rsq);
        Q1 = F1 + Q0;
        Q2 = 8.0f * (QS - Rsq + QB + F0);
    }

    *sphere_x = A;
    *sphere_y = B;
    *sphere_z = C;
    *sphere_radius = sqrtf(Rsq);

    return 0;
}

APMCompassCal::APMCompassCal(void)
    : _vehicle(nullptr)
    , _calWorkerThread(nullptr)
{

}

APMCompassCal::~APMCompassCal()
{
    if (_calWorkerThread) {
        _calWorkerThread->terminate();
        // deleteLater so it happens on correct thread
        _calWorkerThread->deleteLater();
    }
}

void APMCompassCal::setVehicle(Vehicle* vehicle)
{
    if (!vehicle) {
        qWarning() << "vehicle == NULL";
    }

    _vehicle = vehicle;
}

void APMCompassCal::startCalibration(void)
{
    _setSensorTransmissionSpeed(true /* fast */);
    connect (_vehicle, &Vehicle::mavlinkRawImu,     this, &APMCompassCal::_handleMavlinkRawImu);
    connect (_vehicle, &Vehicle::mavlinkScaledImu2, this, &APMCompassCal::_handleMavlinkScaledImu2);
    connect (_vehicle, &Vehicle::mavlinkScaledImu3, this, &APMCompassCal::_handleMavlinkScaledImu3);

    // Simulate a start message
    _emitVehicleTextMessage(QStringLiteral("[cal] calibration started: mag"));

    _calWorkerThread = new CalWorkerThread(_vehicle);
    connect(_calWorkerThread, &CalWorkerThread::vehicleTextMessage, this, &APMCompassCal::vehicleTextMessage);

    // Clear the offset parameters so we get raw data
    for (int i=0; i<3; i++) {
        _calWorkerThread->rgCompassAvailable[i] = true;

        const char* deviceIdParam = CalWorkerThread::rgCompassParams[i][3];
        if (_vehicle->parameterManager()->parameterExists(-1, deviceIdParam)) {
            _calWorkerThread->rgCompassAvailable[i] = _vehicle->parameterManager()->getParameter(-1, deviceIdParam)->rawValue().toInt() > 0;
            for (int j=0; j<3; j++) {
                const char* offsetParam = CalWorkerThread::rgCompassParams[i][j];
                Fact* paramFact = _vehicle->parameterManager()->getParameter(-1, offsetParam);

                _rgSavedCompassOffsets[i][j] = paramFact->rawValue().toFloat();
                paramFact->setRawValue(0.0);
            }
        } else {
            _calWorkerThread->rgCompassAvailable[i] = false;
        }
        qCDebug(APMCompassCalLog) << QStringLiteral("Compass %1 available: %2").arg(i).arg(_calWorkerThread->rgCompassAvailable[i]);
    }

    _calWorkerThread->start();
}

void APMCompassCal::cancelCalibration(void)
{
    _stopCalibration();

    // Put the original offsets back
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            const char* offsetParam = CalWorkerThread::rgCompassParams[i][j];
            if (_vehicle->parameterManager()->parameterExists(-1, offsetParam)) {
                _vehicle->parameterManager()->getParameter(-1, offsetParam)-> setRawValue(_rgSavedCompassOffsets[i][j]);
            }
        }
    }

    // Simulate a cancelled message
    _emitVehicleTextMessage(QStringLiteral("[cal] calibration cancelled"));
}

void APMCompassCal::_handleMavlinkRawImu(mavlink_message_t message)
{
    _calWorkerThread->lastScaledImuMutex.lock();
    mavlink_msg_raw_imu_decode(&message, &_calWorkerThread->lastRawImu);
    _calWorkerThread->rgLastScaledImu[0].xacc = _calWorkerThread->lastRawImu.xacc;
    _calWorkerThread->rgLastScaledImu[0].yacc = _calWorkerThread->lastRawImu.yacc;
    _calWorkerThread->rgLastScaledImu[0].zacc = _calWorkerThread->lastRawImu.zacc;
    _calWorkerThread->rgLastScaledImu[0].xgyro = _calWorkerThread->lastRawImu.xgyro;
    _calWorkerThread->rgLastScaledImu[0].ygyro = _calWorkerThread->lastRawImu.ygyro;
    _calWorkerThread->rgLastScaledImu[0].zgyro = _calWorkerThread->lastRawImu.zgyro;
    _calWorkerThread->rgLastScaledImu[0].xmag = _calWorkerThread->lastRawImu.xmag;
    _calWorkerThread->rgLastScaledImu[0].ymag = _calWorkerThread->lastRawImu.ymag;
    _calWorkerThread->rgLastScaledImu[0].zmag = _calWorkerThread->lastRawImu.zmag;
    _calWorkerThread->lastScaledImuMutex.unlock();
}

void APMCompassCal::_handleMavlinkScaledImu2(mavlink_message_t message)
{
    _calWorkerThread->lastScaledImuMutex.lock();
    mavlink_msg_scaled_imu2_decode(&message, (mavlink_scaled_imu2_t*)&_calWorkerThread->rgLastScaledImu[1]);
    _calWorkerThread->lastScaledImuMutex.unlock();
}

void APMCompassCal::_handleMavlinkScaledImu3(mavlink_message_t message)
{
    _calWorkerThread->lastScaledImuMutex.lock();
    mavlink_msg_scaled_imu3_decode(&message, (mavlink_scaled_imu3_t*)&_calWorkerThread->rgLastScaledImu[2]);
    _calWorkerThread->lastScaledImuMutex.unlock();
}

void APMCompassCal::_setSensorTransmissionSpeed(bool fast)
{
    _vehicle->requestDataStream(MAV_DATA_STREAM_RAW_SENSORS, fast ? 10 : 2);
}

void APMCompassCal::_stopCalibration(void)
{
    _calWorkerThread->cancel();
    disconnect (_vehicle, &Vehicle::mavlinkRawImu,      this, &APMCompassCal::_handleMavlinkRawImu);
    disconnect (_vehicle, &Vehicle::mavlinkScaledImu2,  this, &APMCompassCal::_handleMavlinkScaledImu2);
    disconnect (_vehicle, &Vehicle::mavlinkScaledImu3,  this, &APMCompassCal::_handleMavlinkScaledImu3);
    _setSensorTransmissionSpeed(false /* fast */);
}

void APMCompassCal::_emitVehicleTextMessage(const QString& message)
{
    qCDebug(APMCompassCalLog()) << message;
    emit vehicleTextMessage(_vehicle->id(), 0, MAV_SEVERITY_INFO, message);
}
