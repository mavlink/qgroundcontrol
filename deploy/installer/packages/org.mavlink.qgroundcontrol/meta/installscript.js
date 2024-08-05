function Component()
{

}

Component.prototype.createOperations = function()
{
    try {
        component.createOperations();
    } catch (e) {
        console.log(e);
    }

    if (systemInfo.productType === "windows") {
        component.addElevatedOperation("Execute", "msiexec", ["/i", "@TargetDir@/driver.msi", "/qn"]);

        component.addOperation("CreateShortcut", "@TargetDir@/bin/qgroundcontrol.exe", "@StartMenuDir@/QGroundControl.lnk");
        component.addOperation("CreateShortcut", "@TargetDir@/bin/qgroundcontrol.exe", "@DesktopDir@/QGroundControl.lnk");
    }
}
