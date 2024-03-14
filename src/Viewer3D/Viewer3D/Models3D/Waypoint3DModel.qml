import QtQuick3D
import QtQuick
import QtPositioning

import QGroundControl.Viewer3D
///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node{
    id:  waypointBody

    property var    missionItem
    property double altitudeBias: 0
    property double pose_y:  waypointBody.missionItem.y * 10
    property double pose_z: ( waypointBody.missionItem.z + altitudeBias) * 10
    property double pose_x:  waypointBody.missionItem.x * 10

    position{
        x:  waypointBody.pose_x
        y:  waypointBody.pose_y
        z:  waypointBody.pose_z
    }

    Node{
        eulerRotation{
            x:90
        }
        scale: Qt.vector3d(0.1, 0.1, 0.1)
        Model {
            id: nose
            source: "#Cone"
            materials: [ DefaultMaterial {
                    diffuseColor: {
                        let _abbreviation = abbreviation;
                        if(_abbreviation === "Takeoff"){
                            return "green";
                        }else if(abbreviation === "ROI"){
                            return "red"
                        }
                        return "black"
                    }
                }
            ]
        }
    }

    Node
    {
        Text {
            color: "black"
            text: {
                let _abbreviation = abbreviation;
                if(_abbreviation === "Takeoff"){
                    return "T";
                }else if(abbreviation === "ROI"){
                    return "R"
                }
                return ""
            }
            font.pixelSize: 20
        }

        eulerRotation{
            x:90
            y:0
            z:0
        }
        position{
            z: 30
            x: -6
        }
    }
}



