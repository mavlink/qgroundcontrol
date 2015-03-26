import QtQuick 2.3
import QtLocation 5.3

Item {

Plugin {
id: mapPlugin
name: "osm"
}

    Map {
        plugin: mapPlugin
        width: parent.width
        height: parent.height
    }

}