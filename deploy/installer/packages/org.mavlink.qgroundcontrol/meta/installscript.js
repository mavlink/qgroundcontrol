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
        component.addOperation("CreateShortcut", "@TargetDir@/bin/qgroundcontrol.exe", "@StartMenuDir@/QGroundControl.lnk");
        component.addOperation("CreateShortcut", "@TargetDir@/bin/qgroundcontrol.exe", "@DesktopDir@/QGroundControl.lnk");
    }
}
