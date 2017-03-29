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

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(MixersManagerLog)

class MixersManager : public QObject
{
    Q_OBJECT
    
public:
    MixersManager(Vehicle* vehicle);
    ~MixersManager();

    typedef enum {
        MIXERS_MANAGER_WAITING = 0,
        MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS,
        MIXERS_MANAGER_DOWNLOADING_ALL,
        MIXERS_MANAGER_DOWNLOADING_MISSING,
    } MIXERS_MANAGER_STATUS_e;

    /// true: Mixer data is ready for use
    Q_PROPERTY(bool mixerDataReady READ mixerDataReady NOTIFY mixerDataReadyChanged)
    Q_PROPERTY(MIXERS_MANAGER_STATUS_e mixerManagerStatus READ mixerManagerStatus NOTIFY mixerManagerStatusChanged)
    Q_PROPERTY(MixerGroup* mixerGroupStatus READ mixerGroupStatus NOTIFY mixerGroupStatusChanged)

    bool mixerDataReady(void);
    MixerGroup* mixerGroupStatus(void);

    MIXERS_MANAGER_STATUS_e mixerManagerStatus(void) {return _status;}

    ///* Search for all supported groups and download missing groups*/
    bool searchAllMixerGroupsAndDownload(void);

    MixerGroup* getMixerGroup(unsigned int groupID);
    MixerGroups* getMixerGroups(void) {return &_mixerGroupsData;}

    // These values are public so the unit test can set appropriate signal wait times
    static const int _ackTimeoutMilliseconds = 1000;
    static const int _maxRetryCount = 5;
    
signals:
    void mixerDataReadyChanged(bool mixerDataReady);
    void missingMixerDataChanged(bool missingMixerData);
    void mixerManagerStatusChanged(MIXERS_MANAGER_STATUS_e status);
    void mixerGroupStatusChanged(MixerGroup *mixerGroup);

protected:
    void _paramValueUpdated(const QVariant& value);
    
private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _msgTimeout(void);

private:
    typedef enum {
        AckNone,            ///< State machine is idle
        AckGetParameter,    ///< MIXER_DATA parameter value message expected
        AckSetParameter,    ///< MIXER_DATA parameter value message expected
        AckAll,             ///< ALL mixer data expected
    } AckType_t;


private:
    Vehicle*            _vehicle;
    LinkInterface*      _dedicatedLink;

    MixerGroups         _mixerGroupsData;

    QList<mavlink_mixer_param_value_t*> _mixerParameterMessages;
    QTimer*             _ackTimeoutTimer;
    AckType_t           _expectedAck;
    int                 _retryCount;

    MIXERS_MANAGER_STATUS_e _status;

    unsigned int        _actionGroup;       // The group which MixerManager is working with
    bool                _groupsIndentified; // The mixer groups have been checked and added to _mixerGroupsData

    void _startAckTimeout(AckType_t ack);

    void _setStatus(MIXERS_MANAGER_STATUS_e newStatus);

    void _mixerDataDownloadComplete(unsigned int group);

    bool _requestMixerAll(unsigned int group);

    bool _requestParameter(unsigned int group, unsigned int index);

    ///* Check for support on each possible mixer group on an AP
    /// When a supported group is found a MixerGroup is created for it*/
    bool _searchSupportedMixerGroup(unsigned int group);

    bool _searchMixerGroup();
    bool _searchNextMixerGroup();
    bool _downloadMixerGroup();
    bool _downloadNextMixerGroup();

    ///* Create a mixer group and add it to the mixer groups if the group is missing
    /// return pointer to the existing group or the new one*/
    MixerGroup* _createMixerGroup(unsigned int group);

    ///* Collect mixer data into a list.  Only one list entry per group, data_type, mixer, submixer etc...
    /// The test against data is dependent on the mixer data type*/
    bool _collectMixerData(const mavlink_mixer_param_value_t* param);

    ///* clear all of the recieved mixer messages for a given group */
    void _clearMixerGroupMessages(unsigned int group);

    ///* request to download data on a particular mixer group*/
    bool _requestMixerDownload(unsigned int group);

    ///* Request a missing message. true if there is missing data
    /// call to mixerDataDownloadComplete if no data is missing*/
    bool _requestMissingData(unsigned int group);

//    ///* Build mixer Fact database from the messages collected*/
//    bool _buildFactsFromMessages(unsigned int group);

    ///* Build all mixer data structure and content from whatever source is available
    /// return true if successfull*/
    bool _buildAll(unsigned int group);

    ///* Build mixer structure from messages.  This only includes mixers and submixers with type facts
    /// return true if successfull*/
    bool _buildStructureFromMessages(unsigned int group);


//    bool _checkForExpectedAck(AckType_t receivedAck);

//    bool        _readTransactionInProgress;
//    bool        _writeTransactionInProgress;
//    QList<int>  _itemIndicesToRead;     ///< List of mission items which still need to be requested from vehicle
    
//    QMutex _dataMutex;
    
//    QList<MissionItem*> _missionItems;
};

#endif
