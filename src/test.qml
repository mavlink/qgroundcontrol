import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
//import QGroundControl.FactControls 1.0

Row {
width: 200
Text { id: firstCol; text: "Col 1" }
Text { horizontalAlignment: Text.AlignRight; width: parent.width - firstCol.contentWidth; text: "Col 2" }
}
