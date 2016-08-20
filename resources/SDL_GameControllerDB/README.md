##SDL_GameControllerDB

[![Build Status](https://travis-ci.org/gabomdq/SDL_GameControllerDB.svg?branch=master)](https://travis-ci.org/gabomdq/SDL_GameControllerDB)

A community source database of game controller mappings to be used with SDL2 Game Controller functionality.

####Usage:

Download gamecontrollerdb.txt, place it in your app's directory and load with:

```
SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
```

####Creating new mappings:

To create new mappings, you can use the controllermap utility provided with
SDL2, or using Steam's Big Picture mode, configure your joystick and then 
look in config/config.vdf in your Steam installation directory for the 
SDL_GamepadBind entry.

####Checking your mappings:
You need to have python3 installed. Run

```
python3 check.py gamecontrollerdb.txt
```

####References:

* [SDL2](http://www.libsdl.org)
* [SDL_GameControllerAddMappingsFromFile](http://wiki.libsdl.org/SDL_GameControllerAddMappingsFromFile)
