# Font and Palette

QGC has a standard set of fonts and color palette which should be used by all user interface.

```qml
import QGroundControl.Controls
```

## QGCPalette

This item exposes the QGC color palette. There are two variants of this palette: light and dark. The light palette is meant for outdoor use and the dark palette is for indoor. Normally you should never specify a color directly for UI, you should always use one from the palette. If you don't follow this rule, the user interface you create will not be capable of changing from a light/dark style.

The core roles are layered so nested containers step up in brightness with a thin border around each level:

| Role | Use |
| --- | --- |
| `window` | Page background |
| `windowPanel` | First level container such as a sidebar or group box |
| `windowCard` | Container nested inside a panel, and button backgrounds |
| `windowInset` | Input wells such as text fields and combo boxes |
| `borderSubtle` | One pixel border around every container level, and dividers |
| `textPrimary` | Values and main text |
| `textSecondary` | Captions, descriptions, units and other supporting text |
| `accent` | Selection border, focus border, primary button fill |
| `accentSubtle` | Fill for a selected or checked item, paired with an `accent` border |
| `accentText` | Text drawn on an `accent` fill |

## QGCMapPalette

The map palette is used for colors which are used to draw over a map. Due to the the different map styles, specifically satellite and street you need to have different colors to draw over them legibly. Satellite maps needs lighter colors to be seen whereas street maps need darker colors to be seen. The `QGCMapPalette` item provides a set of colors for this as well as the ability to switch between light and dark colors over maps.

## ScreenTools

The ScreenTools item provides values which you can use to specify font sizing. It also provides information on screen size and whether QGC is running on a mobile device.
