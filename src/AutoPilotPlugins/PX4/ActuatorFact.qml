import QtQuick 2.12
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

Loader {
	property var fact
	id:                     loader

	Component {
		id: factComboBox
		FactComboBox {
			fact:           loader.fact
			indexModel:     false
			sizeToContents: true
		}
	}
	Component {
		id: factCheckbox
		FactCheckBox {
			fact:           loader.fact
		}
	}

	Component {
		id: factTextField
		FactTextField {
			fact:           loader.fact
			width:          ScreenTools.defaultFontPixelWidth * 8
			showUnits:      false
		}
	}
	Component {
		id: factReadOnly
		QGCLabel {
			text:           loader.fact.valueString
		}
	}
	Component {
		id: notAvailable
		QGCLabel {
			text:           qsTr("(Param not available)")
		}
	}
	sourceComponent: fact ? 
		(fact.enumStrings.length > 0 ? factComboBox : 
			(fact.readOnly ? factReadOnly : (fact.typeIsBool ? factCheckbox : factTextField))
		) : notAvailable
}

