# SynclairVision: QGC

A custom fork of QGroundControl by SynclairVision

## Requirements
### Tested On:
- Ubuntu 24.04
- Qt 6.11.1


### You will need:
- Git
- CMake
- Ninja
- Qt 6.11.1
- GCC/G++
- Python 3
- LibUSB development package


### Install Dependencies:

```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    git \
    pkg-config \
    libusb-1.0-0-dev
```

Alternatively, use the repository's dependency helper:

```bash
python3 ./tools/setup/install_dependencies --platform debian

```



## Clone

Clone the repository

```bash
git clone git@github.com:SynclairVision/qgroundcontrol.git
cd qgroundcontrol
```


## Build

Configure the project:

```bash
python3 ./tools/configure.py \
    -B build \
    --release \
    --qt-root "$HOME/Qt/6.11.1/gcc_64"
```

Build it:

```bash
cmake --build build --config Release --parallel
```

## Run

Start SynclairQGC through the binary file 'SynclairQGC' in `./build/Release/`, ` ./SynclairQGC`

# Development

## Navigate the project and files

The custom SynclairQGC code exist mostly in the Flyview mode in QGC (with a few exceptions), which includes a map-view and a video-view that you can switch between. It is in the video-view, that the Synclair code exists (and is only active and displayed when the Synclair Overlay is active), the map-view stays unchanged. 

All of the custom QML elements and backend code lives in `/src/SynclairVision`. It is divided into two folders, **_Digiview_** and _**UI**_, self explanatory names. 

### Backend (_Digiview_)

The backend code functions to communicate with Digiview through Mavlink. There are two layers, _**DigiviewConnection**_ and _**DigiviewManager**_. 

_**DigiviewConnection**_ functions to handle the low level functions of the Mavlink communication, such as controlling host and ports, sending and receiving messages etc. Generally, this isn't code that is supposed to be changed unless a issue is found.

_**DigiviewManager**_ is what handles the actual messages being sent, using the SynclairVision Mavlink dialect. Here, there are helper functions for changing things through the QML code, aswell as the base functions for sending, requesting and receiving different dialect messages from Digiview. Many values for the code are stored here, like view and detection values among other. Much information, such as settings and current program states are stored in the qml code, but still received from here. When adding new Digiview interactions, like a new message or a new feature, this is the place to change it. Generally speaking, sending and receiving Digiview messages in the QML frontend should be done with helper functions for ease of use. 

For example of helper function, check out _changeEuler()_ in _**DigiviewManager**_.

### Frontend (_UI_)

Most of the custom Synclair QML code stems from `src/Flyview/Flyview.qml`. Here, the element **_SVFlyView.qml_** lives in the FlyViewVideo layer, in order to exist under the rest of the UI. This is the connection to the actual SynclairVision overlay. There are three types of layers in the Synclair code. 

The first is as background elements in Flyview. This is the stem of the Synclair code, aswell as including background elements such as visual borders for recording, AI detection, borders and seperating lines for camera-view elements. 

The UI elements, such as buttons, menues, notifications and settingsDrawer and the contolpanel lives in `SVFlyViewWidgetLayer.qml`. Exceptions to this is camera-view ui, the toggle for turning off/on the SynclairVision overlay and popups (SettingsMenu, editing Network Profiles and other popups). This layer has margin against the sides and the top toolbar. The menustrips onscreen live in `SVFlyViewMenus.qml`. There is a blueprint file named `SVFlyViewMenusList.js`, which includes names for buttons and icons etc, that the menu file reads from when creating the menustrips. The menustrips are created as a `SVMenuStrip.qml` element, getting the model from the blueprint file. Functionality from the buttons are controlled through "onClicked" within the created menustrip elements. Here is also where the Controlpanel lives, which holds the joystick and zoom-buttons. 

Because of the different layouts of camera-views, `SVFlyView.qml` camera layers are made here with `SVCameraLayer.qml`, sized and positioned relative to the different camera-views displayed in the Flyview Video. In the same place, the AI Detection ` SVFlyViewDetectionOverlay.qml` is already created. The camera-view layer has a similar structure to the SynclairVision Flyview layer, where it itself is split into a UI layer with margin and background elements. The UI layer has different versions of the compass for atitiude and pitch, with a custom function to make the pitch compass work in full rotation, aswell as a DetectionFlag currently used stop tracking for that camera-view. `SVCAmeraLayer` itself includes a overlays layer for guiding lines and crosshair, a visual overlay for not selectable when pinpointing tracking through STT or Cursor for one of the other camera-views. 

### Settings and States
There are two types of values stored in SynclairQGC. The first is settings-values controlled through the settingsmenu and and the other is state-values, such as which layout-view you are in, if the HUD is showing or not and other active states. Both of these files are singletons, which means that they can be accessed anywhere in the project. 

Settings are stored in `SVSettings.qml`, which also include functions for resetting SynclairVisionQGC settings when requested, network settings among others. These settings are controlled through the settingsmenu. The settingsmenu stems from `SVSettingsDrawer`, which creates a visual popup, a secondary settings menustrip on the side and some margins. Inside of that, the `SVSettingsMenu.qml` lives, which actually renders settings-segments, loading in new values for the settingsmenu, aswell as handling interaction with these. There are a few different types of buttons and sliders that can be used, created in this file. The settingsmenu gets the blueprints from `SVSettingsDefinitions.js`, where different settings are placed into different segments and categories. The _property_ value in these blueprints are what links to `SVSettings.qml`, and makes it possible to change settings. 

States are stored in ```bash SVState.qml```, which controls everything from the toolbar or HUD showing or not, if the user is actively recording, overlays and more. Useful features here are **_hud_** and **_synclairOverlay_**. 

### Resources

Many QGroundControl resources are used in this custom overlay, such as different types of buttons, labels, color-palette and mainly sizes and units. This makes the code consistant and visually similar to the rest of qgc, aswell as also easing development and new features. Outside of these original resources, SynclairQGC also includes its own Resources, such as `SVArrow.qml`, `SVBackground` (used for UI elements and background, includes visual elements and consistant with all SynclairQGC UI) and `SVTooltip.qml`, among others. 

An important file here is `SVUnits.qml`, which is a extension of the base units given in QGC. These are standardized, with nearly all UI using whole numbers of these values. This makes it so that all margins, corner-radius, text sizes among others stays consistant. 

In the Resource folder, all images and icons used for the overlay is stored. 

## Workflow

There are few standards worth noting when developing for SynclairQGC. 

- All files names start with "SV" in the beginning, making it easier to find the correct SynclairVision files.
- Keep files within the "SynclairVision" folder and in respective subfolders.
- All colors used (outside of standard colors like Black or White) should be taken from `QGCPalette.cc`. This is a singleton file, easily accessable anywhere. Using these colors makes Dark/Light mode, aswell as enabled/disabled colors for elements consistant, but most imporantly keeps the visual elements consistant in therms of color.
- Use resources like SVBackground, SVLine, etc, as much as possible. This eases development and keeps visual elements consistant.
- Design all elements to scale and interact correctly in both fullscreen, maximized window and sized window. Responsive design.
- Use standardized sizes given in `SVUnits.qml` for consistant sizes.

## Known issues

- Tracking is not correctly implemented and the code is due to be rewritten.
- Positing the controlpanel at the top through the settings locks the size of the zoombuttons.
- Issues with Jetson 60 (virtual lence?).
- Max-min values for Joystick and Zoom are unbalanced.
- Different sensitivity for Joystick and Zoom between clicking and using shortcuts.
- Welcome menu buttons/links not implemented.
- Checksum is wrong for videoOutput like layout and AI detection. Doesn't effect functions but appears as errors in Digiview.
- Possible to interact with some shortcuts outside of their intended area (example Joystick and Zoom in map view in Flyview-view).

## To-do:
- Tracking (STT, Cursor and Manual). Make it interact correctly with SynclairQGC UI.
- Checksum is wrong for 
- When AI detection is active for different camera-views the DetectionFlag should get marked with with an identifier for what detection button it is following. 



  
