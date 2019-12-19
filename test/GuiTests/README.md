
# GUI Automated Tests 

This folder contains automated GUI tests for QGroundControl Ground Control Station.  
Definitions for the terms used in the the following document:
- **AUT** - Application Under Test.  
- **Test Environment** - the environment where **AUT** is running.
- **Dev Environment** - the environment used for test development

The automated GUI tests were created with [Squish GUI Tester](https://www.froglogic.com/squish/).


## Squish Configuration & Setup

### Configuration

A [compatible Squish package](https://doc.froglogic.com/squish/latest/ins-commandline-tools-server-ide.html#ins-binary-get) has to be used to launch the QGC application.

For the test case development, Squish package with Python 3.7 was used.

### Setup

During the initial test development, the Ubuntu Virtual Machine was used as the **Test Environment**

- **AUT** running on the **Test Environment**
- squishserver running on the same **Test Environment**
- SquishIDE running on the **Dev Environment**

### Configuration

In the case where VM is used as **Test Environment**, the [Remote]((https://doc.froglogic.com/squish/latest/rg-regressiontesting.html#rgr-disttesting)) testing and test development are recommended.

Tests created with loacl and remote approaches are compatible as long as the developer remembers about some basic rules like:
- Test scripts are always interpreted and executed on the squishrunner side
- Some applications and libraries may be vulnerable to file path formats.

#### Local configuration

- The AUT needs to be [registered](https://doc.froglogic.com/squish/latest/ide.dialogs.html#manage.auts.dialog).

- No additional configuration is required as both squishrunner and squishserver are running on the same machine.


#### Remote configuration

- **AUT** and squishserver running on **Test Environment**
- SquishIDE running on **Dev Environment**
- Both **Test Environment** and **Dev Environment** need to be accessible to each other(ping/telnet).
- squishserver access settings have bo be [configured](https://doc.froglogic.com/squish/latest/rg-regressiontesting.html#rgr-disttesting) in squishserverc
- The AUT needs to be [registered](https://doc.froglogic.com/squish/latest/ide.dialogs.html#manage.auts.dialog) on the Test Environment.
- SquishIDE has to be [configured](https://doc.froglogic.com/squish/latest/rg-regressiontesting.html#rgr-disttesting) for Remote Testing




