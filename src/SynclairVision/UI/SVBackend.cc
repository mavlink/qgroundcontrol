#include "SVBackend.h"

SVBackend::SVBackend(DigiviewManager* digiview, QObject* parent)
    : QObject(parent)
    , _digiview(digiview)
{
}
