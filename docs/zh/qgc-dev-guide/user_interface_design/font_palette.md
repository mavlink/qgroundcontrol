# 字体和调色板

QGC有一套标准的字体和调色板，应该由所有用户界面使用。

```
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
```

## QGCPalette(QGC调色板)

此项目显示QGC调色板。 这个调色板有两种变体：浅色和深色。 较浅的调色板适合户外使用，黑色调色板适用于室内。 通常，您不应该直接为UI指定颜色，您应该使用调色板中的颜色。 如果不遵循此规则，则您创建的用户界面将无法通过浅色/深色样式更改。 There are two variants of this palette: light and dark. The light palette is meant for outdoor use and the dark palette is for indoor. Normally you should never specify a color directly for UI, you should always use one from the palette. If you don't follow this rule, the user interface you create will not be capable of changing from a light/dark style.

## QGCMapPalette(QGC地图调色板)

The map palette is used for colors which are used to draw over a map. 地图调色板用于绘制地图的颜色。 由于不同的地图样式，特别是卫星和街道，您需要有不同的颜色来清晰地绘制它们。 卫星地图需要更浅的颜色，而街道地图需要更深的颜色。 QGCMapPalette项目为此提供了一组颜色，以及在地图上切换浅色和深色的功能。 Satellite maps needs lighter colors to be seen whereas street maps need darker colors to be seen. The `QGCMapPalette` item provides a set of colors for this as well as the ability to switch between light and dark colors over maps.

## ScreenTools (屏幕工具)

The ScreenTools item provides values which you can use to specify font sizing. ScreenTools项提供可用于指定字体大小的值。 它还提供有关屏幕大小以及QGC是否在移动设备上运行的信息。
