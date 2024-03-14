function Component()
{
    if (!installer.isCommandLineInstance())
        gui.pageWidgetByObjectName("LicenseAgreementPage").entered.connect(changeLicenseLabels);
}

changeLicenseLabels = function()
{
    page = gui.pageWidgetByObjectName("LicenseAgreementPage");
    page.AcceptLicenseLabel.setText("Yes I do!");
}

function abortInstaller()
{
    installer.setDefaultPageVisible(QInstaller.Introduction, false);
    installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
    installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);

    var abortText = "<font color='red' size=3>" + qsTr("Installation failed:") + "</font>";

    var error_list = installer.value("component_errors").split(";;;");
    abortText += "<ul>";
    // ignore the first empty one
    for (var i = 0; i < error_list.length; ++i) {
        if (error_list[i] !== "") {
            console.log(error_list[i]);
            abortText += "<li>" + error_list[i] + "</li>"
        }
    }
    abortText += "</ul>";
    installer.setValue("FinishedText", abortText);
}

function reactOnAbortInstallerChange()
{
    if (installer.value("ComponentError") === "true")
        abortInstaller();
}

function Component()
{
    installer.finishAllComponentsReset.connect(reactOnAbortInstallerChange);
}

function Component()
{
    var result = QMessageBox["question"]("test.quit", "Installer", "Do you want to quit the installer?<br>" +
        "This message box was created using JavaScript.", QMessageBox.Ok | QMessageBox.Cancel);
    if (result == QMessageBox.Ok) {
        abortInstaller();
        gui.clickButton(buttons.NextButton);
    } else {
        installer.setValue("FinishedText",
            "<font color='green' size=3>The installer was not quit by JavaScript.</font>");
    }
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/README.txt", "@StartMenuDir@/README.lnk",
            "workingDirectory=@TargetDir@", "iconPath=%SystemRoot%/system32/SHELL32.dll",
            "iconId=2", "description=Open README file");
    }
}

function Component()
{
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
}

function majorVersion(str)
{
    return parseInt(str.split(".", 1));
}

function Component()
{
    console.log("OS: " + systemInfo.productType);
    console.log("Kernel: " + systemInfo.kernelType + "/" + systemInfo.kernelVersion);

    var validOs = false;

    if (systemInfo.kernelType === "winnt") {
        if (majorVersion(systemInfo.kernelVersion) >= 6)
            validOs = true;
    } else if (systemInfo.kernelType === "darwin") {
        if (majorVersion(systemInfo.kernelVersion) >= 11)
            validOs = true;
    } else {
        if (systemInfo.productType !== "opensuse"
                || systemInfo.productVersion !== "13.2") {
            QMessageBox["warning"]("os.warning", "Installer",
                                   "Note that the binaries are only tested on OpenSUSE 13.2.",
                                   QMessageBox.Ok);
        }
        validOs = true;
    }

    if (!validOs) {
        cancelInstaller("Installation on " + systemInfo.prettyProductName + " is not supported");
        return;
    }

    console.log("CPU Architecture: " +  systemInfo.currentCpuArchitecture);

    installer.componentByName("root.i386").setValue("Virtual", "true");
    installer.componentByName("root.x86_64").setValue("Virtual", "true");

    if ( systemInfo.currentCpuArchitecture === "i386") {
        installer.componentByName("root.i386").setValue("Virtual", "false");
        installer.componentByName("root.i386").setValue("Default", "true");
    }
    if ( systemInfo.currentCpuArchitecture === "x86_64") {
        installer.componentByName("root.x86_64").setValue("Virtual", "false");
        installer.componentByName("root.x86_64").setValue("Default", "true");
    }
}

function Component()
{
    // constructor
    component.loaded.connect(this, Component.prototype.loaded);
    if (!installer.addWizardPage(component, "Page", QInstaller.TargetDirectory))
        console.log("Could not add the dynamic page.");
}

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}

Component.prototype.createOperations = function()
{
    try {
        // call the base create operations function
        component.createOperations();
    } catch (e) {
        console.log(e);
    }
}

Component.prototype.loaded = function ()
{
    var page = gui.pageByObjectName("DynamicPage");
    if (page != null) {
        console.log("Connecting the dynamic page entered signal.");
        page.entered.connect(Component.prototype.dynamicPageEntered);
    }
}

Component.prototype.dynamicPageEntered = function ()
{
    var pageWidget = gui.pageWidgetByObjectName("DynamicPage");
    if (pageWidget != null) {
        console.log("Setting the widgets label text.")
        pageWidget.m_pageLabel.text = "This is a dynamically created page.";
    }
}
