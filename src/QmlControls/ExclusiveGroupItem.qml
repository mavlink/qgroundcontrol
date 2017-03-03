import QtQuick                  2.7
import QtQuick.Controls         1.4


/// The ExclusiveGroupItem control can be used as a base class for a control which
/// needs support for ExclusiveGroup
Item {
    id: _root

    property bool checked: false
    property ExclusiveGroup exclusiveGroup: null

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }
}
