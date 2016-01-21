        /*=====================================================================

     QGroundControl Open Source Ground Control Station

     (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

     This file is part of the QGROUNDCONTROL project

     QGROUNDCONTROL is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     QGROUNDCONTROL is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

     ======================================================================*/

    /// @file
    ///     @author Don Gagne <don@thegagnes.com>

    #ifndef AUTOPILOTPLUGIN_H
    #define AUTOPILOTPLUGIN_H

    #include <QObject>
    #include <QList>
    #include <QString>
    #include <QQmlContext>

    #include "VehicleComponent.h"
    #include "FactSystem.h"
    #include "Vehicle.h"

    class ParameterLoader;
    class Vehicle;
    class FirmwarePlugin;

    /// This is the base class for AutoPilot plugins
    ///
    /// The AutoPilotPlugin class is an abstract base class which represent the methods and objects
    /// which are specific to a certain AutoPilot. This is the only place where AutoPilot specific
    /// code should reside in QGroundControl. The remainder of the QGroundControl source is
    /// generic to a common mavlink implementation.

    class AutoPilotPlugin : public QObject
    {
        Q_OBJECT

    public:
        AutoPilotPlugin(Vehicle* vehicle, QObject* parent);
        ~AutoPilotPlugin();

        /// true: parameters are ready for use
        Q_PROPERTY(bool parametersReady READ parametersReady NOTIFY parametersReadyChanged)

        /// true: parameters are missing from firmware response, false: all parameters received from firmware
        Q_PROPERTY(bool missingParameters READ missingParameters NOTIFY missingParametersChanged)

        /// List of VehicleComponent objects
        Q_PROPERTY(QVariantList vehicleComponents READ vehicleComponents CONSTANT)

        /// false: One or more vehicle components require setup
        Q_PROPERTY(bool setupComplete READ setupComplete NOTIFY setupCompleteChanged)

        /// Reset all parameters to their default values
        Q_INVOKABLE void resetAllParametersToDefaults(void);

        /// Re-request the full set of parameters from the autopilot
        Q_INVOKABLE void refreshAllParameters(unsigned char componentID = MAV_COMP_ID_ALL);

        /// Request a refresh on the specific parameter
        Q_INVOKABLE void refreshParameter(int componentId, const QString& name);

        /// Request a refresh on all parameters that begin with the specified prefix
        Q_INVOKABLE void refreshParametersPrefix(int componentId, const QString& namePrefix);

        /// Returns true if the specifed parameter exists from the default component
        Q_INVOKABLE bool parameterExists(int componentId, const QString& name);

        /// Returns all parameter names
        QStringList parameterNames(int componentId);

        /// Returns the specified parameter Fact from the default component
        /// WARNING: Returns a default Fact if parameter does not exists. If that possibility exists, check for existence first with
        /// parameterExists.
        Fact* getParameterFact(int componentId, const QString& name);

        /// Writes the parameter facts to the specified stream
        void writeParametersToStream(QTextStream &stream);

        /// Reads the parameters from the stream and updates values
        /// @return Errors during load. Empty string for no errors
        QString readParametersFromStream(QTextStream &stream);

        /// Returns true if the specifed fact exists
        Q_INVOKABLE bool factExists(FactSystem::Provider_t  provider,       ///< fact provider
                                    int                     componentId,    ///< fact component, -1=default component
                                    const QString&          name);          ///< fact name

        /// Returns the specified Fact.
        /// WARNING: Will assert if fact does not exists. If that possibility exists, check for existence first with
        /// factExists.
        Fact* getFact(FactSystem::Provider_t    provider,       ///< fact provider
                      int                       componentId,    ///< fact component, -1=default component
                      const QString&            name);          ///< fact name

        const QMap<int, QMap<QString, QStringList> >& getGroupMap(void);

        // Must be implemented by derived class
        virtual const QVariantList& vehicleComponents(void) = 0;

        // Property accessors
        bool parametersReady(void) { return _parametersReady; }
        bool missingParameters(void) { return _missingParameters; }
        bool setupComplete(void);

        Vehicle* vehicle(void) { return _vehicle; }
        virtual void _parametersReadyPreChecks(bool parametersReady) = 0;

    signals:
        void parametersReadyChanged(bool parametersReady);
        void missingParametersChanged(bool missingParameters);
        void setupCompleteChanged(bool setupComplete);
        void parameterListProgress(float value);

    protected:
        /// All access to AutoPilotPugin objects is through getInstanceForAutoPilotPlugin
        AutoPilotPlugin(QObject* parent = NULL) : QObject(parent) { }

        Vehicle*        _vehicle;
        FirmwarePlugin* _firmwarePlugin;
        bool            _parametersReady;
        bool            _missingParameters;
        bool            _setupComplete;


    private slots:
        void _uasDisconnected(void);
        void _parametersReadyChanged(bool parametersReady);

    private:
        void _recalcSetupComplete(void);
    };

    #endif
