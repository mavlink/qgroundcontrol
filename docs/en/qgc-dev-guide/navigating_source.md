# Navigating QGC Source Code

QGC is a large code base. With that it can be daunting to find what your are looking for in the source. Below are listed some tips to help you find what you are looking for.

## Start from the top of the UI

The top level window Qml UI code is found in `MainRootWindow.qml`. You can start here and work your way down through the UI hierarchy till you find what you are looking for.

In this qml file you'll find things like:

* How the toolbar is created
* How the top level views are created: Fly, Plan, ...

## Global Search

The best way to find something is to find the UI which is closest to what you are looking for by doing text searches.

### Example: Find the source for the GPS drop down in the toolbar

In the dropdown you see the words "Vehicle GPS Status". Do a global search for those words. Make sure you are doing a case sensitive search. Also make sure you are matching whole words. It can also be helpful to exclude the `*.ts` translation files. Since they can lead to multiple results. If you do the search described here it will take you right to `GPSIndicatorPage.qml`. Some times a search may lead to many results in both qml and .cpp files. In this case limit the results to `*.qml` files.
