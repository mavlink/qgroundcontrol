#pragma once

#include "KMLDomDocument.h"

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Used to convert a Plan to a KML document
class KMLPlanDomDocument : public KMLDomDocument
{

public:
    KMLPlanDomDocument();

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    static constexpr const char* surveyPolygonStyleName =   "SurveyPolygonStyle";

private:
    void _addStyles         (void);
    void _addFlightPath     (Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _addComplexItems   (QmlObjectListModel* visualItems);

    static constexpr const char* _missionLineStyleName =     "MissionLineStyle";
};
