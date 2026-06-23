import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

QGCComboBox {
    property Fact fact: Fact { }
    property bool indexModel: fact ? fact.enumValues.length === 0 : true // true: Fact values are indices, false: Fact values are FactMetadata.enumValues

    model: fact ? fact.enumStrings : null

    currentIndex: fact ? (indexModel ? fact.value : fact.enumIndex) : 0

    function _updateCurrentIndex() {
        Qt.callLater(function() {
            currentIndex = Qt.binding(function() {
                return fact ? (indexModel ? fact.value : fact.enumIndex) : 0
            })
        })
    }

    onModelChanged: {
        // When the model changes, the index gets reset to 0, so re-establish
        // the declarative currentIndex binding. callLater() avoids a binding
        // loop since enumIndex could trigger a model change.
        _updateCurrentIndex()
    }

    onFactChanged: {
        // When the fact changes to a different Fact object with the same
        // enumStrings, modelChanged does not fire, so the binding must be
        // re-established here to point at the new Fact.
        _updateCurrentIndex()
    }

    onActivated: (index) => {
        if (indexModel) {
            fact.value = index
        } else {
            fact.value = fact.enumValues[index]
        }
    }
}
