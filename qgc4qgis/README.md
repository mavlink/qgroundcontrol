# QGC4QGIS — QGroundControl Flight Planning Plugin for QGIS

> Portuguese version: [README.pt-BR.md](README.pt-BR.md)

**QGC4QGIS** is a QGIS *plugin* that integrates **QGroundControl (QGC)** photogrammetric flight planning features directly into the GIS environment. It lets you generate flight grids (*Survey Grids*), simulate photo centers and footprints, calculate mission statistics, and export flight plan files in QGroundControl's native `.plan` format.

---

## 1. Installation

### Requirements
- **QGIS**: version 3.34 to 4.x.
- **Python**: 3.9 or higher (included in QGIS standard distributions).

### Installation Methods

#### Method A: Direct Copy (Recommended for Development)
Copy or create a symbolic link of the `qgc4qgis` folder into your QGIS profile's plugins directory:

- **Linux**:
  ```bash
  mkdir -p ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins
  ln -s /caminho/para/qgroundcontrol-4qgis/qgc4qgis ~/.local/share/QGIS/QGIS3/profiles/default/python/plugins/qgc4qgis
  ```
- **Windows**:
  ```text
  %APPDATA%\QGIS\QGIS3\profiles\default\python\plugins\qgc4qgis
  ```
- **macOS**:
  ```bash
  ~/Library/Application Support/QGIS/QGIS3/profiles/default/python/plugins/qgc4qgis
  ```

#### Method B: Installation via ZIP File
1. Compress the `qgc4qgis` folder into a `.zip` file.
2. In QGIS, go to the **Plugins** menu > **Manage and Install Plugins...**
3. Select the **Install from ZIP** tab.
4. Select the `.zip` file you created and click **Install Plugin**.

### Activation
After copying or installing the file, open QGIS, go to the **Plugins** menu > **Manage and Install Plugins...**, find **QGC4QGIS** in the list, and check the box to enable it.

---

## 2. Five-Step Workflow

Photogrammetric flight mission planning through the QGC4QGIS dock widget (*Dock Widget*) follows a structured **five-step** workflow:

```text
[Step 1: Polygon] ➔ [Step 2: Camera] ➔ [Step 3: Altitude/GSD] ➔ [Step 4: Grid] ➔ [Step 5: Export]
```

1. **Coverage Polygon Selection**:
   - Select the polygon vector layer that defines the area of interest (AOI).
   - Choose the specific feature in the layer or use all features to delimit the survey perimeter.

2. **Camera Configuration**:
   - Choose a preconfigured camera model from the integrated camera library (e.g., Sony ILCE-7R, DJI cameras, etc.).
   - Alternatively, select *Custom Camera* (Manual Camera) to specify the sensor's physical properties: sensor width ($mm$), sensor height ($mm$), image width ($px$), image height ($px$), and focal length ($mm$).

3. **Flight Altitude or GSD Definition**:
   - Choose the main control parameter: **Flight Altitude (m)** or **GSD (cm/px)**.
   - When one of the values is changed, the plugin automatically calculates the corresponding value while keeping the camera's optical relationship.

4. **Grid and Overlap Adjustment**:
   - Set the **Side Overlap (%)** (*side overlap*) and the **Front Overlap (%)** (*front overlap*).
   - Adjust the **Grid Angle (degrees)** to orient the flight transects in the desired direction (e.g., aligned with the wind or with the largest dimension of the terrain).
   - Configure the **Turnaround Distance (m)** to extend the strips beyond the polygon, allowing the flight to stabilize before taking photos.
   - Set the **Entry Point** (*Top-Left*, *Top-Right*, *Bottom-Left*, *Bottom-Right*) and enable **Cross Grid (Refly 90°)** if you want a double orthogonal flight.

5. **Preview and Export (.plan)**:
   - Preview the transect grid, photo centers, and footprint polygons directly on the QGIS map in real time.
   - Check the calculated statistics: total area ($ha$), flight length ($km$), estimated number of photos, and flight time.
   - Click **Export QGC Plan (.plan)** to save the file ready to be imported into QGroundControl or loaded onto the drone.

---

## 3. Parameter Table

The table below describes all the parameters available in the plugin's processing tools (`gerar_grade_voo`, `gerar_centros_foto`, and `exportar_plano_qgc`):

| Parameter | Identifier | Type / Unit | Default Value | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Input Layer** | `INPUT` | Vector (Polygon / Line) | *Required* | Vector layer containing the coverage area geometry or transect lines. |
| **Camera** | `CAMERA` | Enum / Text | `0` (First in the list) | Camera selected from the predefined library or manual model (*Custom Camera*). |
| **Flight Altitude** | `ALTITUDE` | Numeric (`Double`) / meters ($m$) | `100.0` | Relative flight altitude above the takeoff point. |
| **GSD** | `GSD` | Numeric (`Double`) / $cm/px$ | `0.0` | Ground resolution. If $> 0$, it calculates and overrides the flight altitude. |
| **Side Overlap** | `OVERLAP_SIDE` | Numeric (`Double`) / percentage ($\%$) | `70.0` | Overlap percentage between adjacent flight strips. |
| **Front Overlap** | `OVERLAP_FRONTAL` | Numeric (`Double`) / percentage ($\%$) | `70.0` | Overlap percentage between consecutive images in the same strip. |
| **Grid Angle** | `ANGLE` | Numeric (`Double`) / degrees ($^\circ$) | `0.0` | Orientation of the flight transects relative to North ($-180^\circ$ to $180^\circ$). |
| **Turnaround** | `TURNAROUND` | Numeric (`Double`) / meters ($m$) | `0.0` | Extension of lines beyond the polygon for the aircraft's turn and acceleration maneuver. |
| **Entry Point** | `ENTRY_LOCATION` | Enum (`0`: Top-Left, `1`: Top-Right, `2`: Bottom-Left, `3`: Bottom-Right) | `0` | Starting corner for the flight grid execution. |
| **Cross Grid** | `REFLY` | Boolean (`True`/`False`) | `False` | If enabled, generates a second transect grid perpendicular ($90^\circ$) to the first. |
| **Sensor Width** | `SENSOR_WIDTH` | Numeric (`Double`) / $mm$ | `35.9` | Physical width of the photographic sensor (used in Manual Camera). |
| **Sensor Height** | `SENSOR_HEIGHT` | Numeric (`Double`) / $mm$ | `24.0` | Physical height of the photographic sensor (used in Manual Camera). |
| **Image Width** | `IMAGE_WIDTH` | Numeric (`Integer`) / $pixels$ | `7952` | Horizontal resolution of the captured image (used in Manual Camera). |
| **Image Height** | `IMAGE_HEIGHT` | Numeric (`Integer`) / $pixels$ | `5304` | Vertical resolution of the captured image (used in Manual Camera). |
| **Focal Length** | `FOCAL_LENGTH` | Numeric (`Double`) / $mm$ | `35.0` | Actual focal length of the camera lens (used in Manual Camera). |
| **Cruise Speed** | `CRUISE_SPEED` | Numeric (`Double`) / $m/s$ | `15.0` | Nominal horizontal speed of the aircraft in flight (used when exporting the `.plan`). |
| **Hover Speed** | `HOVER_SPEED` | Numeric (`Double`) / $m/s$ | `5.0` | Horizontal deceleration/hover speed for multicopters. |
| **Firmware** | `FIRMWARE_TYPE` | Enum (`12`: PX4, `3`: ArduPilot) | `12` (PX4) | Autopilot protocol and format for mission export. |
| **Vehicle Type** | `VEHICLE_TYPE` | Enum (`2`: Multicopter, `1`: Fixed Wing, `19`: VTOL) | `2` (Multicopter) | Category of the unmanned aerial vehicle. |

---

## 4. Limitations Inherited from QGroundControl

To maintain full compatibility with QGroundControl's original algorithm, QGC4QGIS inherits **two architectural limitations** from the QGC's `SurveyComplexItem` library.

### 1. Concave Polygon Without Decomposition (*Concave Polygon Decomposition*)
- **Description**: QGC's flight transect generator treats the coverage polygon as a single continuous outer ring (*outer ring*). When processing areas with concave geometries (such as "L", "C", or "U" shapes) or polygons containing holes (*donuts*), the grid is generated by sweeping the full extent of the bounding envelope (*bounding envelope*).
- **Consequence**: The algorithm does not perform automatic decomposition of the geometry into isolated convex subpolygons. As a result, some transects may cross regions outside the polygon between two concave indentations.

### 2. GSD Calculated Using Sensor Width Only (*Width-Only GSD Calculation*)
- **Description**: The conversion equation between Flight Altitude ($m$) and GSD ($cm/px$) in QGroundControl calculates ground resolution exclusively based on the sensor's horizontal dimension (`sensorWidth`) and the image width in pixels (`imageWidth`):

$$\text{GSD} = \frac{\text{Flight Altitude (m)} \times \text{Sensor Width (mm)}}{\text{Focal Length (mm)} \times \text{Image Width (px)}} \times 100$$

$$\text{Flight Altitude (m)} = \frac{\text{GSD (cm/px)} \times \text{Focal Length (mm)} \times \text{Image Width (px)}}{\text{Sensor Width (mm)} \times 100}$$

- **Consequence**: The sensor's vertical dimension (`sensorHeight`) and the image height in pixels (`imageHeight`) do not affect the scalar GSD calculation or the resulting flight altitude; they are used only to determine the extent of the footprints and the front photo trigger distance.

---

## 5. Export to Litchi and DJI Fly

In addition to QGroundControl's native `.plan` format, QGC4QGIS lets you export missions to the **Litchi** (`.csv` format) and **DJI Fly** (WPML `.kmz` format) applications.

### 1. Export and Loading in Litchi

When loading missions into Litchi, it is important to distinguish between the two Mission Hub interfaces:

| Interface | URL | Supported Mission Formats |
| :--- | :--- | :--- |
| **Classic Hub** | `flylitchi.com/hub` | `.csv`, `.kml`, `.wpml` |
| **Hub 2** | `hub.flylitchi.com` | `.csv`, `.kmz` (WPML) |

> [!WARNING]
> **Caution when importing into Hub 2 (`hub.flylitchi.com`):**  
> Hub 2 has **two** distinct import dialogs. The area/overlay dialog (`.kml`, `.kmz`, `.geojson`, `.json`) is only for drawing map layers and **never creates flight** waypoints. To load the flight route, always use the mission import dialog.

> [!NOTE]
> **`.kmz` file compatibility:**  
> The same `.kmz` file exported for **DJI Fly** is imported by **Hub 2** as a mission, because Hub 2 detects the `template.kml` + `waylines.wpml` files inside the compressed file. The **classic hub does not read `.kmz` files**.

1. **Loading Steps**:
   - Access the **Litchi Mission Hub** (`flylitchi.com/hub` or `hub.flylitchi.com`) or open the Litchi mobile app.
   - Access the mission import option (**Mission → Import**) and select the `.csv` file (or `.kmz` on Hub 2) exported by the plugin.
   - Click **Save** to sync the imported mission with your account and devices.
2. **Three Manual Global Adjustments Required (D8 Format)**:
   When importing a CSV mission in the Litchi Mission Hub, three global parameters need to be set manually in the panel before saving:
   - **Heading Mode**: choose the heading behavior. With **"Custom (WP)"**, the application uses the `heading(deg)` recorded in the CSV (the azimuth of each transect).
   - **Finish Action**: define the action executed at the end of the route (e.g., *Return to Home (RTH)*, *None*, or *Land*).
   - **Path Mode**: check **"Straight Lines"** — the CSV is generated with `curvesize=0` and does not represent curves.

### 1.b Loading via KML on the Classic Hub (with a per-waypoint photo action)

1. **Loading Steps**:
   - Access the **Classic Litchi Mission Hub** (`flylitchi.com/hub`).
   - Access the import option (**Mission → Import**) and select the `.kml` file.
   - **Check the "Add take photo action" option** and **leave "Placemarks as POI" unchecked**.
   - This setting automatically inserts a *Take Photo* action at **all** waypoints and forces the path mode to straight lines (*Straight Lines*).

2. **Trigger Mode Requirement**:
   - Using the `.kml` format only makes sense when the mission is generated with **Trigger Mode = "By photo"**. Otherwise, the KML vertices will only be the endpoints (start and end) of the transects.
   - This same option (**Trigger Mode = "By photo"**) also resolves the waypoint sampling for the `.csv` format, which is the recommended option since it also carries heading, gimbal, speed, and curve.

3. **Parameters Missing from KML and Adjustments in the Mission Hub**:
   Since the KML format contains only geographic coordinates and altitudes, the remaining parameters are not imported and must be adjusted manually in the Mission Hub:

   | Parameter Missing from KML | Default Behavior in the Classic Hub | Where to Adjust in the Mission Hub |
   | :--- | :--- | :--- |
   | **Heading** | Direction to the next waypoint (or North) | Mission Settings Panel (`Heading Mode`) |
   | **Gimbal Pitch** | $0^\circ$ (horizontal) | Waypoint Properties or Actions |
   | **Speed** | Global default mission speed | Mission Settings Panel (`Speed`) |
   | **Curve (Cruising/Curved)** | *Straight Lines* (forced by *Take Photo*) | Mission Settings Panel (`Path Mode`) |
   | **Points of Interest (POI)** | Not imported (uncheck "Placemarks as POI") | Add manually on the map |
   | **Explicit Altitude Mode** | Relative to the ground at the takeoff point (*Relative*) | Individual waypoint adjustment |

4. **Silent Behaviors and Limits of the KML Importer**:
   - **Silent Altitude Warnings**: The Classic Hub's KML importer silently converts **height 0 m to 30 m** and truncates any altitude outside the `[-200, 500]` meter range (the plugin warns the user before export about these scenarios).
   - **Waypoint Ceiling**: The measured waypoint ceiling for KML files on the classic hub is **10,000** (above that, the excess waypoints silently disappear with no warning), while the `.csv` exporter continues to warn starting at 99.
   - **`.kmz` and `waylines.wpml` Incompatibility**: The **`.kmz` format does not work on the classic hub** (it only works on Hub 2, `hub.flylitchi.com`). You should **not** feed the `waylines.wpml` file to the classic hub, because the WPML coordinates have no altitude and all mission heights would turn into 30 m.

### 2. Export and Loading in DJI Fly
1. **Loading Steps**:
   - In the **DJI Fly** app (or on the DJI RC / RC 2 / RC Pro remote controller), create and save a test mission with **1 waypoint** so that the app creates the file structure and a unique identifier (GUID).
   - Locate the created GUID folder in the device's storage:
     - **Android**: `Android/data/dji.go.v5/files/waypoint`
     - **iOS / Controller Storage**: `Files/DJI Fly/wayline_mission/`
   - Rename the `.kmz` file exported by QGC4QGIS with the same GUID name as the folder.
   - Replace the original `.kmz` file located inside the device's GUID folder with the renamed file generated by the plugin.
2. **Do Not Re-edit in DJI Fly Warning**:
   > [!WARNING]
   > **Do not edit or save the imported mission inside DJI Fly!**  
   > If the mission is re-edited in the DJI Fly app, the app will rewrite the WPML `.kmz` file structure, which can corrupt or remove distance/time-based photo capture triggers and custom waypoint actions.

### 3. Application Limitations and Behavior
- **Waypoint Limit**:
  - **Litchi**: maximum limit of **99 waypoints** per mission; the plugin warns when the mission exceeds this limit.
  - **DJI Fly**: the waypoint ceiling is not publicly documented by DJI; the value cited by the community is **200**, and the plugin warns when the mission exceeds it.
- **Terrain Mode Support in DJI Fly (WPML)**:
  - Terrain mode is converted to altitude relative to the takeoff point, allowing the flight to continue following the terrain.
  - The takeoff point is determined by the `PONTO_DECOLAGEM` parameter (default: elevation of the first waypoint).
  - The plugin issues a warning when any calculated relative height is $\le 0$.

---

## 6. Automatic Elevation Base

QGC4QGIS includes a feature for automatically obtaining terrain elevation data (DEM/DTM) directly from the internet, allowing missions to be planned with terrain tracking (*Terrain Following*) without needing to preload a local raster file.

### 1. Data Source and Consistency with QGC

- **Source**: Copernicus DEM GLO-30 (30-meter global resolution), made available via Auterion's REST web service at the `terrain-ce.suite.auterion.com/api/v1/carpet` endpoint.
- **Consistency with QGroundControl**: This is exactly the **same** source and API natively used by QGroundControl (as implemented in the source [`ElevationMapProvider.h`](../src/Terrain/ElevationMapProvider.h)), ensuring that the terrain altitudes sampled in the `.plan` file are identical to those QGC recalculates and queries internally.

### 2. How to Use It

You can download the elevation grid for your mission area through two interfaces in QGIS:

1. **Dock Widget**: In the **Terrain** group, click the **"Download area DEM…"** button. The plugin will calculate the mission area envelope with the configured margin, automatically download the elevation raster, and add it to the QGIS project layers.
2. **Processing Toolbox**: Use the `qgc4qgis:baixar_dem_copernicus` algorithm. It lets you specify the geographic extent (polygon or envelope), the safety margin, and the destination path of the generated GeoTIFF file.

### 3. Margin Parameter

The **Margin** parameter (extent expansion) adds an extra coverage radius around the mission polygon when requesting data from the server. This margin serves to:
- Ensure elevation coverage in the maneuver curve and acceleration areas (*turnaround*) located outside the main polygon perimeter.
- Ensure terrain sampling at the takeoff point and along the approach trajectories to the first waypoint.

### 4. API Limitations

- **Spatial Resolution**: Nominal resolution of ~30 meters (1 arc second).
- **Internet Connection**: Requires active network access during tile download (*tiles*).
- **Tile Ceiling per Request**: The server limits each request to a maximum of **256 tiles (*tiles*)**, which corresponds to a bounding box (*bounding box*) of approximately **$18 \times 18\text{ km}$**. Requests with larger bounding boxes are rejected by the server.

### 5. Required Attribution

Per the provider's terms of use, using the Copernicus DEM GLO-30 data requires the following copyright attribution:

```text
© Airbus Defence and Space GmbH
```

### 6. Altitude Reference (EGM2008 Geoid)

> [!NOTE]
> **Orthometric Altitude Convention:**  
> The heights provided by the Copernicus DEM are **orthometric** (referenced to the **EGM2008** geoid model — altitude above mean sea level), following exactly the same convention adopted by QGroundControl. They **should not be confused with the ellipsoidal height** obtained directly from GNSS/GPS receivers without applying the geoid model.

### 7. NumPy Compatibility

Downloading the DEM **does not require NumPy**, operating directly via Python and native GDAL bindings. Since version 0.6.1, the plugin does not import the GDAL bindings when loaded (lazy import, only at the moment of writing the DEM). Starting with version 0.6.2, the plugin also does not enable GDAL exceptions through the path that imports `osgeo.gdal_array` (a binary linked against NumPy). This way, **no plugin path (loading or DEM download) loads NumPy**, ensuring conflict-free operation in environments with NumPy 2.x (such as QGIS 4), even when the environment's GDAL bindings were compiled with NumPy 1.x.

### 8. Known Issues

- **"A module that was compiled using NumPy 1.x…" warning in the log during DEM download**:
  If the traceback *"A module that was compiled using NumPy 1.x..."* appears in the QGIS **log** as a `WARNING` **while the DEM downloads and loads normally**, this is a C extension compiled against NumPy 1.x being imported by another component of the environment and printing the warning — the operation does not fail.
- **"A module that was compiled using NumPy 1.x…" dialog when installing**:
  If the dialog *"A module that was compiled using NumPy 1.x..."* appears **when installing** the plugin, the origin is another component of QGIS's Python — typical of QGIS in Flatpak with `numpy` 2.x installed via `pip` in `/var/data/python/...`, which shadows the runtime's NumPy and breaks extensions compiled against the older NumPy.
  - **Environment fix**: Remove that `numpy` installed via `pip` in `/var/data/python/...` in the Flatpak (the runtime reverts to using the NumPy paired with its binaries), instead of downgrading NumPy.
  - **Official repository**: Installation via the official repository requires the new version to already be published there.
