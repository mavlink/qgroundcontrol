#!/bin/sh -e
# QT-CI Project
# License: Apache-2.0
# https://github.com/benlau/qtci

if [ $# -lt 2 ];
then
    echo extract-qt-installer qt-installer output_path
    exit -1
fi

INSTALLER=$1
OUTPUT=$2
SCRIPT="$(mktemp /tmp/tmp.XXXXXXXXX)"
PACKAGES=$QT_CI_PACKAGES

cat <<EOF > $SCRIPT
function Controller() {
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    });

    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("installationErrorWithRetry", QMessageBox.Ignore);

}

Controller.prototype.WelcomePageCallback = function() {
    console.log("Welcome Page");
    gui.clickButton(buttons.NextButton, 3000);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    console.log("Select components");

    function trim(str) {
        return str.replace(/^ +/,"").replace(/ *$/,"");
    }

    var widget = gui.currentPageWidget();

    var packages = trim("$PACKAGES").split(",");
    if (packages.length > 0 && packages[0] !== "") {
        widget.deselectAll();
        for (var i in packages) {
            var pkg = trim(packages[i]);
            console.log("Select " + pkg);
            widget.selectComponent(pkg);
        }
    } else {
       console.log("Use default component list");
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    console.log("Introduction Page");
    console.log("Retrieving meta information from remote repository");
    gui.clickButton(buttons.NextButton);
}


Controller.prototype.TargetDirectoryPageCallback = function() {
    console.log("Set target installation page: $OUTPUT");
    var widget = gui.currentPageWidget();

    if (widget != null) {
        widget.TargetDirectoryLineEdit.setText("$OUTPUT");
    }

    gui.clickButton(buttons.NextButton);

}

Controller.prototype.LicenseAgreementPageCallback = function() {
    console.log("Accept license agreement");
    var widget = gui.currentPageWidget();

    if (widget != null) {
        widget.AcceptLicenseRadioButton.setChecked(true);
    }

    gui.clickButton(buttons.NextButton);

}

Controller.prototype.ReadyForInstallationPageCallback = function() {
    console.log("Ready to install");
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.FinishedPageCallback = function() {
    var widget = gui.currentPageWidget();

    if (widget.LaunchQtCreatorCheckBoxForm) {
        // No this form for minimal platform
        widget.LaunchQtCreatorCheckBoxForm.launchQtCreatorCheckBox.setChecked(false);
    }
    gui.clickButton(buttons.FinishButton);
}
EOF

chmod u+x $INSTALLER
QT_QPA_PLATFORM=minimal $INSTALLER -v --script $SCRIPT
