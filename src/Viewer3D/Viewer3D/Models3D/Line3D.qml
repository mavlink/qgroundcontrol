import QtQuick
import QtQuick3D

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Node{

    id: lineBody
    property vector3d p_1: Qt.vector3d(10, 0, 0)
    property vector3d p_2: Qt.vector3d(0, 20, 0)
    property real lineWidth: 5
    property alias color: line_mat.diffuseColor

    readonly property vector3d vec_1: Qt.vector3d(p_2.x - p_1.x,
                                                  p_2.y - p_1.y,
                                                  p_2.z - p_1.z)
    readonly property real length_: vecNorm(vec_1)
    readonly property vector3d vec_2: Qt.vector3d(0, length_, 0)

    function crossProduct(vec_a, vec_b)
    {
        var vec_c = Qt.vector3d(0, 0, 0)
        vec_c.x = vec_a.y * vec_b.z - vec_a.z * vec_b.y
        vec_c.y = -(vec_a.x * vec_b.z - vec_a.z * vec_b.x)
        vec_c.z = vec_a.x * vec_b.y - vec_a.y * vec_b.x

        return vec_c
    }

    function dotProduct(vec_a, vec_b)
    {
        return (vec_a.x*vec_b.x + vec_a.y*vec_b.y + vec_a.z*vec_b.z)
    }

    function vecNorm(_vec)
    {
        return Math.sqrt(dotProduct(_vec, _vec))
    }

    function normalizeVec(_vec)
    {
        var norm_vec = vecNorm(_vec)
        return Qt.vector3d(_vec.x/norm_vec, _vec.y/norm_vec, _vec.z/norm_vec)
    }

    function getRotationBetween(vec_a, vec_b)
    {
        var vec_a_n = normalizeVec(vec_a)
        var vec_b_n = normalizeVec(vec_b)

        var cos_angle = dotProduct(vec_a_n, vec_b_n)
        if(cos_angle === 1)
        {
            return Quaternion.fromEulerAngles(Qt.vector3d(0, 0, 0))
        }else if(cos_angle === -1)
        {
            var axis_idx = 0
            var dx = Math.abs(vec_a_n.x - vec_b_n.x)
            if(dx < Math.abs(vec_a_n.y - vec_b_n.y))
            {
                dx = Math.abs(vec_a_n.y - vec_b_n.y)
                axis_idx = 1
            }
            if(dx < Math.abs(vec_a_n.z - vec_b_n.z))
                axis_idx = 2

            switch(axis_idx)
            {
            case 0:
                return Quaternion.fromEulerAngles(Qt.vector3d(0, 180, 0))
            case 1:
                return Quaternion.fromEulerAngles(Qt.vector3d(0, 0, 180))
            case 2:
                return Quaternion.fromEulerAngles(Qt.vector3d(180, 0, 0))
            }
        }

        var angle_ = Math.acos(cos_angle)
        var axis_ = normalizeVec(crossProduct(vec_a_n, vec_b_n))

        return Quaternion.fromAxisAndAngle(axis_, angle_ * 180/3.1415)
    }

    rotation: getRotationBetween(vec_2, vec_1)
    position: p_1

    Model {
        readonly property real scalePose: 50
        readonly property real height: lineBody.length_
        readonly property real radius: lineBody.lineWidth * 0.1

        source: "#Cylinder"
        scale: Qt.vector3d(radius/scalePose, 0.5 * height/scalePose, radius/scalePose)
        position: Qt.vector3d(0, 0.5 * height, 0)

        materials:
            DefaultMaterial {
            id: line_mat
            diffuseColor: "blue"
        }
    }
}
