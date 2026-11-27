#include "QtLocationMapId.h"

#include "MapProvider.h"
#include "MapProviderRegistry.h"

namespace QtLocationMapId {

const QList<int>& mapTypeProviderIds()
{
    static const QList<int> providerIds = [] {
        QList<int> ids;
        for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
            // Not a map to show: QtLocation would request map tiles from an elevation provider
            if (!provider->isElevationProvider()) {
                ids.append(provider->mapId());
            }
        }
        return ids;
    }();
    return providerIds;
}

int toProviderId(int qtMapId, const QList<int>& providerIds)
{
    return ((qtMapId >= 1) && (qtMapId <= providerIds.size())) ? providerIds.at(qtMapId - 1) : -1;
}

int fromProviderId(int providerId, const QList<int>& providerIds)
{
    const qsizetype index = providerIds.indexOf(providerId);
    return (index >= 0) ? static_cast<int>(index + 1) : -1;
}

}  // namespace QtLocationMapId
