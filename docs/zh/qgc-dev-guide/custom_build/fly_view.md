# Fly View Customization

The Fly View is designed in such a way that it can be customized in multiple ways from simple to more complex. It is designed in three separate layers each of which are customizable providing different levels of change.

## Layers

- There are three layers to the fly view from top to bottom visually:
  - [`FlyView.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlyView/FlyView.qml) This is the base layer of UI and business logic to control map and video switching.
  - [`FlyViewWidgetLayer.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlyView/FlyViewWidgetLayer.qml) This layer includes all the remaining widgets for the fly view.
  - [`FlyViewCustomLayer.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlyView/FlyViewCustomLayer.qml) This is a layer you override using resource override to add your own custom layer.

### Positioning with Occluders

An important aspect of the Fly View is that it needs to know which parts of the map are covered by UI widgets. It uses this information to pan the map when the vehicle goes out of view or flies under a widget.

This is done through occluders. An occluder is the rectangle of a widget which covers the map, in the coordinates of the widget layer. The widget layer exposes a [`FlyViewOccluders`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlyView/FlyViewOccluders.qml) object through its `occluders` property with one named rectangle for each upstream widget: `pipView`, `toolStrip`, `topRight`, `bottomRight`, `mapScale`, `virtualJoystickLeft` and `virtualJoystickRight`. When a widget is hidden its rectangle collapses to zero size at the corner it is anchored to, so its position can still be used to place your own controls.

The custom layer is given these through its `occluders` property. You use them to position your controls around the upstream widgets, which means your custom layer needs to know how the upstream Fly View is laid out. You then report the rectangles of your own controls through the `customOccluders` property so the map also keeps the vehicle out from under them. The map recenters the vehicle on the center of the view, so keep your controls toward the edges and never over the center. If you override `FlyViewWidgetLayer.qml` your override must take the `pipViewRect` input and provide the same `occluders` property, setting every occluder and collapsing hidden widgets to their anchor corner. To see the occluders, set the `FlyViewOccluderViewer` in `FlyView.qml` to visible. The best way to understand this is to look at both the upstream and custom example code.

### `FlyView.qml`

The base layer for the view is also the most complex from UI interactions and business logic. 它包括地图和录像的主要显示元素以及有导向的控制。 Although you can resource override this layer it is not recommended. And if you do you better really (really) know what you are doing. The reason it is a separate layer is to make the layer above much simpler and easier to customize.

### `FlyViewWidgetLayer.qml`

This layer contains all the remaining controls of the fly view. You have the ability to hide the controls through use of [`QGCFlyViewOptions`](https://github.com/mavlink/qgroundcontrol/blob/master/src/API/QGCOptions.h). But in order to change the layout of the upstream controls you must use a resource override. If you look at the source you'll see that the controls themselves are well encapsulated such that it should not be that difficult to create your own override which repositions them and/or adds your own UI. While maintaining a connection to the upstream implementations of the controls.

### `FlyViewCustomLayer.qml`

This provides the simplest customization ability to the Fly View. Allowing you to add UI elements which are additive to the existing upstream controls. The upstream code adds no UI elements and is meant to be the basis for your own custom code used as a resource override for this QML. The custom example code provides you with an example of how to do it.

## Recommendations

### Simple customization

The best place to start is using a custom layer override plus turning off UI elements from the widgets layer (if needed). 如果可能的话，我建议尽量只使用这个。 It provides the greatest ability to not get screwed by upstream changes in the layers below.

### Moderate complexity customization

If you really need to reposition upstream UI elements then your only choice is overriding `FlyViewWidgetLayer.qml`. By doing this you are distancing yourself a bit from upstream changes. Although you will still get changes in the upstream controls for free. If there is a whole new control added to the fly view upstream you won't get it until you add it to your own override.

### Highly complex customization

The last and least recommended customization mechanism is overriding `FlyView.qml`. By doing this you are distancing yourself even further from getting upstream changes for free.
