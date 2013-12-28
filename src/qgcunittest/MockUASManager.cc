#include "MockUASManager.h"

MockUASManager::MockUASManager(void) :
    _mockUAS(NULL)
{
    
}

UASInterface* MockUASManager::getActiveUAS(void)
{
    return _mockUAS;
}

void MockUASManager::setMockActiveUAS(MockUAS* mockUAS)
{
    // Signal components that the last UAS is no longer active.
    if (_mockUAS != NULL) {
        emit activeUASStatusChanged(_mockUAS, false);
        emit activeUASStatusChanged(_mockUAS->getUASID(), false);
    }
    _mockUAS = mockUAS;
    
    // And signal that a new UAS is.
    emit activeUASSet(_mockUAS);
    if (_mockUAS)
    {
        // We don't support swiching between different UAS
        //_mockUAS->setSelected();
        emit activeUASSet(_mockUAS->getUASID());
        emit activeUASStatusChanged(_mockUAS, true);
        emit activeUASStatusChanged(_mockUAS->getUASID(), true);
    }
}

