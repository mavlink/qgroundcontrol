#include "StagedExclusionZone.h"

#include "QGCFencePolygon.h"

StagedExclusionZone::StagedExclusionZone(QGCFencePolygon* polygon, QObject* parent) : QObject(parent), _polygon(polygon)
{
    _polygon->setParent(this);
}

void StagedExclusionZone::setApproved(bool approved)
{
    if (_approved != approved) {
        _approved = approved;
        emit approvedChanged(_approved);
    }
}
