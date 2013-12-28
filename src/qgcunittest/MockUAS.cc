#include "MockUAS.h"

QString MockUAS::_bogusStaticString;

MockUAS::MockUAS(void) :
    _systemType(MAV_TYPE_QUADROTOR),
    _systemId(1)
{
    
}

void MockUAS::setMockParametersAndSignal(MockQGCUASParamManager::ParamMap_t& map)
{
    _paramManager.setMockParameters(map);
    
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        emit parameterChanged(_systemId, 0, i.key(), i.value());
    }
}