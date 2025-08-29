Summary for Claude Code: AM32 ESC Configuration Integration for QGroundControl

Project Overview:
Adding AM32 ESC configuration capabilities to QGroundControl (QGC) to allow users to read/write AM32 ESC EEPROM settings via DShot programming mode through the flight controller, eliminating the need for separate configuration tools.

Current Task:

There is a race condition right now between initializing the sliders and checkboxes and the reception of eeprom data from the hardware. The way the flow should work is this: AM32 ESC page is selected, if there are ESCs already instantiated in the Vehicle class we create the ESC boxes (that are in a row that we can select) and emit the request for ESC info. If there is no ESCs then we should never have shown the AM32 ESC selection in the toolbar anyway. We wait until the ESC info has been received from all of the ESCs or until a timeout elapses (3 seconds). We then initialize all of the sliders and checkboxes. Please update the implementation.
