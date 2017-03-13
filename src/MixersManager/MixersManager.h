/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MixersManager_H
#define MixersManager_H

#include <QObject>
#include <QLoggingCategory>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <FactMetaData.h>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "LinkInterface.h"
#include "MixerFacts.h"
#include "MixerMetaData.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(MixersManagerLog)

class MixersManager : public QObject
{
    Q_OBJECT
    
public:
    MixersManager(Vehicle* vehicle);
    ~MixersManager();

    /// true: Parameters are ready for use
    Q_PROPERTY(bool mixerDataReady READ mixerDataReady NOTIFY mixerDataReadyChanged)
    bool mixerDataReady(void) { return _mixerDataReady; }

    
    bool inProgress(void);

    bool requestMixerCount(unsigned int group);
    bool requestSubmixerCount(unsigned int group, unsigned int mixer);
    bool requestMixerType(unsigned int group, unsigned int mixer, unsigned int submixer);
    bool requestParameterCount(unsigned int group, unsigned int mixer, unsigned int submixer);
    bool requestParameter(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int parameter);
    bool requestMixerAll(unsigned int group);
    bool requestMissingData(unsigned int group);
    bool requestConnectionCount(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType);
    bool requestConnection(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType, unsigned conn);

    MixerGroup* getMixerGroup(unsigned int groupID);
    MixerMetaData* getMixerMetaData() {return &_mixerMetaData;}

    // These values are public so the unit test can set appropriate signal wait times
    static const int _ackTimeoutMilliseconds = 1000;
    static const int _maxRetryCount = 5;
    
signals:
    void mixerDataReadyChanged(bool mixerDataReady);
    void missingMixerDataChanged(bool missingMixerData);

protected:
    void _paramValueUpdated(const QVariant& value);
    
private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _ackTimeout(void);

private:
    typedef enum {
        AckNone,            ///< State machine is idle
        AckMixersCount,     ///< MIXER_DATA mixers count message expected
        AckSubmixersCount,  ///< MIXER_DATA submixers count message expected
        AckMixerType,       ///< MIXER_DATA mixer type value message expected
        AckParameterCount,  ///< MIXER_DATA parameter value message expected
        AckGetParameter,    ///< MIXER_DATA parameter value message expected
        AckSetParameter,    ///< MIXER_DATA parameter value message expected
        AckAll,             ///< ALL mixer data expected
    } AckType_t;


private:
    Vehicle*            _vehicle;
    LinkInterface*      _dedicatedLink;

    MixerGroups         _mixerGroupsData;
    MixerMetaData       _mixerMetaData;

    QList<mavlink_mixer_data_t*> _mixerDataMessages;
    QTimer*             _ackTimeoutTimer;
    AckType_t           _expectedAck;
    int                 _retryCount;
    bool                _getMissing;
    unsigned int        _requestGroup;

    bool                _mixerDataReady;               ///< true: mixer data load complete
    bool                _missingMixerData;             ///< true: mixer data missing from load

    void _startAckTimeout(AckType_t ack);

    void mixerDataDownloadComplete(unsigned int group);

    //* Return index of matching message,group,mixer,submixer and parameter etc..*/
    int _getMessageOfKind(const mavlink_mixer_data_t* data);

    ///* Collect mixer data into a list.  Only one list entry per group, data_type, mixer, submixer etc...
    /// The test against data is dependent on the mixer data type*/
    bool _collectMixerData(const mavlink_mixer_data_t* data);

    ///* Request a missing message. true if there is missing data */
    bool _requestMissingData(unsigned int group);

//    ///* Build mixer Fact database from the messages collected*/
//    bool _buildFactsFromMessages(unsigned int group);

    ///* Build all mixer data structure and content from whatever source is available
    /// return true if successfull*/
    bool _buildAll(unsigned int group);


    ///* Build mixer structure from messages.  This only includes mixers and submixers with type facts
    /// return true if successfull*/
    bool _buildStructureFromMessages(unsigned int group);

    ///* Build parameters from included headers.  TODO: DEPRECIATE AND CHANGE TO FILE INSTEAD OF HEADERS
    ///  return true if successfull*/
    bool _buildParametersFromHeaders(unsigned int group);

    ///* Build connections
    /// return true if successfull*/
    bool _buildConnections(unsigned int group);

    ///* Set parameter values from mixer data messages
    /// return true if successfull*/
    bool _parameterValuesFromMessages(unsigned int group);

    ///* Set connection points from mixer data messages
    /// return true if successfull*/
    bool _connectionsFromMessages(unsigned int group);

    ///* Get mixer connection count from whatever vehicle data source is available*/
    int _getMixerConnCountFromVehicle(int mixerType, int connType);

    ///* Set parameter Fact value from whatever vehicle data source is available*/
    void _setParameterFactFromVehicle(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int param, Fact* paramFact);

    ///* Set parameter Fact value from downloaded message*/
    void _setParameterFactFromMessage(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int param, Fact* paramFact);

    ///* Set connection values from whatever vehicle data source is available*/
    void _setMixerConnectionFromVehicle(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int connType, unsigned int connIndex, MixerConnection* conn );

    ///* Set connection values from downloaded message*/
    void _setMixerConnectionFromMessage(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int connType, unsigned int connIndex, MixerConnection* conn );


//    bool _checkForExpectedAck(AckType_t receivedAck);

//    bool        _readTransactionInProgress;
//    bool        _writeTransactionInProgress;
//    QList<int>  _itemIndicesToRead;     ///< List of mission items which still need to be requested from vehicle
    
//    QMutex _dataMutex;
    
//    QList<MissionItem*> _missionItems;
};

#endif
