import QtQuick 2.12

Item {
    // `null` for default which makes the wizzard display one of the buttons: "Next" if more pages or "Done" if the last
    property string doneText: null
    // Blocks user from closing the wizard or going to the next page until this becomes true
    property bool forceConfirmation: false

    signal closeView()
}
