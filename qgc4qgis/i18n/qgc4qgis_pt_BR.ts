<?xml version='1.0' encoding='utf-8'?>
<TS version="2.1">
<context>
    <name>qgc4qgis</name>
    <message>
        <location filename="../core/cameras.py" line="9" />
        <source>Custom camera (manual)</source>
        <translation>Câmera personalizada (manual)</translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="30" />
        <source>KML does not carry heading, gimbal, speed, curve, or POI: </source>
        <translation>O KML não transporta proa, gimbal, velocidade, curva nem POI: </translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="30" />
        <source>Litchi applies its own defaults. Adjust in Mission Hub after importing, or use the .csv, which carries everything.</source>
        <translation>o Litchi aplica os padrões dele. Ajuste no Mission Hub após importar, ou use o .csv, que leva tudo.</translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="42" />
        <source>Number of waypoints ({n}) exceeds the Mission Hub ceiling ({MAX_HUB_WAYPOINTS}); </source>
        <translation>Número de waypoints ({n}) excede o teto do Mission Hub ({MAX_HUB_WAYPOINTS}); </translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="42" />
        <source>the excess is silently discarded on import.</source>
        <translation>os excedentes são descartados em silêncio na importação.</translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="52" />
        <source>Height of {a:.2f} m at waypoint {i} outside the range [{MIN_KML_ALTITUDE}, {MAX_KML_ALTITUDE}]: </source>
        <translation>Altura de {a:.2f} m no waypoint {i} fora da faixa [{MIN_KML_ALTITUDE}, {MAX_KML_ALTITUDE}]: </translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="52" />
        <source>Mission Hub truncates it to the limit.</source>
        <translation>o Mission Hub trunca para o limite.</translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="62" />
        <source>Height of waypoint {i} is zero in the KML: </source>
        <translation>Altura do waypoint {i} é zero no KML: </translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="62" />
        <source>Mission Hub replaces zero height with 30 m without warning.</source>
        <translation>o Mission Hub substitui altura zero por 30 m sem avisar.</translation>
    </message>
    <message>
        <location filename="../core/kml.py" line="80" />
        <source>Route has no waypoints: nothing to export to KML.</source>
        <translation>Rota sem waypoints: nada a exportar para KML.</translation>
    </message>
    <message>
        <location filename="../core/litchi.py" line="77" />
        <source>Number of waypoints ({n_waypoints}) exceeds the Litchi limit ({MAX_LITCHI_WAYPOINTS}).</source>
        <translation>Número de waypoints ({n_waypoints}) excede o limite do Litchi ({MAX_LITCHI_WAYPOINTS}).</translation>
    </message>
    <message>
        <location filename="../core/litchi.py" line="86" />
        <source>Gimbal pitch ({pitch:.1f}°) at waypoint {wp_index} outside the range [{MIN_GIMBAL_PITCH:.1f}, {MAX_GIMBAL_PITCH:.1f}].</source>
        <translation>Gimbal pitch ({pitch:.1f}°) no waypoint {wp_index} fora da faixa [{MIN_GIMBAL_PITCH:.1f}, {MAX_GIMBAL_PITCH:.1f}].</translation>
    </message>
    <message>
        <location filename="../core/litchi.py" line="99" />
        <source>Altitude ({wp.altura:.1f}m) at waypoint {wp_index} outside the range [{MIN_ALTITUDE:.1f}, {MAX_ALTITUDE:.1f}].</source>
        <translation>Altitude ({wp.altura:.1f}m) no waypoint {wp_index} fora da faixa [{MIN_ALTITUDE:.1f}, {MAX_ALTITUDE:.1f}].</translation>
    </message>
    <message>
        <location filename="../core/litchi.py" line="124" />
        <source>Trigger mode "{modo}": the CSV does not carry a "Take Photo" action per waypoint — waypoints only sit at the ends of transects and the capture depends on the interval (photo_distinterval/photo_timeinterval). For a photo action at every photo center, use trigger mode "By photo".</source>
        <translation>Modo de disparo "{modo}": o CSV não traz ação "Take Photo" por waypoint — os waypoints ficam só nas pontas dos transectos e a captura depende do intervalo (photo_distinterval/photo_timeinterval). Para uma ação de foto em cada centro de foto, use o modo de disparo "Por foto".</translation>
    </message>
    <message>
        <location filename="../core/route.py" line="255" />
        <source>Number of waypoints ({n_waypoints}) exceeds the limit ({max_waypoints}).</source>
        <translation>Número de waypoints ({n_waypoints}) excede o limite ({max_waypoints}).</translation>
    </message>
    <message>
        <location filename="../core/route.py" line="299" />
        <source>Terrain mode converted to height relative to the takeoff point (elevation {takeoff_elevation:.1f} m); heights from {min_alt:.1f} m to {max_alt:.1f} m.</source>
        <translation>Modo terreno convertido para altura relativa ao ponto de decolagem (elevação {takeoff_elevation:.1f} m); alturas de {min_alt:.1f} m a {max_alt:.1f} m.</translation>
    </message>
    <message>
        <location filename="../core/wpml.py" line="1038" />
        <source>Relative height ≤ 0 in {le_zero_count} waypoint(s): the route goes below the takeoff point.</source>
        <translation>Altura relativa ≤ 0 em {le_zero_count} waypoint(s): a rota passa abaixo do ponto de decolagem.</translation>
    </message>
    <message>
        <location filename="../core/wpml.py" line="1018" />
        <source>WPML export only accepts height relative to the takeoff point; </source>
        <translation>Exportação WPML aceita apenas altura relativa ao ponto de decolagem; </translation>
    </message>
    <message>
        <location filename="../core/wpml.py" line="1018" />
        <source>convert the route with rebase_route_to_takeoff() before exporting (D10).</source>
        <translation>converta a rota com rebase_route_to_takeoff() antes de exportar (D10).</translation>
    </message>
    <message>
        <location filename="../core/wpml.py" line="1045" />
        <source>Number of waypoints ({n_waypoints}) exceeds the DJI WPML limit ({max_waypoints}).</source>
        <translation>Número de waypoints ({n_waypoints}) excede o limite do DJI WPML ({max_waypoints}).</translation>
    </message>
</context>
<context>
    <name>DownloadDemAlgorithm</name>
    <message>
        <location filename="../processing/alg_download_dem.py" line="37" />
        <source>Download Copernicus DEM for area</source>
        <translation>Baixar DEM Copernicus da área</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="41" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="53" />
        <source>Downloads the Copernicus (GLO-30) digital elevation model (DEM) for the extent of the input layer, with a configurable safety margin.

Source: {attribution}</source>
        <translation>Baixa o modelo digital de elevação (DEM) Copernicus (GLO-30) para a extensão da camada de entrada com uma margem de segurança configurável.

Fonte: {attribution}</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="61" />
        <source>Polygon layer</source>
        <translation>Camada de polígonos</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="69" />
        <source>Safety margin (m)</source>
        <translation>Margem de segurança (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="79" />
        <source>Output DEM</source>
        <translation>DEM de saída</translation>
    </message>
    <message>
        <location filename="../processing/alg_download_dem.py" line="131" />
        <source>Error downloading Copernicus DEM: {error}</source>
        <translation>Erro ao baixar DEM Copernicus: {error}</translation>
    </message>
</context>
<context>
    <name>ExportDjiAlgorithm</name>
    <message>
        <location filename="../processing/alg_export_dji.py" line="72" />
        <source>Export DJI Fly mission (.kmz)</source>
        <translation>Exportar missão DJI Fly (.kmz)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="76" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="88" />
        <source>Exports a flight mission in DJI WPML format (.kmz) from coverage polygons or flight grid lines.</source>
        <translation>Exporta uma missão de voo no formato DJI WPML (.kmz) a partir de polígonos de cobertura ou linhas de grade de voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="98" />
        <source>Input layer (Polygons or Lines)</source>
        <translation>Camada de entrada (Polígonos ou Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="106" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="115" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="125" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="135" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="146" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="157" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="168" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="178" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="187" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="195" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="206" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="217" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="228" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="239" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="250" />
        <source>Trigger mode</source>
        <translation>Modo de disparo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="250" />
        <source>By distance</source>
        <translation>Por distância</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="250" />
        <source>By time</source>
        <translation>Por tempo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="250" />
        <source>By photo</source>
        <translation>Por foto</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="259" />
        <source>Speed (m/s)</source>
        <translation>Velocidade (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="269" />
        <source>Gimbal angle (degrees)</source>
        <translation>Ângulo de gimbal (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="280" />
        <source>Wait at waypoint (s)</source>
        <translation>Espera no waypoint (s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="290" />
        <source>Action on finish</source>
        <translation>Ação ao terminar</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="290" />
        <source>Return to home (goHome)</source>
        <translation>Retornar ao início (goHome)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="290" />
        <source>No action (noAction)</source>
        <translation>Nenhuma ação (noAction)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="290" />
        <source>Auto land (autoLand)</source>
        <translation>Pouso automático (autoLand)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="290" />
        <source>Go to first waypoint (gotoFirstWaypoint)</source>
        <translation>Ir para primeiro waypoint (gotoFirstWaypoint)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="304" />
        <source>Action on RC signal lost</source>
        <translation>Ação em perda de rádio</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="304" />
        <source>Return (goBack)</source>
        <translation>Retornar (goBack)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="304" />
        <source>Land (landing)</source>
        <translation>Pousar (landing)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="304" />
        <source>Hover (hover)</source>
        <translation>Pairar (hover)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="317" />
        <source>Transitional speed (m/s)</source>
        <translation>Velocidade de transição (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="327" />
        <source>KMZ (ZIP) file layout</source>
        <translation>Layout do arquivo KMZ (ZIP)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="327" />
        <source>wpmz/ subfolder (DJI default)</source>
        <translation>Subpasta wpmz/ (padrão DJI)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="327" />
        <source>At file root</source>
        <translation>Na raiz do arquivo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="336" />
        <source>Elevation layer (DEM) — if set, exports in above-terrain mode</source>
        <translation>Camada de elevação (DEM) — se definida, exporta em modo acima do terreno</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="345" />
        <source>Terrain tolerance (m)</source>
        <translation>Tolerância do terreno (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="355" />
        <source>Takeoff point (optional — default: first waypoint)</source>
        <translation>Ponto de decolagem (opcional — padrão: primeiro waypoint)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="364" />
        <source>Output file (.kmz)</source>
        <translation>Arquivo de destino (.kmz)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="380" />
        <source>Output file path not specified.</source>
        <translation>Caminho do arquivo de saída não especificado.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="449" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_dji.py" line="573" />
        <source>Could not sample elevation at the takeoff point (outside the DEM or NoData). Choose another point or expand the elevation raster.</source>
        <translation>Não foi possível amostrar a elevação no ponto de decolagem (fora do DEM ou NoData). Escolha outro ponto ou amplie o raster de elevação.</translation>
    </message>
</context>
<context>
    <name>ExportLitchiAlgorithm</name>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="68" />
        <source>Export Litchi mission (.csv)</source>
        <translation>Exportar missão Litchi (.csv)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="72" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="84" />
        <source>Exports a flight mission in Litchi format (.csv) from coverage polygons or flight grid lines.</source>
        <translation>Exporta uma missão de voo no formato Litchi (.csv) a partir de polígonos de cobertura ou linhas de grade de voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="94" />
        <source>Input layer (Polygons or Lines)</source>
        <translation>Camada de entrada (Polígonos ou Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="102" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="111" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="121" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="131" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="142" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="153" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="164" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="174" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="183" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="191" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="202" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="213" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="224" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="235" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="246" />
        <source>Trigger mode</source>
        <translation>Modo de disparo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="246" />
        <source>By distance</source>
        <translation>Por distância</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="246" />
        <source>By time</source>
        <translation>Por tempo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="246" />
        <source>By photo</source>
        <translation>Por foto</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="255" />
        <source>Speed (m/s)</source>
        <translation>Velocidade (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="265" />
        <source>Gimbal angle (degrees)</source>
        <translation>Ângulo de gimbal (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="276" />
        <source>Wait at waypoint (s)</source>
        <translation>Espera no waypoint (s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="286" />
        <source>Elevation layer (DEM) — if set, exports in above-terrain mode</source>
        <translation>Camada de elevação (DEM) — se definida, exporta em modo acima do terreno</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="295" />
        <source>Terrain tolerance (m)</source>
        <translation>Tolerância do terreno (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="305" />
        <source>Takeoff point (optional — default: first waypoint)</source>
        <translation>Ponto de decolagem (opcional — padrão: primeiro waypoint)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="314" />
        <source>Output file (.csv)</source>
        <translation>Arquivo de destino (.csv)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="330" />
        <source>Output file path not specified.</source>
        <translation>Caminho do arquivo de saída não especificado.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="387" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_litchi.py" line="512" />
        <source>Could not sample elevation at the takeoff point (outside the DEM or NoData). Choose another point or expand the elevation raster.</source>
        <translation>Não foi possível amostrar a elevação no ponto de decolagem (fora do DEM ou NoData). Escolha outro ponto ou amplie o raster de elevação.</translation>
    </message>
</context>
<context>
    <name>ExportLitchiKmlAlgorithm</name>
    <message>
        <location filename="../processing/alg_export_kml.py" line="66" />
        <source>Export Litchi Mission Hub mission (.kml)</source>
        <translation>Exportar missão Litchi Mission Hub (.kml)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="70" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="82" />
        <source>Exports a flight mission in Litchi Mission Hub format (.kml) from coverage polygons or flight grid lines.

The generated KML file is meant for import into Litchi Mission Hub (flylitchi.com/hub -&gt; Import).
In the Mission Hub import window, make sure to keep 'Add take photo action' CHECKED and 'Placemarks as POI' UNCHECKED.</source>
        <translation>Exporta uma missão de voo no formato Litchi Mission Hub (.kml) a partir de polígonos de cobertura ou linhas de grade de voo.

O arquivo KML gerado é destinado à importação no Litchi Mission Hub (flylitchi.com/hub -&gt; Import).
Na janela de importação do Mission Hub, certifique-se de manter 'Add take photo action' MARCADO e 'Placemarks as POI' DESMARCADO.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="94" />
        <source>Input layer (Polygons or Lines)</source>
        <translation>Camada de entrada (Polígonos ou Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="102" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="111" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="121" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="131" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="142" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="153" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="164" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="174" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="183" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="191" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="202" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="213" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="224" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="235" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="246" />
        <source>Trigger mode</source>
        <translation>Modo de disparo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="246" />
        <source>By distance</source>
        <translation>Por distância</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="246" />
        <source>By time</source>
        <translation>Por tempo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="246" />
        <source>By photo</source>
        <translation>Por foto</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="255" />
        <source>Speed (m/s)</source>
        <translation>Velocidade (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="265" />
        <source>Elevation layer (DEM) — if set, exports in above-terrain mode</source>
        <translation>Camada de elevação (DEM) — se definida, exporta em modo acima do terreno</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="274" />
        <source>Terrain tolerance (m)</source>
        <translation>Tolerância do terreno (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="284" />
        <source>Takeoff point (optional — default: first waypoint)</source>
        <translation>Ponto de decolagem (opcional — padrão: primeiro waypoint)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="293" />
        <source>Output file (.kml)</source>
        <translation>Arquivo de destino (.kml)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="309" />
        <source>Output file path not specified.</source>
        <translation>Caminho do arquivo de saída não especificado.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="364" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="483" />
        <source>Could not sample elevation at the takeoff point (outside the DEM or NoData). Choose another point or expand the elevation raster.</source>
        <translation>Não foi possível amostrar a elevação no ponto de decolagem (fora do DEM ou NoData). Escolha outro ponto ou amplie o raster de elevação.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_kml.py" line="493" />
        <source>Trigger mode other than "By photo": the KML will only have vertices at the transect ends, and the Mission Hub "Add take photo action" will only trigger there.</source>
        <translation>Modo de disparo diferente de "Por foto": o KML terá vértices só nas pontas dos transectos, e o "Add take photo action" do Mission Hub vai disparar apenas neles.</translation>
    </message>
</context>
<context>
    <name>ExportPlanAlgorithm</name>
    <message>
        <location filename="../processing/alg_export_plan.py" line="70" />
        <source>Export QGC plan (.plan)</source>
        <translation>Exportar plano do QGC (.plan)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="74" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="86" />
        <source>Exports a mission plan in QGroundControl format (.plan) from coverage polygons or flight grid lines.</source>
        <translation>Exporta um plano de missão no formato QGroundControl (.plan) a partir de polígonos de cobertura ou linhas de grade de voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="96" />
        <source>Input layer (Polygons or Lines)</source>
        <translation>Camada de entrada (Polígonos ou Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="104" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="113" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="123" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="133" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="144" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="155" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="166" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="176" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="185" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="193" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="204" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="215" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="226" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="237" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="248" />
        <source>Cruise speed (m/s)</source>
        <translation>Velocidade de cruzeiro (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="258" />
        <source>Hover speed (m/s)</source>
        <translation>Velocidade de pairar (m/s)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="268" />
        <source>Firmware Type</source>
        <translation>Tipo de Firmware</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="277" />
        <source>Vehicle Type</source>
        <translation>Tipo de Veículo</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="277" />
        <source>Fixed Wing (1)</source>
        <translation>Asa Fixa (1)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="286" />
        <source>Elevation layer (DEM) — if set, exports in above-terrain mode</source>
        <translation>Camada de elevação (DEM) — se definida, exporta em modo acima do terreno</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="295" />
        <source>Terrain tolerance (m)</source>
        <translation>Tolerância do terreno (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="305" />
        <source>Output file (.plan)</source>
        <translation>Arquivo de destino (.plan)</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="321" />
        <source>Output file path not specified.</source>
        <translation>Caminho do arquivo de saída não especificado.</translation>
    </message>
    <message>
        <location filename="../processing/alg_export_plan.py" line="379" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
</context>
<context>
    <name>FlightPreviewManager</name>
    <message>
        <location filename="../gui/preview.py" line="57" />
        <source>Preview - Flight Area</source>
        <translation>Pré-visualização - Área de Voo</translation>
    </message>
    <message>
        <location filename="../gui/preview.py" line="131" />
        <source>Preview - Flight Lines</source>
        <translation>Pré-visualização - Linhas de Voo</translation>
    </message>
</context>
<context>
    <name>PhotoCentersAlgorithm</name>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="78" />
        <source>Generate photo centers and footprints</source>
        <translation>Gerar centros de foto e pegadas</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="82" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="94" />
        <source>Generates point layers of photo center positions and footprint polygons, oriented by the flight transects' azimuth.</source>
        <translation>Gera camadas de pontos de centros de tomada de foto e polígonos de pegadas (footprints) orientados pelo azimute dos transectos de voo.</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="104" />
        <source>Input layer (Polygons or Lines)</source>
        <translation>Camada de entrada (Polígonos ou Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="112" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="121" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="131" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="141" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="152" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="163" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="174" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="184" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="193" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="201" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="212" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="223" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="234" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="245" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="256" />
        <source>Photo centers (Points)</source>
        <translation>Centros de foto (Pontos)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="264" />
        <source>Photo footprints (Polygons)</source>
        <translation>Pegadas de foto (Polígonos)</translation>
    </message>
    <message>
        <location filename="../processing/alg_photo_centers.py" line="320" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
</context>
<context>
    <name>Qgc4QgisPlugin</name>
    <message>
        <location filename="../plugin.py" line="82" />
        <source>{name} - Version {version}</source>
        <translation>{name} - Versão {version}</translation>
    </message>
</context>
<context>
    <name>QgcPlanningDockWidget</name>
    <message>
        <location filename="../gui/dock.py" line="63" />
        <source>QGC Flight Planning</source>
        <translation>Planejamento QGC</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="86" />
        <source>Layer / Polygon Feature</source>
        <translation>Camada / Feição de Polígono</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="92" />
        <source>Layer:</source>
        <translation>Camada:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="96" />
        <source>Feature:</source>
        <translation>Feição:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="101" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="111" />
        <source>Manual Camera Specifications</source>
        <translation>Especificações da Câmera Manual</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="120" />
        <source>Sensor width:</source>
        <translation>Largura do sensor:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="128" />
        <source>Sensor height:</source>
        <translation>Altura do sensor:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="135" />
        <source>Image width:</source>
        <translation>Largura da imagem:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="142" />
        <source>Image height:</source>
        <translation>Altura da imagem:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="150" />
        <source>Focal length:</source>
        <translation>Distância focal:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="158" />
        <source>Flight Altitude / GSD</source>
        <translation>Altura de Voo / GSD</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="162" />
        <source>Flight Altitude</source>
        <translation>Altura de Voo</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="163" />
        <source>GSD</source>
        <translation type="unfinished" />
    </message>
    <message>
        <location filename="../gui/dock.py" line="183" />
        <source>Flight Altitude:</source>
        <translation>Altura de Voo:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="192" />
        <source>GSD:</source>
        <translation type="unfinished" />
    </message>
    <message>
        <location filename="../gui/dock.py" line="195" />
        <source>Adjusted footprint:</source>
        <translation>Footprint ajustado:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="201" />
        <source>Flight Parameters</source>
        <translation>Parâmetros do Voo</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="209" />
        <source>Side overlap:</source>
        <translation>Sobreposição lateral:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="216" />
        <source>Frontal overlap:</source>
        <translation>Sobreposição frontal:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="223" />
        <source>Grid angle:</source>
        <translation>Ângulo da grade:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="230" />
        <source>Turnaround:</source>
        <translation type="unfinished" />
    </message>
    <message>
        <location filename="../gui/dock.py" line="235" />
        <source>Entry location:</source>
        <translation>Sentido de entrada:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="237" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="244" />
        <source>Terrain / Elevation</source>
        <translation>Terreno / Elevação</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="251" />
        <source>Elevation layer:</source>
        <translation>Camada de elevação:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="253" />
        <source>Download area DEM…</source>
        <translation>Baixar DEM da área…</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="263" />
        <source>Terrain tolerance:</source>
        <translation>Tolerância do terreno:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="268" />
        <source>Flight Statistics</source>
        <translation>Estatísticas do Voo</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="272" />
        <source>Flight area:</source>
        <translation>Área de voo:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="275" />
        <source>Total distance:</source>
        <translation>Distância total:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="278" />
        <source>Total photos:</source>
        <translation>Total de fotos:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="281" />
        <source>Estimated flight time:</source>
        <translation>Tempo estimado de voo:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="284" />
        <source>Interval between photos:</source>
        <translation>Intervalo entre fotos:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="287" />
        <source>QGC waypoints:</source>
        <translation>Waypoints QGC:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="290" />
        <source>Litchi waypoints:</source>
        <translation>Waypoints Litchi:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="293" />
        <source>DJI waypoints:</source>
        <translation>Waypoints DJI:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="304" />
        <source>Export to</source>
        <translation>Exportar para</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="308" />
        <source>By distance</source>
        <translation>Por distância</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="308" />
        <source>By time</source>
        <translation>Por tempo</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="308" />
        <source>By photo</source>
        <translation>Por foto</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="312" />
        <source>Trigger mode:</source>
        <translation>Modo de disparo:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="320" />
        <source>Flight speed:</source>
        <translation>Velocidade de voo:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="328" />
        <source>Gimbal angle:</source>
        <translation>Ângulo de gimbal:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="336" />
        <source>Waypoint wait:</source>
        <translation>Espera no waypoint:</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="338" />
        <source>Export Litchi (.csv)…</source>
        <translation>Exportar Litchi (.csv)…</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="342" />
        <source>Export classic Litchi Hub (.kml)…</source>
        <translation>Exportar Litchi Hub clássico (.kml)…</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="343" />
        <source>KML for flylitchi.com/hub → Import, with "Add take photo action" checked. Does not carry heading, gimbal, or speed — use the .csv for that.</source>
        <translation>KML para flylitchi.com/hub → Import, com "Add take photo action" marcado. Não leva proa, gimbal nem velocidade — para isso use o .csv.</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="352" />
        <source>Export DJI Fly / Litchi Hub 2 (.kmz)…</source>
        <translation>Exportar DJI Fly / Litchi Hub 2 (.kmz)…</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="355" />
        <source>KMZ WPML: opens in DJI Fly and is imported as a mission by Litchi Hub 2 (hub.flylitchi.com → Import mission). The classic hub (flylitchi.com/hub) does NOT read .kmz — for that use the .csv or the .kml.</source>
        <translation>KMZ WPML: abre no DJI Fly e é importado como missão pelo Litchi Hub 2 (hub.flylitchi.com → Importar missão). O hub clássico (flylitchi.com/hub) NÃO lê .kmz — para ele use o .csv ou o .kml.</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="368" />
        <source>Generate Flight Grid</source>
        <translation>Gerar Grade de Voo</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="372" />
        <source>Export .plan…</source>
        <translation>Exportar .plan…</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="376" />
        <source>Add layers to project</source>
        <translation>Adicionar camadas ao projeto</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="414" />
        <source>All features</source>
        <translation>Todas as feições</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="425" />
        <source>Feature {feat_id}</source>
        <translation>Feição {feat_id}</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="781" />
        <source>{count} (exceeds limit: 99)</source>
        <translation>{count} (excede limite: 99)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="788" />
        <source>{count} (exceeds limit: 200)</source>
        <translation>{count} (excede limite: 200)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="798" />
        <source>Waypoint count ({count}) exceeds the Litchi limit (99).</source>
        <translation>Número de waypoints ({count}) excede o limite do Litchi (99).</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="804" />
        <source>Waypoint count ({count}) exceeds the DJI limit (200).</source>
        <translation>Número de waypoints ({count}) excede o limite do DJI (200).</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1137" />
        <source>Warning</source>
        <translation>Aviso</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1137" />
        <source>Select a valid polygon layer.</source>
        <translation>Selecione uma camada de polígonos válida.</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="906" />
        <source>Export QGC Plan</source>
        <translation>Exportar Plano QGC</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="906" />
        <source>QGroundControl Plan (*.plan);;All Files (*)</source>
        <translation>QGroundControl Plan (*.plan);;Todos os Arquivos (*)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="953" />
        <source>Export Litchi CSV</source>
        <translation>Exportar Litchi CSV</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="953" />
        <source>Litchi Mission (*.csv);;All Files (*)</source>
        <translation>Litchi Mission (*.csv);;Todos os Arquivos (*)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="987" />
        <source>Litchi export error</source>
        <translation>Erro na exportação Litchi</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1003" />
        <source>Export Litchi Mission Hub KML</source>
        <translation>Exportar Litchi Mission Hub KML</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1003" />
        <source>Litchi Mission Hub KML (*.kml);;All Files (*)</source>
        <translation>Litchi Mission Hub KML (*.kml);;Todos os Arquivos (*)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1037" />
        <source>KML export error</source>
        <translation>Erro na exportação KML</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1053" />
        <source>Export DJI Fly KMZ</source>
        <translation>Exportar DJI Fly KMZ</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1053" />
        <source>DJI Fly Mission (*.kmz);;All Files (*)</source>
        <translation>DJI Fly Mission (*.kmz);;Todos os Arquivos (*)</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1087" />
        <source>DJI export error</source>
        <translation>Erro na exportação DJI</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1164" />
        <source>Copernicus DEM</source>
        <translation>DEM Copernicus</translation>
    </message>
    <message>
        <location filename="../gui/dock.py" line="1122" />
        <source>Flight Grid</source>
        <translation>Grade de Voo</translation>
    </message>
</context>
<context>
    <name>SurveyGridAlgorithm</name>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="77" />
        <source>Generate flight grid</source>
        <translation>Gerar grade de voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="81" />
        <source>Flight Planning</source>
        <translation>Planejamento de Voo</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="93" />
        <source>Generates photogrammetric flight grid lines from a polygon layer, camera settings, altitude/GSD, overlaps, angle and turnaround distance.</source>
        <translation>Gera linhas de grade de voo fotogramétrico a partir de uma camada de polígonos, configurações de câmera, altura/GSD, sobreposições, ângulo e distância de turnaround.</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="103" />
        <source>Polygon layer</source>
        <translation>Camada de polígonos</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="111" />
        <source>Camera</source>
        <translation>Câmera</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="120" />
        <source>Flight altitude (m)</source>
        <translation>Altura de voo (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="130" />
        <source>GSD (cm/px) - if &gt; 0, overrides/calculates altitude</source>
        <translation>GSD (cm/px) - se &gt; 0 sobrescreve/calcula altura</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="140" />
        <source>Side overlap (%)</source>
        <translation>Sobreposição lateral (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="151" />
        <source>Frontal overlap (%)</source>
        <translation>Sobreposição frontal (%)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="162" />
        <source>Grid angle (degrees)</source>
        <translation>Ângulo da grade (graus)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="173" />
        <source>Turnaround distance (m)</source>
        <translation>Distância de turnaround (m)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="183" />
        <source>Entry point</source>
        <translation>Ponto de entrada</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="192" />
        <source>Cross grid (Refly 90°)</source>
        <translation>Grade cruzada (Refly 90°)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="200" />
        <source>Manual camera: Sensor width (mm)</source>
        <translation>Câmera manual: Largura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="211" />
        <source>Manual camera: Sensor height (mm)</source>
        <translation>Câmera manual: Altura do sensor (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="222" />
        <source>Manual camera: Image width (px)</source>
        <translation>Câmera manual: Largura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="233" />
        <source>Manual camera: Image height (px)</source>
        <translation>Câmera manual: Altura da imagem (px)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="244" />
        <source>Manual camera: Focal length (mm)</source>
        <translation>Câmera manual: Distância focal (mm)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="255" />
        <source>Flight grid (Lines)</source>
        <translation>Grade de voo (Linhas)</translation>
    </message>
    <message>
        <location filename="../processing/alg_survey_grid.py" line="311" />
        <source>Failed to calculate camera/flight parameters.</source>
        <translation>Falha ao calcular os parâmetros da câmera/voo.</translation>
    </message>
</context>
</TS>