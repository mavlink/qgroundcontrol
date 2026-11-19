#pragma once

#include <QtCore/QList>

/// QtLocation expects the ids of a plugin's map types to start at 1 and be sequential. Provider ids
/// (MapProvider::getMapId()) have gaps: a build can lack providers, and the id of a removed provider is not reused.
/// So QtLocation's id of a map type is the 1-based position of its provider in the plugin's list of map types.
namespace QGeoMapIdQGC {

/// Ids of the providers the plugin has map types for, in the order of QtLocation's map ids
const QList<int>& mapTypeProviderIds();

/// \return The provider id of QtLocation's \a qtMapId, or -1 without such a map type
int toProviderId(int qtMapId, const QList<int>& providerIds = mapTypeProviderIds());

/// \return QtLocation's map id of the provider, or -1 for a provider without a map type
int fromProviderId(int providerId, const QList<int>& providerIds = mapTypeProviderIds());

}  // namespace QGeoMapIdQGC
