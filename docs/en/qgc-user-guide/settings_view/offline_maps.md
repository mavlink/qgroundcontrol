# Offline Maps

![](../../../assets/settings/offline_maps.jpg)

Offline Maps allows you to cache map tiles for use when not connected to the Internet. You can create multiple offline sets, each for a different location.

## Add new set

To create a new offline map set, click "Add new set". Which will take you to this page:
![](../../../assets/settings/offline_maps_add.jpg)

From here you can name your set as well as specify the zoom levels you want to cache. Move the map to the position you want to cache and then set the zoom levels and click Download to cache the tiles.

To the left you can see previews of the min and max zoom levels you have chosen.

## Managing downloads

Each offline tile set shows live download statistics (pending, active, and error tiles) so you can see whether work is still in progress. You can pause an in-flight download, resume it later, or retry only the tiles that previously failed. Pausing keeps your place in the queue, which is especially useful when you need to temporarily disable connectivity or suspend caching from the main Map Settings page.

## Default cache toggle

The **Default Cache** switch near the top of the Offline Maps page controls whether QGroundControl stores tiles that are fetched during normal map browsing. Leave it enabled if you rely on the automatic cache for day-to-day flying, or disable it to save disk space and rely exclusively on the offline tile sets you create manually. The toggle simply affects the default/system cache—the user-defined offline sets continue to work normally.
