import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.Controls

QGCComboBox {
    property Fact fact: Fact { }
    property bool indexModel: fact ? fact.enumValues.length === 0 : true // true: Fact values are indices, false: Fact values are FactMetadata.enumValues

    model: fact ? fact.enumStrings : null

    currentIndex: fact ? (indexModel ? fact.value : fact.enumIndex) : 0

    onModelChanged: {
        // When the model changes, the index gets reset to 0, so make sure to
        // restore it correctly.
        // Since enumIndex could trigger a model change, we use callLater() to
        // avoid an event binding loop (the 2. call will for certain not trigger
        // another model change)
        Qt.callLater(function() {
            currentIndex = fact ? (indexModel ? fact.value : fact.enumIndex) : 0
        })
    }

    onActivated: (index) => {
        if (indexModel) {
            fact.value = index
        } else {
            fact.value = fact.enumValues[index]
        }
    }
}
