function Controller() {
    installer.unstableComponentFound.connect(unstableComponentFound)
}

unstableComponentFound = function(type, message, comp)
{
    console.log("Unstable component, type: " + type)
    console.log("Unstable component, message: " + message)
    console.log("Unstable component, name: " + comp)
}