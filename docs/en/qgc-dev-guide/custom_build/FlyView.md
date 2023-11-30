# Fly View Customization

The Fly View is designed in such a way that it can be cusomtized in multiple ways from simple to more complex. It is designed in three separate layers each of which are customizable providing different levels of change.


## Layers

* There are three layers to the fly view from top to bottom visually:
  * [`FlyView.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlightDisplay/FlyView.qml) This is the base layer of ui and business logic to control map and video switching.
  * [`FlyViewWidgetsOverlay.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlightDisplay/FlyViewWidgetLayer.qml) This layer includes all the remaining widgets for the fly view.
  * [`FlyViewCustomLayer.qml`](https://github.com/mavlink/qgroundcontrol/blob/master/src/FlightDisplay/FlyViewCustomLayer.qml) This is a layer you override using resource override to add your own custom layer.

### Inset Negotiation using `QGCToolInsets`
An important aspect of the Fly View is that it needs to understand how much central space it has in the middle of it's map window which is not obstructed by ui widgets which are at the edges of the window. It uses this information to pan the map when the vehicle goes out of view. This need to be done not only for the window edges but also for the widgets themselve such that the map pans before it goes under a widget.

This is done through the use of the [`QGCToolInsets`](https://github.com/mavlink/qgroundcontrol/blob/master/src/QmlControls/QGCToolInsets.qml) object included in each layer. This objects provides inset information for each window edge informing the system as to how much real estate is taken up by edge based ui. Each layer is given the insets of the layer below it through `parentToolInsets` and then reports back the new insets taking into account the layer below and it's own additions through `toolInsets`. The final results total inset is then given to the map so it can do the right thing. The best way to understand this is to look at both the upstream and custom example code.

### `FlyView.qml`
The base layer for the view is also the most complex from ui interactions and business logic. It includes the main display elements of map and video as well as the guided controls. Although you can resource override this layer it is not recommended. And if you do you better really (really) know what you are doing. The reason it is a separate layer is to  make the layer above much simpler and easier to customize.

### `FlyViewWidgetsOverlay.qml`
This layer contains all the remaining controls of the fly view. You have the ability to hide the controls through use of [`QGCFlyViewOptions`](https://github.com/mavlink/qgroundcontrol/blob/master/src/api/QGCOptions.h). But in order to change the layout of the upstream controls you must use a resource override. If you look at the source you'll see that the controls themselves are well encapsulated such that it should not be that difficult to create your own override which repositions them and/or adds your own ui. While maintaining a connection to the upstream implementaions of the controls.

### `FlyViewCustomLayer.qml`
This provides the simplest customization ability to the Fly View. Allowing you the add ui elements which are additive to the existing upstream controls. The upstream code adds no ui elements and is meant to be the basis for your own custom code used as a resource override for this qml. The custom example code provides you with an example of how to do it. 

## Recommendations

### Simple customization
The best place to start is using a custom layer override plus turning off ui elements from the widgets layer (if needed). I would recommend trying to stick with only this if at all possible. It provides the greatest abilty to not get screwed by upstream changes in the layers below.

### Moderate complexity customization
If you really need to reposition upstream ui elements then your only choice is overriding `FlyViewWidgetsOverlay.qml`. By doing this you are distancing yourself a bit from upstream changes. Although you will still get changes in the upstream controls for free. If there is a whole new control added to the fly view upstream you won't get it until you add it to your own override.

### Highly complex customization
The last and least recommended customization mechanism is overriding `FlyView.qml`. By doing this you are distancing yourself even further from getting upstream changes for free.