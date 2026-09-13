#include "SVBackend.h"

#include "DigiviewManager.h"

SVBackend::SVBackend(QObject* parent)
    : QObject(parent)
{
}

DigiviewManager* SVBackend::digiview() const
{
    return DigiviewManager::instance();
}
