import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    id: combo

    property real   pointSize:      ScreenTools.defaultFontPointSize    ///< Point size for button text
    property bool   centeredLabel:  false
    property alias  model:          popupItems.model
    property alias  textRole:       popup.textRole
    property alias  currentIndex:   popup.__selectedIndex

    readonly property alias count:          popupItems.count
    readonly property alias currentText:    popup.currentText

    property bool   _showBorder:        _qgcPal.globalTheme === QGCPalette.Light
    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property int    _horizontalPadding: ScreenTools.defaultFontPixelWidth
    property int    _verticalPadding:   Math.round(ScreenTools.defaultFontPixelHeight / 2)
    property real   _dropImageWidth:    ScreenTools.defaultFontPixelHeight / 2
    property real   _dropImageMargin:   _dropImageWidth / 2
    property var    __popup:            popup

    signal activated(int index)

    style: ButtonStyle {
        /*! The padding between the background and the label components. */
        padding {
            top:    _verticalPadding
            bottom: _verticalPadding
            left:   _horizontalPadding
            right:  _horizontalPadding
        }

        /*! This defines the background of the button. */
        background: Rectangle {
            implicitWidth:  ScreenTools.implicitComboBoxWidth
            implicitHeight: ScreenTools.implicitComboBoxHeight
            color:          control._qgcPal.textField
            border.width:   enabled ? 1 : 0
            border.color:   "#999"

            QGCColoredImage {
                id:                     image
                width:                  _dropImageWidth
                height:                 _dropImageWidth
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin:    _dropImageMargin
                anchors.right:          parent.right
                source:                 "/qmlimages/arrow-down.png"
                color:                  control._qgcPal.textFieldText
            }
        }

        /*! This defines the label of the button.  */
        label: Item {
            implicitWidth:  text.implicitWidth + _dropImageWidth
            implicitHeight: text.implicitHeight
            baselineOffset: text.y + text.baselineOffset

            QGCLabel {
                id:                         text
                anchors.verticalCenter:     parent.verticalCenter
                anchors.horizontalCenter:   centeredLabel ? parent.horizontalCenter : undefined
                text:                       control.currentText
                color:                      control._qgcPal.textFieldText
                font.pointSize:             pointSize
            }
        }
    }

    onClicked: {
        combo.focus = true
        popup.toggleShow()
    }

    Component.onCompleted: {
        if (currentIndex === -1) {
            currentIndex = 0
        }
        popup.resolveTextValue(textRole)
    }

    function textAt (index) {
        if (index >= count || index < 0)
            return null;
        return popupItems.objectAt(index).text;
    }

    function find (text) {
        for (var i = 0 ; i < popupItems.count ; ++i) {
            var currentString = popupItems.objectAt(i).text
            if (text === currentString) {
                return i
            }
        }
        return -1
    }

    ExclusiveGroup { id: eg }

    Menu {
        id:             popup
        __minimumWidth: combo.width
        __visualItem:   combo

        style: MenuStyle {
            font.pointSize:             ScreenTools.defaultFontPointSize
            font.family:                ScreenTools.normalFontFamily
            __labelColor:               combo._qgcPal.buttonText
            __selectedLabelColor:       combo._qgcPal.buttonHighlightText
            __selectedBackgroundColor:  combo._qgcPal.buttonHighlight
            __backgroundColor:          combo._qgcPal.button
            __maxPopupHeight:           600
            __menuItemType:             "comboboxitem"
            __scrollerStyle:            ScrollViewStyle { }
        }

        property string textRole: ""
        property bool   showing: false

        property string currentText:    selectedText
        property string selectedText

        onSelectedTextChanged: popup.currentText = selectedText

        on__SelectedIndexChanged: {
            if (__selectedIndex === -1)
                popup.currentText = ""
            else
                updateSelectedText()
        }

        property int    _y:             combo.height
        property bool   _modelIsArray:  false

        onAboutToShow: showing = true
        onAboutToHide: showing = false

        function toggleShow() {
            if (popup._popupVisible) {
                popup.__dismissAndDestroy()
            } else {
                __popup(Qt.rect(0, _y, 0, 0), 0)
            }
        }

        function resolveTextValue(initialTextRole) {
            if (!model) {
                return
            }

            var get = model['get'];
            if (!get && popup._modelIsArray && !!model[0]) {
                if (model[0].constructor !== String && model[0].constructor !== Number)
                    get = function(i) { return model[i]; }
            }

            var modelMayHaveRoles = get !== undefined
            textRole = initialTextRole
            if (textRole === "" && modelMayHaveRoles && get(0)) {
                // No text role set, check whether model has a suitable role
                // If 'text' is found, or there's only one role, pick that.
                var listElement = get(0)
                var roleName = ""
                var roleCount = 0
                for (var role in listElement) {
                    if (listElement[role].constructor === Function)
                        continue;
                    if (role === "text") {
                        roleName = role
                        break
                    } else if (!roleName) {
                        roleName = role
                    }
                    ++roleCount
                }
                if (roleCount > 1 && roleName !== "text") {
                    console.warn("No suitable 'textRole' found for ComboBox.")
                } else {
                    textRole = roleName
                }
            }
            updateSelectedText()
        }

        function updateSelectedText() {
            var selectedItem
            if (__selectedIndex !== -1 && (selectedItem = items[__selectedIndex])) {
                selectedText = Qt.binding(function () { return selectedItem.text })
                if (currentText !== selectedText) // __selectedIndex went form -1 to 0
                    selectedTextChanged()
            }
        }

        Instantiator {
            id: popupItems

            onModelChanged: {
                popup._modelIsArray = !!model ? model.constructor === Array : false
                popup.resolveTextValue(popup.textRole)
            }

            onObjectAdded: {
                // There is a bug in Instantiator which can cause objects to be added out of order from an index standpoint.
                // If not handled correctly this will cause menu items to be added incorrectly due to the way Menu.insertItem works.
                //console.log("menu add", index, object.text)
                if (index === popup.__selectedIndex) {
                    popup.selectedText = object["text"]
                }

                // Find the correct place for menu item. We can't just add at index, due to possible bad ordering
                var insertIndex = -1
                for (var i=0; i<popup.items.length;  i++) {
                    //console.log("position search", i, popup.items[i].itemIndex)
                    if (popup.items[i].itemIndex > index) {
                        insertIndex = i
                        break
                    }
                }
                if (insertIndex === -1) {
                    popup.insertItem(popup.items.length, object)
                } else {
                    //console.log("out of order menu add", index, insertIndex)
                    popup.insertItem(insertIndex, object)
                }
            }

            onObjectRemoved: popup.removeItem(object)

            MenuItem {
                text:           popup.textRole === '' ? modelData : ((popup._modelIsArray ? modelData[popup.textRole] : model[popup.textRole]) || '')
                checked:        index == currentIndex
                checkable:      true
                exclusiveGroup: eg

                property int itemIndex: index

                onTriggered: {
                    //console.log("onTriggered", index, currentIndex)
                    if (index !== currentIndex) {
                        //console.log("activated", index)
                        activated(index)
                    }
                }
            }
        }
    }
}
