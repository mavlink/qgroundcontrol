<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-CN" sourcelanguage="en">
  <context>
    <name>GimbalFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[gimbalRoll].shortDesc, </extracomment>
      <location filename="../src/Gimbal/GimbalFact.json"/>
      <source>Gimbal Roll</source>
      <translation>云台横滚</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gimbalPitch].shortDesc, </extracomment>
      <location filename="../src/Gimbal/GimbalFact.json"/>
      <source>Gimbal Pitch</source>
      <translation>云台俯仰</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gimbalYaw].shortDesc, </extracomment>
      <location filename="../src/Gimbal/GimbalFact.json"/>
      <source>Gimbal Yaw</source>
      <translation>云台偏航</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gimbalAzimuth].shortDesc, </extracomment>
      <location filename="../src/Gimbal/GimbalFact.json"/>
      <source>Azimuth</source>
      <translation>方位角</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[deviceId].shortDesc, </extracomment>
      <location filename="../src/Gimbal/GimbalFact.json"/>
      <source>gimbal device Id</source>
      <translation>云台设备 ID</translation>
    </message>
  </context>
  <context>
    <name>OfflineMaps.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[minZoomLevelDownload].shortDesc, </extracomment>
      <location filename="../src/Settings/OfflineMaps.SettingsGroup.json"/>
      <source>Minimum zoom level for downloads.</source>
      <translation>下载的最小缩放级别。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxZoomLevelDownload].shortDesc, </extracomment>
      <location filename="../src/Settings/OfflineMaps.SettingsGroup.json"/>
      <source>Maximum zoom level for downloads.</source>
      <translation>下载的最大缩放级别。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxTilesForDownload].shortDesc, </extracomment>
      <location filename="../src/Settings/OfflineMaps.SettingsGroup.json"/>
      <source>Maximum number of tiles for download.</source>
      <translation>可下载的最大瓦片数量。</translation>
    </message>
  </context>
  <context>
    <name>FlyView.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[guidedMinimumAltitude].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Minimum altitude for guided actions altitude slider.</source>
      <translation>引导动作高度滑块的最小高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[guidedMaximumAltitude].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Maximum altitude for guided actions altitude slider.</source>
      <translation>引导动作高度滑块的最大高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showLogReplayStatusBar].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Show/Hide Log Replay status bar</source>
      <translation>显示/隐藏日志回放状态栏</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showAdditionalIndicatorsCompass].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Show additional heading indicators on Compass</source>
      <translation>在罗盘上显示额外航向指示</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lockNoseUpCompass].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Lock Compass Nose-Up</source>
      <translation>锁定罗盘机头朝上</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[keepMapCenteredOnVehicle].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Keep map centered on vehicle</source>
      <translation>地图保持以飞行器为中心</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showSimpleCameraControl].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Show controls for camera triggering using MAV_CMD_DO_DIGICAM_CONTROL.</source>
      <translation>显示使用 MAV_CMD_DO_DIGICAM_CONTROL 触发相机的控件。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showObstacleDistanceOverlay].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Show obstacle distance overlay on map and video.</source>
      <translation>在地图和视频上显示障碍物距离叠加层。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxGoToLocationDistance].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Maximum distance allowed for Go To Location.</source>
      <translation>“前往位置”允许的最大距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardFlightGoToLocationLoiterRad].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Loiter radius for orbiting the Go To Location during forward flight. This only applies if the firmware supports a radius in MAV_CMD_DO_REPOSITION commands.</source>
      <translation>前飞时围绕“前往位置”盘旋的半径。仅当固件支持 MAV_CMD_DO_REPOSITION 命令中的半径时适用。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goToLocationRequiresConfirmInGuided].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Require slide confirmation for Go To Location when the vehicle is already in Guided mode.</source>
      <translation>飞行器已处于引导模式时，执行“前往位置”需要滑动确认。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[updateHomePosition].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Send updated GCS&apos; home position to autopilot in case of change of the home position</source>
      <translation>返航点变化时，将更新后的地面站返航点发送给飞控</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[instrumentQmlFile2].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Qml file for instrument panel</source>
      <translation>仪表面板使用的 QML 文件</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[instrumentQmlFile2].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Integrated Compass &amp; Attitude,Horizontal Compass &amp; Attitude,Large Vertical</source>
      <translation>集成罗盘与姿态,水平罗盘与姿态,大型垂直</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[requestControlAllowTakeover].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>When requesting vehicle control, allow other GCS to override control automatically, or require this GCS to accept the request first.</source>
      <translation>请求飞行器控制权时，允许其他地面站自动接管，或要求本地地面站先接受请求。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[requestControlTimeout].shortDesc, </extracomment>
      <location filename="../src/Settings/FlyView.SettingsGroup.json"/>
      <source>Timeout in seconds before a request to a GCS to allow takeover is assumed to be rejected. This is used to display the timeout graphically on requestor and GCS in control.</source>
      <translation>请求地面站允许接管的超时时间（秒）；超时后视为拒绝。该值用于在请求方和当前控制地面站上显示超时进度。</translation>
    </message>
  </context>
  <context>
    <name>App.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingFirmwareClass].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing firmware class</source>
      <translation>离线编辑固件类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingFirmwareClass].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>ArduPilot,PX4 Pro,Mavlink Generic</source>
      <translation>ArduPilot,PX4 Pro,MAVLink 通用</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingVehicleClass].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing vehicle class</source>
      <translation>离线编辑飞行器类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingVehicleClass].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Fixed Wing,Multi-Rotor,VTOL,Rover,Sub,Mavlink Generic</source>
      <translation>固定翼,多旋翼,VTOL,车/船,潜航器,MAVLink 通用</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingCruiseSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing cruise speed</source>
      <translation>离线编辑巡航速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingCruiseSpeed].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>This value defines the default speed for calculating mission statistics for vehicles which do not support hover or VTOL vehicles in fixed wing mode. It does not modify the flight speed for a specific flight plan.</source>
      <translation>此值用于为不支持悬停的飞行器或处于固定翼模式的 VTOL 飞行器计算任务统计信息的默认速度。它不会修改具体飞行计划的飞行速度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingHoverSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing hover speed</source>
      <translation>离线编辑悬停速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingHoverSpeed].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>This value defines the default speed for calculating mission statistics for multi-rotor vehicles or VTOL vehicle in multi-rotor mode. It does not modify the flight speed for a specific flight plan.</source>
      <translation>此值用于为多旋翼飞行器或处于多旋翼模式的 VTOL 飞行器计算任务统计信息的默认速度。它不会修改具体飞行计划的飞行速度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingAscentSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing ascent speed</source>
      <translation>离线编辑上升速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingAscentSpeed].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>This value defines the ascent speed for multi-rotor vehicles for use in calculating mission duration.</source>
      <translation>此值定义多旋翼飞行器的上升速度，用于计算任务时长。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingDescentSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Offline editing descent speed</source>
      <translation>离线编辑下降速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[offlineEditingDescentSpeed].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>This value defines the cruising speed for multi-rotor vehicles for use in calculating mission duration.</source>
      <translation>此值定义多旋翼飞行器用于计算任务时长的巡航速度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryPercentRemainingAnnounce].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Announce battery remaining percent</source>
      <translation>播报电池剩余百分比</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryPercentRemainingAnnounce].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Announce the remaining battery percent when it falls below the specified percentage.</source>
      <translation>当电池剩余百分比低于指定值时播报。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[defaultMissionItemAltitude].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Default value for altitude</source>
      <translation>默认高度值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[defaultMissionItemAltitude].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>This value specifies the default altitude for new items added to a mission.</source>
      <translation>此值指定任务中新添加项目的默认高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[audioMuted].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Mute audio output</source>
      <translation>静音音频输出</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[audioMuted].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is enabled all audio output will be muted.</source>
      <translation>启用此选项后，所有音频输出都将被静音。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystick].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Show virtual joystick</source>
      <translation>显示虚拟摇杆</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystick].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is enabled the virtual joystick will be shown on the Fly view.</source>
      <translation>启用此选项后，飞行视图中将显示虚拟摇杆。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystickAutoCenterThrottle].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Auto-Center Throttle</source>
      <translation>油门自动回中</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystickAutoCenterThrottle].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If enabled the throttle stick will snap back to center when released.</source>
      <translation>启用后，松开油门杆时油门会自动回中。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystickLeftHandedMode].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Left Handed Mode</source>
      <translation>左手模式</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[virtualJoystickLeftHandedMode].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is enabled the virtual joystick layout will be reversed</source>
      <translation>启用此选项后，虚拟摇杆布局将反向显示</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gstDebugLevel].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Video streaming debug</source>
      <translation>视频流调试</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gstDebugLevel].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Sets the environment variable GST_DEBUG for all pipeline elements on boot.</source>
      <translation>启动时为所有管线元素设置 GST_DEBUG 环境变量。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gstDebugLevel].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Disabled,Error,Warning,FixMe,Info,Debug,Log,Trace</source>
      <translation>禁用,错误,警告,FixMe,信息,调试,日志,跟踪</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[useChecklist].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Use preflight checklist</source>
      <translation>使用飞前检查单</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[useChecklist].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is enabled the preflight checklist will be used.</source>
      <translation>启用此选项后，将使用飞前检查单。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enforceChecklist].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Preflight checklist must pass before arming</source>
      <translation>在解锁前必须先通过检查清单</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enforceChecklist].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is enabled the preflight checklist must pass before arming.</source>
      <translation>如果启用此选项，则在解锁前必须通过解锁检查。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enableMultiVehiclePanel].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Enable Multi-Vehicle Panel</source>
      <translation>启用多机面板</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enableMultiVehiclePanel].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Enable Multi-Vehicle Panel when multiple vehicles are connected.</source>
      <translation>连接多架飞行器时启用多机面板。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[appFontPointSize].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Application font size</source>
      <translation>应用字体大小</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[appFontPointSize].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>The point size for the default font used.</source>
      <translation>所用默认字体的点数大小。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[indoorPalette].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Application color scheme</source>
      <translation>应用配色方案</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[indoorPalette].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>The color scheme for the user interface.</source>
      <translation>用户界面的配色方案。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[indoorPalette].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Indoor,Outdoor</source>
      <translation>室内,室外</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[savePath].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Application save directory</source>
      <translation>应用程序保存目录</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[savePath].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Directory to which all data files are saved/loaded from</source>
      <translation>所有数据文件保存和加载所使用的目录</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[androidSaveToSDCard].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Save to SD card</source>
      <translation>保存到 SD 卡</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[androidSaveToSDCard].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Application data is saved to the sd card</source>
      <translation>应用程序数据保存到 sd 卡</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxToken].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Access token to Mapbox maps</source>
      <translation>Mapbox 地图访问令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxToken].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Your personal access token for Mapbox maps</source>
      <translation>你的 Mapbox 地图个人访问令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxAccount].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Account name for Mapbox maps</source>
      <translation>Mapbox 地图账户名称</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxAccount].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Your personal account name for Mapbox maps</source>
      <translation>你的 Mapbox 地图个人账户名称</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxStyle].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Map style ID</source>
      <translation>地图样式 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapboxStyle].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Map design style ID for Mapbox maps</source>
      <translation>Mapbox 地图的地图设计样式 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[esriToken].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Access token to Esri maps</source>
      <translation>Esri 地图访问令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[esriToken].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Your personal access token for Esri maps</source>
      <translation>你的 Esri 地图个人访问令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[customURL].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Custom Map URL</source>
      <translation>自定义地图 URL</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[customURL].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>URL for X Y Z map with {x} {y} {z} or {zoom} substitutions. Eg: https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/{z}/{x}/{y}.png?api=d01ev80nqcjxddfvc6amyvkk1ka</source>
      <translation>X Y Z 地图 URL，可使用 {x} {y} {z} 或 {zoom} 替换。例如：https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/{z}/{x}/{y}.png?api=d01ev80nqcjxddfvc6amyvkk1ka</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vworldToken].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>VWorld Token</source>
      <translation>VWorld 令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vworldToken].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Your personal access token for VWorld maps</source>
      <translation>你的 VWorld 地图个人访问令牌</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[followTarget].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Stream GCS&apos; coordinates to Autopilot</source>
      <translation>将地面站坐标流式发送给自动驾驶仪</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[followTarget].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Never,Always,When in Follow Me Flight Mode</source>
      <translation>从不,始终,处于“跟随我”飞行模式时</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[qLocaleLanguage].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Language</source>
      <translation>语言</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[disableAllPersistence].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Disable all data persistence</source>
      <translation>禁用所有数据持久化</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[disableAllPersistence].longDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>If this option is set, nothing will be saved to disk.</source>
      <translation>设置此选项后，不会向磁盘保存任何内容。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[firstRunPromptIdsShown].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>Comma separated list of first run prompt ids which have already been shown.</source>
      <translation>已显示的首次运行提示 ID 列表，使用英文逗号分隔。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[loginAirLink].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>AirLink User Name</source>
      <translation>AirLink 用户名</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[passAirLink].shortDesc, </extracomment>
      <location filename="../src/Settings/App.SettingsGroup.json"/>
      <source>AirLink Password</source>
      <translation>AirLink 密码</translation>
    </message>
  </context>
  <context>
    <name>MavlinkActions.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[flyViewActionsFile].shortDesc, </extracomment>
      <location filename="../src/Settings/MavlinkActions.SettingsGroup.json"/>
      <source>Name of JSON custom actions file for Fly View</source>
      <translation>Fly 视图 JSON 自定义操作文件名称</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[joystickActionsFile].shortDesc, </extracomment>
      <location filename="../src/Settings/MavlinkActions.SettingsGroup.json"/>
      <source>Name of JSON custom actions file for Joysticks</source>
      <translation>摇杆 JSON 自定义操作文件名称</translation>
    </message>
  </context>
  <context>
    <name>FirmwareUpgrade.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[defaultFirmwareType].shortDesc, </extracomment>
      <location filename="../src/Settings/FirmwareUpgrade.SettingsGroup.json"/>
      <source>Default firmware type for flashing</source>
      <translation>默认刷写固件类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[apmChibiOS].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/FirmwareUpgrade.SettingsGroup.json"/>
      <source>ChibiOS,NuttX</source>
      <translation>ChibiOS,NuttX</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[apmVehicleType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/FirmwareUpgrade.SettingsGroup.json"/>
      <source>Multi-Rotor,Helicopter,Plane,Rover,Sub</source>
      <translation>多旋翼,直升机,固定翼,车/船,潜航器</translation>
    </message>
  </context>
  <context>
    <name>BrandImage.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[userBrandImageIndoor].shortDesc, .QGC.MetaData.Facts[userBrandImageOutdoor].shortDesc, </extracomment>
      <location filename="../src/Settings/BrandImage.SettingsGroup.json"/>
      <source>User-selected brand image</source>
      <translation>用户选择的品牌图像</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[userBrandImageIndoor].longDesc, </extracomment>
      <location filename="../src/Settings/BrandImage.SettingsGroup.json"/>
      <source>Location in file system of user-selected brand image (indoor)</source>
      <translation>用户选择的品牌图像在文件系统中的位置（室内）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[userBrandImageOutdoor].longDesc, </extracomment>
      <location filename="../src/Settings/BrandImage.SettingsGroup.json"/>
      <source>Location in file system of user-selected brand image (outdoor)</source>
      <translation>用户选择的品牌图像在文件系统中的位置（室外）</translation>
    </message>
  </context>
  <context>
    <name>FlightMode.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModes].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes</source>
      <translation>以逗号分隔的隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesMultiRotor].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesMultiRotor].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for MultiRotor</source>
      <translation>以逗号分隔的多旋翼隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesFixedWing].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesFixedWing].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for FixedWing</source>
      <translation>以逗号分隔的固定翼隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesVTOL].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesVTOL].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for VTOL</source>
      <translation>以逗号分隔的 VTOL 隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesRoverBoat].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesRoverBoat].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for RoverBoat</source>
      <translation>以逗号分隔的 Rover/Boat 隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesSub].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesSub].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for Sub</source>
      <translation>以逗号分隔的 Sub 隐藏飞行模式列表</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[px4HiddenFlightModesAirship].shortDesc, .QGC.MetaData.Facts[apmHiddenFlightModesAirship].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMode.SettingsGroup.json"/>
      <source>Comma separated list of hidden flight modes for Airship</source>
      <translation>以逗号分隔的飞艇隐藏飞行模式列表</translation>
    </message>
  </context>
  <context>
    <name>RTK.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[surveyInAccuracyLimit].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Survey in accuracy (U-blox only)</source>
      <translation>精确度调查 (仅限U-blex)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[surveyInAccuracyLimit].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>The minimum accuracy value that Survey-In must achieve before it can complete.</source>
      <translation>Survey-In 完成前必须达到的最低精度值。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[surveyInMinObservationDuration].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Min observation time</source>
      <translation>最短观测时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[surveyInMinObservationDuration].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Defines the minimum amount of observation time for the position calculation.</source>
      <translation>定义位置计算所需的最短观测时间。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[useFixedBasePosition].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Use specified base position</source>
      <translation>使用指定基站位置</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[useFixedBasePosition].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Specify the values for the RTK base position without having to do a survey in.</source>
      <translation>无需执行 Survey-In 即可指定 RTK 基站位置值。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionLatitude].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Base Position Latitude</source>
      <translation>基站位置纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionLatitude].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Defines the latitude of the fixed RTK base position.</source>
      <translation>定义固定 RTK 基站位置的纬度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionLongitude].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Base Position Longitude</source>
      <translation>基站位置经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionLongitude].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Defines the longitude of the fixed RTK base position.</source>
      <translation>定义固定 RTK 基站位置的经度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionAltitude].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Base Position Alt (WGS84)</source>
      <translation>基站位置高度 (WGS84)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionAltitude].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Defines the altitude of the fixed RTK base position.</source>
      <translation>定义固定 RTK 基站位置的高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionAccuracy].shortDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Base Position Accuracy</source>
      <translation>基站位置精度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fixedBasePositionAccuracy].longDesc, </extracomment>
      <location filename="../src/Settings/RTK.SettingsGroup.json"/>
      <source>Defines the accuracy of the fixed RTK base position.</source>
      <translation>定义固定 RTK 基站位置的精度。</translation>
    </message>
  </context>
  <context>
    <name>APMMavlinkStreamRate.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Defines.StreamRateEnumStrings, </extracomment>
      <location filename="../src/Settings/APMMavlinkStreamRate.SettingsGroup.json"/>
      <source>Controlled By Vehicle,0 hz,1 hz,2 hz,3 hz,4 hz,5 hz,6 hz,7 hz,8 hz,9 hz,10 hz,50 hz,100 hz</source>
      <translation>由飞行器控制,0 Hz,1 Hz,2 Hz,3 Hz,4 Hz,5 Hz,6 Hz,7 Hz,8 Hz,9 Hz,10 Hz,50 Hz,100 Hz</translation>
    </message>
  </context>
  <context>
    <name>FlightMap.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapProvider].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMap.SettingsGroup.json"/>
      <source>Currently selected map provider for flight maps</source>
      <translation>当前飞行地图选中的地图提供商</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mapType].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMap.SettingsGroup.json"/>
      <source>Currently selected map type for flight maps</source>
      <translation>当前飞行地图选中的地图类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[elevationMapProvider].shortDesc, </extracomment>
      <location filename="../src/Settings/FlightMap.SettingsGroup.json"/>
      <source>Currently selected elevation map provider</source>
      <translation>当前选中的高程地图提供商</translation>
    </message>
  </context>
  <context>
    <name>AutoConnect.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectUDP].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically open a connection over UDP</source>
      <translation>自动通过 UDP 打开连接</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectUDP].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to a vehicle which is detected on a UDP communication link.</source>
      <translation>启用后，GroundControl 会自动连接通过 UDP 通信链路检测到的飞行器。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectPixhawk].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically connect to a Pixhawk board</source>
      <translation>自动连接 Pixhawk 飞控板</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectPixhawk].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to a Pixhawk board which is connected via USB.</source>
      <translation>启用后，GroundControl 会自动连接通过 USB 接入的 Pixhawk 飞控板。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectSiKRadio].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically connect to a SiK Radio</source>
      <translation>自动连接 SiK 电台</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectSiKRadio].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to a vehicle which is detected on a SiK Radio communication link.</source>
      <translation>启用后，GroundControl 会自动连接通过 SiK 电台通信链路检测到的飞行器。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectRTKGPS].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically connect to an RTK GPS</source>
      <translation>自动连接 RTK GPS</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectRTKGPS].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to an RTK GPS which is connected via USB.</source>
      <translation>启用后，GroundControl 会自动连接通过 USB 接入的 RTK GPS。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectLibrePilot].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically connect to a LibrePilot</source>
      <translation>自动连接 LibrePilot</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectLibrePilot].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to a LibrePilot board which is connected via USB.</source>
      <translation>启用后，GroundControl 会自动连接通过 USB 接入的 LibrePilot 飞控板。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectNmeaPort].shortDesc, .QGC.MetaData.Facts[autoConnectNmeaPort].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>NMEA GPS device for GCS position</source>
      <translation>用于地面站位置的 NMEA GPS 设备</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectNmeaBaud].shortDesc, .QGC.MetaData.Facts[autoConnectNmeaBaud].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>NMEA GPS Baudrate</source>
      <translation>NMEA GPS 波特率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectZeroConf].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Automatically open a connection with Zero-Conf</source>
      <translation>自动通过 Zero-Conf 打开连接</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[autoConnectZeroConf].longDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>If this option is enabled GroundControl will automatically connect to a vehicle which is detected over Zero-Conf.</source>
      <translation>启用后，GroundControl 会自动连接通过 Zero-Conf 检测到的飞行器。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[udpListenPort].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>UDP port for autoconnect</source>
      <translation>自动连接使用的 UDP 端口</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[udpTargetHostIP].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>UDP target host IP for autoconnect</source>
      <translation>自动连接使用的 UDP 目标主机 IP</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[udpTargetHostPort].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>UDP target host port for autoconnect</source>
      <translation>自动连接使用的 UDP 目标主机端口</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[nmeaUdpPort].shortDesc, </extracomment>
      <location filename="../src/Settings/AutoConnect.SettingsGroup.json"/>
      <source>Udp port to receive NMEA streams</source>
      <translation>接收 NMEA 数据流的 UDP 端口</translation>
    </message>
  </context>
  <context>
    <name>Viewer3D.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[enabled].shortDesc, </extracomment>
      <location filename="../src/Settings/Viewer3D.SettingsGroup.json"/>
      <source>Enable the 3D viewer</source>
      <translation>启用 3D 查看器</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[osmFilePath].shortDesc, </extracomment>
      <location filename="../src/Settings/Viewer3D.SettingsGroup.json"/>
      <source>Path to the OSM file for the 3D viewer.</source>
      <translation>3D 查看器的 OSM 文件路径。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[buildingLevelHeight].shortDesc, </extracomment>
      <location filename="../src/Settings/Viewer3D.SettingsGroup.json"/>
      <source>Average Height for each level of the buildings</source>
      <translation>建筑每层平均高度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeBias].shortDesc, </extracomment>
      <location filename="../src/Settings/Viewer3D.SettingsGroup.json"/>
      <source>Altitude bias for vehicles in the 3D View</source>
      <translation>3D 视图中飞行器的高度偏移</translation>
    </message>
  </context>
  <context>
    <name>Maps.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxCacheDiskSize].shortDesc, </extracomment>
      <location filename="../src/Settings/Maps.SettingsGroup.json"/>
      <source>Max disk cache</source>
      <translation>最大磁盘缓存</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxCacheMemorySize].shortDesc, </extracomment>
      <location filename="../src/Settings/Maps.SettingsGroup.json"/>
      <source>Max memory cache</source>
      <translation>最大内存缓存</translation>
    </message>
  </context>
  <context>
    <name>BatteryIndicator.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[valueDisplay].shortDesc, </extracomment>
      <location filename="../src/Settings/BatteryIndicator.SettingsGroup.json"/>
      <source>Select values to display in indicator</source>
      <translation>选择指示器中显示的数值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[valueDisplay].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/BatteryIndicator.SettingsGroup.json"/>
      <source>Percentage,Voltage,Percentage and Voltage</source>
      <translation>百分比,电压,百分比和电压</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[threshold1].shortDesc, </extracomment>
      <location filename="../src/Settings/BatteryIndicator.SettingsGroup.json"/>
      <source>Battery level threshold 1</source>
      <translation>电池电量阈值 1</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[threshold2].shortDesc, </extracomment>
      <location filename="../src/Settings/BatteryIndicator.SettingsGroup.json"/>
      <source>Battery level threshold 2</source>
      <translation>电池电量阈值 2</translation>
    </message>
  </context>
  <context>
    <name>Mavlink.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[telemetrySave].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Save telemetry Log after each flight</source>
      <translation>每次飞行后保存遥测日志</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[telemetrySave].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>If this option is enabled a telemetry will be saved after each flight completes.</source>
      <translation>启用后，每次飞行完成后都会保存遥测日志。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[telemetrySaveNotArmed].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Save telemetry log even if vehicle was not armed</source>
      <translation>即使飞行器未解锁也保存遥测日志</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[telemetrySaveNotArmed].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>If this option is enabled a telemtry log will be saved even if vehicle was never armed.</source>
      <translation>启用后，即使飞行器从未解锁也会保存遥测日志。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[apmStartMavlinkStreams].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Request start of MAVLink telemetry streams (ArduPilot only)</source>
      <translation>请求启动MAVLink遥测流（仅ArduPilot）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[saveCsvTelemetry].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Save CSV Telementry Logs</source>
      <translation>保存 CSV 遥测日志</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[saveCsvTelemetry].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>If this option is enabled, all Facts will be written to a CSV file with a 1 Hertz frequency.</source>
      <translation>启用后，所有 Fact 会以 1 Hz 频率写入 CSV 文件。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardMavlink].shortDesc, .QGC.MetaData.Facts[forwardMavlink].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Enable mavlink forwarding</source>
      <translation>启用 MAVLink 转发</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardMavlinkHostName].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Host name</source>
      <translation>主机名</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardMavlinkHostName].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Host name to forward mavlink to. i.e: localhost:14445</source>
      <translation>MAVLink 转发目标主机名，例如 localhost:14445</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardMavlinkAPMSupportHostName].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Ardupilot Support Host name</source>
      <translation>ArduPilot 支持服务器主机名</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forwardMavlinkAPMSupportHostName].longDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Ardupilot Support server to forward mavlink to. i.e: support.ardupilot.org:xxxx</source>
      <translation>MAVLink 要转发到的 ArduPilot 支持服务器，例如 support.ardupilot.org:xxxx</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mavlink2SigningKey].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>MAVLink 2.0 signing key</source>
      <translation>MAVLink 2.0 签名密钥</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendGCSHeartbeat].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>Send GCS Heartbeat</source>
      <translation>发送地面站心跳</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gcsMavlinkSystemID].shortDesc, </extracomment>
      <location filename="../src/Settings/Mavlink.SettingsGroup.json"/>
      <source>GCS MAVLink System ID</source>
      <translation>地面站 MAVLink 系统 ID</translation>
    </message>
  </context>
  <context>
    <name>Video.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoSource].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video source</source>
      <translation>视频源</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoSource].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Source for video. UDP, TCP, RTSP and UVC Cameras may be supported depending on Vehicle and ground station version.</source>
      <translation>视频来源。根据飞行器和地面站版本，可能支持 UDP、TCP、RTSP 和 UVC 相机。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[udpUrl].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video UDP Url</source>
      <translation>视频 UDP URL</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[udpUrl].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>UDP url address and port to bind to for video stream. Example: 0.0.0.0:5600</source>
      <translation>视频流要绑定的 UDP 地址和端口。例如：0.0.0.0:5600</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rtspUrl].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video RTSP Url</source>
      <translation>视频 RTSP URL</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rtspUrl].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>RTSP url address and port to bind to for video stream. Example: rtsp://192.168.42.1:554/live</source>
      <translation>视频流要绑定的 RTSP 地址和端口。例如：rtsp://192.168.42.1:554/live</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[tcpUrl].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video TCP Url</source>
      <translation>视频 TCP URL</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[tcpUrl].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>TCP url address and port to bind to for video stream. Example: 192.168.143.200:3001</source>
      <translation>视频流要绑定的 TCP 地址和端口。例如：192.168.143.200:3001</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoSavePath].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video save directory</source>
      <translation>视频保存目录</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoSavePath].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Directory to save videos to.</source>
      <translation>用于保存视频的目录。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[aspectRatio].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Aspect Ratio</source>
      <translation>视频宽高比</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[aspectRatio].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Aspect Ratio (width / height). Use 0.0 to ignore it.</source>
      <translation>视频宽高比（宽/高）。设置为 0.0 表示忽略。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gridLines].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Grid Lines</source>
      <translation>视频网格线</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gridLines].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Displays a grid overlaid over the video view.</source>
      <translation>在视频视图上显示网格覆盖。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoFit].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Display Fit</source>
      <translation>适合的视频显示</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoFit].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Handle Video Aspect Ratio.</source>
      <translation>处理视频宽高比。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[videoFit].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Fit Width,Fit Height,Fill,No Crop</source>
      <translation>适应宽度,适应高度,填充,不裁剪</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showRecControl].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Show Video Record Control</source>
      <translation>显示视频录制控件</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showRecControl].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Show recording control in the UI.</source>
      <translation>在界面中显示录制控件。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[recordingFormat].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Recording Format</source>
      <translation>视频录制格式</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[recordingFormat].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video recording file format.</source>
      <translation>视频录制文件格式。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[recordingFormat].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>mkv,mov,mp4</source>
      <translation>mkv,mov,mp4</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxVideoSize].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Max Video Storage Usage</source>
      <translation>最大视频存储占用</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxVideoSize].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Maximum amount of disk space used by video recording.</source>
      <translation>视频录制可使用的最大磁盘空间。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enableStorageLimit].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Enable/Disable Limits on Storage Usage</source>
      <translation>启用/禁用存储占用限制</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[enableStorageLimit].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>When enabled, old video files will be auto-deleted when the total size of QGC-recorded video exceeds the maximum video storage usage.</source>
      <translation>启用后，当 QGC 录制的视频总大小超过最大视频存储占用时，会自动删除旧视频文件。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rtspTimeout].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>RTSP Video Timeout</source>
      <translation>RTSP 视频超时</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rtspTimeout].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>How long to wait before assuming RTSP link is gone.</source>
      <translation>判定 RTSP 链路断开前的等待时间。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[streamEnabled].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Stream Enabled</source>
      <translation>启用视频流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[streamEnabled].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Start/Stop Video Stream.</source>
      <translation>启动/停止视频流。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[disableWhenDisarmed].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Video Stream Disnabled When Armed</source>
      <translation>解锁时禁用视频流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[disableWhenDisarmed].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Disable Video Stream when disarmed.</source>
      <translation>加锁时禁用视频流。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lowLatencyMode].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Tweaks video for lower latency</source>
      <translation>针对低延迟优化视频</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lowLatencyMode].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>If this option is enabled, the rtpjitterbuffer is removed and the video sink is set to assynchronous mode, reducing the latency by about 200 ms.</source>
      <translation>启用后会移除 rtpjitterbuffer，并将视频接收端设为异步模式，约可降低 200 ms 延迟。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forceVideoDecoder].shortDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Force specific category of video decode</source>
      <translation>强制指定视频解码类别</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forceVideoDecoder].longDesc, </extracomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Force the change of prioritization between video decode methods, allowing the user to force some video hardware decode plugins if necessary.</source>
      <translation>强制调整视频解码方式优先级，必要时可强制使用某些硬件解码插件。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[forceVideoDecoder].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/Video.SettingsGroup.json"/>
      <source>Default,Force software decoder,Force NVIDIA decoder,Force VA-API decoder,Force DirectX3D 11 decoder,Force VideoToolbox decoder,Force Intel decoder,Force Vulkan decoder</source>
      <translation>默认,强制软件解码,强制 NVIDIA 解码,强制 VA-API 解码,强制 DirectX3D 11 解码,强制 VideoToolbox 解码,强制 Intel 解码,强制 Vulkan 解码</translation>
    </message>
  </context>
  <context>
    <name>RemoteID.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorID].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator ID</source>
      <translation>操作者 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorID].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator ID. Maximum 20 characters.</source>
      <translation>操作者 ID，最多 20 个字符。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorIDValid].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator ID is valid</source>
      <translation>操作者 ID 有效</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorIDValid].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator ID has been checked using checksum.</source>
      <translation>操作者 ID 已通过校验和检查。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorIDType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator ID type</source>
      <translation>操作者 ID 类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[operatorIDType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>CAA</source>
      <translation>CAA</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendOperatorID].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Send Operator ID</source>
      <translation>发送操作者 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendOperatorID].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>When enabled, sends operator ID message</source>
      <translation>启用后发送操作者 ID 消息</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDFree].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Flight Purpose</source>
      <translation>飞行目的</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDFree].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Optional plain text for operator to specify operations data (Free Text). Maximum 23 characters.</source>
      <translation>操作者用于指定运行数据的可选纯文本（自由文本），最多 23 个字符。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDEmergency].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Emergency Text</source>
      <translation>紧急文本</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDEmergency].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Optional plain text for operator to specify operations data (Emergency Text). Maximum 23 characters.</source>
      <translation>操作者用于指定运行数据的可选纯文本（紧急文本），最多 23 个字符。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDExtended].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Extended Status</source>
      <translation>扩展状态</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDExtended].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Optional plain text for operator to specify operations data (Extended Text). Maximum 23 characters.</source>
      <translation>操作者用于指定运行数据的可选纯文本（扩展文本），最多 23 个字符。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Self ID type</source>
      <translation>自定义 ID 类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[selfIDType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Flight Purpose,Emergency,Extended Status</source>
      <translation>飞行目的,紧急,扩展状态</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendSelfID].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Send Self ID</source>
      <translation>发送自定义 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendSelfID].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>When enabled, sends self ID message</source>
      <translation>启用后发送自定义 ID 消息</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[basicID].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Basic ID</source>
      <translation>基础 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[basicIDType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Basic ID Type</source>
      <translation>基础 ID 类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[basicIDType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>None,SerialNumber (ANSI/CTA-2063),CAA,UTM (RFC4122),Specific</source>
      <translation>无,序列号 (ANSI/CTA-2063),CAA,UTM (RFC4122),指定</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[basicIDUaType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>UA type</source>
      <translation>无人机类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[basicIDUaType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Undefined,Airplane/FixedWing,Helicopter/Multirrotor,Gyroplane,VTOL,Ornithopter,Glider,Kite,Free Ballon,Captive Ballon,Airship,Parachute,Rocket,Tethered powered aircraft,Ground Obstacle,Other</source>
      <translation>未定义,飞机/固定翼,直升机/多旋翼,旋翼机,VTOL,扑翼机,滑翔机,风筝,自由气球,系留气球,飞艇,降落伞,火箭,系留动力航空器,地面障碍物,其他</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendBasicID].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Send Basic ID</source>
      <translation>发送基础 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sendBasicID].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>When enabled, sends basic ID message</source>
      <translation>启用后发送基础 ID 消息</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[region].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Region of operation</source>
      <translation>运行区域</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[region].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>The region of operation the mission will take place in</source>
      <translation>任务将执行的运行区域</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[region].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>FAA,EU</source>
      <translation>FAA,EU</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[locationType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Location Type</source>
      <translation>位置类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[locationType].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Operator location Type</source>
      <translation>操作者位置类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[locationType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Takeoff(Not Supported),Live GNNS, Fixed (not for FAA)</source>
      <translation>起飞点（不支持）,实时 GNSS,固定（不适用于 FAA）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[latitudeFixed].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Latitude Fixed</source>
      <translation>固定纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[latitudeFixed].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Fixed latitude to send on SYSTEM message</source>
      <translation>在 SYSTEM 消息中发送的固定纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[longitudeFixed].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Longitude Fixed</source>
      <translation>固定经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[longitudeFixed].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Fixed Longitude to send on SYSTEM message</source>
      <translation>在 SYSTEM 消息中发送的固定经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeFixed].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Altitude Fixed</source>
      <translation>固定高度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeFixed].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Fixed Altitude to send on SYSTEM message</source>
      <translation>在 SYSTEM 消息中发送的固定高度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classificationType].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Classification Type</source>
      <translation>分类类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classificationType].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Classification Type of UAS</source>
      <translation>无人机系统的分类类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classificationType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Undeclared,EU</source>
      <translation>未声明,EU</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[categoryEU].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Category</source>
      <translation>类别</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[categoryEU].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Category of the UAS in the EU region</source>
      <translation>EU 区域内无人机系统的类别</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[categoryEU].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Undeclared,Open,Specific,Certified</source>
      <translation>未声明,开放类,特定类,认证类</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classEU].shortDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Class</source>
      <translation>等级</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classEU].longDesc, </extracomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Class of the UAS in the EU region</source>
      <translation>EU 区域内无人机系统的等级</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[classEU].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/RemoteID.SettingsGroup.json"/>
      <source>Undeclared,Class 0,Class 1,Class 2,Class 3,Class 4,Class 5,Class 6</source>
      <translation>未声明,0 类,1 类,2 类,3 类,4 类,5 类,6 类</translation>
    </message>
  </context>
  <context>
    <name>ADSBVehicleManager.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[adsbServerConnectEnabled].shortDesc, </extracomment>
      <location filename="../src/Settings/ADSBVehicleManager.SettingsGroup.json"/>
      <source>Connect to ADSB SBS server</source>
      <translation>连接到 ADSB SBS 服务器</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[adsbServerConnectEnabled].longDesc, </extracomment>
      <location filename="../src/Settings/ADSBVehicleManager.SettingsGroup.json"/>
      <source>Connect to ADSB SBS-1 server using specified address/port</source>
      <translation>使用指定地址/端口连接到 ADSB SBS-1 服务器</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[adsbServerHostAddress].shortDesc, </extracomment>
      <location filename="../src/Settings/ADSBVehicleManager.SettingsGroup.json"/>
      <source>Host address</source>
      <translation>主机地址</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[adsbServerPort].shortDesc, </extracomment>
      <location filename="../src/Settings/ADSBVehicleManager.SettingsGroup.json"/>
      <source>Server port</source>
      <translation>服务器端口</translation>
    </message>
  </context>
  <context>
    <name>GimbalController.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[EnableOnScreenControl].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Enable on Screen Camera Control</source>
      <translation>启用屏幕相机控制</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ControlType].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Type of on-screen control</source>
      <translation>屏幕控制类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ControlType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Click to point, click and drag</source>
      <translation>点击指向,点击并拖动</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraVFov].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Vertical camera field of view</source>
      <translation>相机垂直视场角</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraHFov].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Horizontal camera field of view</source>
      <translation>相机水平视场角</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraSlideSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Maximum gimbal speed on click and drag (deg/sec)</source>
      <translation>点击并拖动时的最大云台速度（度/秒）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showAzimuthIndicatorOnMap].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Show gimbal Azimuth indicator over vehicle icon in map</source>
      <translation>在地图飞行器图标上显示云台方位指示</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[toolbarIndicatorShowAzimuth].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Show Azimuth instead of local yaw on top toolbar gimbal indicator</source>
      <translation>顶部工具栏云台指示器显示方位角，而不是本地偏航角</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[toolbarIndicatorShowAcquireReleaseControl].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Show Azimuth Acquire/release buttons in the gimbal buttons panel</source>
      <translation>在云台按钮面板中显示方位角获取/释放按钮</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[joystickButtonsSpeed].shortDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>Rate used for joystick button control (deg/sec)</source>
      <translation>摇杆按钮控制使用的速率（度/秒）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[joystickButtonsSpeed].longDesc, </extracomment>
      <location filename="../src/Settings/GimbalController.SettingsGroup.json"/>
      <source>When a joystick button is set to gimbal left/right/up/down, it will send this rate when pressed, and it will stop moving when button is released</source>
      <translation>当摇杆按钮设置为云台左/右/上/下时，按下按钮会以此速率发送控制，松开按钮后停止移动。</translation>
    </message>
  </context>
  <context>
    <name>PlanView.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[displayPresetsTabFirst].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Display the presets tab at start</source>
      <translation>启动时显示预设标签页</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showMissionItemStatus].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Show/Hide the mission item status display</source>
      <translation>显示/隐藏任务项状态显示</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[takeoffItemNotRequired].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Allow missions to not require a takeoff item</source>
      <translation>允许任务不要求起飞项</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[allowMultipleLandingPatterns].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Allow configuring multiple landing sequences if the firmware supports it. The first one will be used for the mission, but in the event of an RTL, the one that is closest will be used instead.</source>
      <translation>允许在固件支持时配置多个着陆序列。第一个将用于任务，但发生 RTL 时将改用距离最近的着陆序列。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[useConditionGate].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Use MAV_CMD_CONDITION_GATE for pattern generation</source>
      <translation>使用 MAV_CMD_CONDITION_GATE 生成航线模式</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[showGimbalOnlyWhenSet].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Show gimbal yaw visual only when set explicitly for the waypoint</source>
      <translation>仅当为航点明确设置时，才显示云台偏航视觉信息。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vtolTransitionDistance].shortDesc, </extracomment>
      <location filename="../src/Settings/PlanView.SettingsGroup.json"/>
      <source>Amount of distance required for vehicle to complete a transition</source>
      <translation>飞行器完成转换所需的距离</translation>
    </message>
  </context>
  <context>
    <name>DistanceSensorFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationNone].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Forward</source>
      <translation>前方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw45].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Forward/Right</source>
      <translation>前右</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw90].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Right</source>
      <translation>右方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw135].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Rear/Right</source>
      <translation>后右</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw180].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Rear</source>
      <translation>后方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw225].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Rear/Left</source>
      <translation>后左</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw270].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Left</source>
      <translation>左方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationYaw315].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Forward/Left</source>
      <translation>前左</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationPitch90].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Up</source>
      <translation>上方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rotationPitch270].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Down</source>
      <translation>下方</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[minDistance].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Minimum distance sensor can detect</source>
      <translation>距离传感器可检测的最小距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[maxDistance].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/DistanceSensorFact.json"/>
      <source>Maximum distance sensor can detect</source>
      <translation>距离传感器可检测的最大距离</translation>
    </message>
  </context>
  <context>
    <name>SetpointFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[roll].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Roll Setpoint</source>
      <translation>横滚设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[pitch].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Pitch Setpoint</source>
      <translation>俯仰设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[yaw].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Yaw Setpoint</source>
      <translation>偏航设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rollRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Roll Rate Setpoint</source>
      <translation>横滚速率设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[pitchRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Pitch Rate Setpoint</source>
      <translation>俯仰速率设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[yawRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SetpointFact.json"/>
      <source>Yaw Rate Setpoint</source>
      <translation>偏航速率设定值</translation>
    </message>
  </context>
  <context>
    <name>BatteryFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[id].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Battery Id</source>
      <translation>电池 ID</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryFunction].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Battery Function</source>
      <translation>电池功能</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryFunction].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>n/a,All Flight Systems,Propulsion,Avionics,Payload</source>
      <translation>n/a,All Flight Systems,Propulsion,Avionics,Payload</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryType].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Battery Type</source>
      <translation>电池类型</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryType].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>n/a,LIPO,LIFE,LION,NIMH</source>
      <translation>n/a,LIPO,LIFE,LION,NIMH</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[voltage].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Voltage</source>
      <translation>电压</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[percentRemaining].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Percent</source>
      <translation>百分比</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mahConsumed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Consumed</source>
      <translation>已消耗</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[current].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Current</source>
      <translation>电流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[temperature].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Temperature</source>
      <translation>温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[instantPower].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Watts</source>
      <translation>瓦特</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[timeRemaining].shortDesc, .QGC.MetaData.Facts[timeRemainingStr].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Time Remaining</source>
      <translation>剩余时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[chargeState].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>Charge State</source>
      <translation>充电状态</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[chargeState].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/BatteryFact.json"/>
      <source>n/a,Ok,Low,Critical,Emergency,Failed,Unhealthy,Charging</source>
      <translation>n/a,Ok,Low,Critical,Emergency,Failed,Unhealthy,Charging</translation>
    </message>
  </context>
  <context>
    <name>GeneratorFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[status].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Status</source>
      <translation>状态</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[genSpeed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Generator Speed</source>
      <translation>发电机转速</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batteryCurrent].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Battery Current</source>
      <translation>电池电流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[loadCurrent].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Load Current</source>
      <translation>负载电流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[powerGenerated].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Power Generated</source>
      <translation>发电功率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[busVoltage].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Bus Voltage</source>
      <translation>母线电压</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rectifierTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Rectifier Temperature</source>
      <translation>整流器温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[batCurrentSetpoint].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Battery Current Setpoint</source>
      <translation>电池电流设定值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[genTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Generator Temperature</source>
      <translation>发电机温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[runtime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>runtime</source>
      <translation>运行时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[timeMaintenance].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GeneratorFact.json"/>
      <source>Time until Maintenance</source>
      <translation>维护前剩余时间</translation>
    </message>
  </context>
  <context>
    <name>GPSFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[lat].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>Latitude</source>
      <translation>纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lon].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>Longitude</source>
      <translation>经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[mgrs].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>MGRS Position</source>
      <translation>MGRS 位置</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[hdop].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>HDOP</source>
      <translation>HDOP</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vdop].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>VDOP</source>
      <translation>VDOP</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[courseOverGround].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>Course Over Ground</source>
      <translation>地面航迹向</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lock].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>GPS Lock</source>
      <translation>GPS 锁定</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lock].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>None,None,2D Lock,3D Lock,3D DGPS Lock,3D RTK GPS Lock (float),3D RTK GPS Lock (fixed),Static (fixed)</source>
      <translation>无,无,2D 锁定,3D 锁定,3D DGPS 锁定,3D RTK GPS 锁定（浮动）,3D RTK GPS 锁定（固定）,静态（固定）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[count].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/GPSFact.json"/>
      <source>Sat Count</source>
      <translation>卫星数</translation>
    </message>
  </context>
  <context>
    <name>VibrationFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[xAxis].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Vibe xAxis</source>
      <translation>X 轴振动</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[yAxis].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Vibe yAxis</source>
      <translation>Y 轴振动</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[zAxis].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Vibe zAxis</source>
      <translation>Z 轴振动</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[clipCount1].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Clip Count (1)</source>
      <translation>削波计数 (1)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[clipCount2].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Clip Count (2)</source>
      <translation>片段数量 (2)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[clipCount3].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VibrationFact.json"/>
      <source>Clip Count (3)</source>
      <translation>削波计数 (3)</translation>
    </message>
  </context>
  <context>
    <name>WindFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[direction].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/WindFact.json"/>
      <source>Wind Direction</source>
      <translation>风向</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[speed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/WindFact.json"/>
      <source>Wind Spd</source>
      <translation>风速</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[verticalSpeed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/WindFact.json"/>
      <source>Wind Spd (vert)</source>
      <translation>垂直风速</translation>
    </message>
  </context>
  <context>
    <name>EscStatusFactGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[index].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EscStatusFactGroup.json"/>
      <source>Index Of The First ESC In This Message</source>
      <translation>此消息中第一个 ESC 的索引</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpmFirst].shortDesc, .QGC.MetaData.Facts[rpmSecond].shortDesc, .QGC.MetaData.Facts[rpmThird].shortDesc, .QGC.MetaData.Facts[rpmFourth].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EscStatusFactGroup.json"/>
      <source>Rotation Per Minute</source>
      <translation>每分钟转速</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentFirst].shortDesc, .QGC.MetaData.Facts[currentSecond].shortDesc, .QGC.MetaData.Facts[currentThird].shortDesc, .QGC.MetaData.Facts[currentFourth].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EscStatusFactGroup.json"/>
      <source>Current</source>
      <translation>电流</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[voltageFirst].shortDesc, .QGC.MetaData.Facts[voltageSecond].shortDesc, .QGC.MetaData.Facts[voltageThird].shortDesc, .QGC.MetaData.Facts[voltageFourth].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EscStatusFactGroup.json"/>
      <source>Voltage</source>
      <translation>电压</translation>
    </message>
  </context>
  <context>
    <name>TerrainFactGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[blocksPending].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/TerrainFactGroup.json"/>
      <source>Blocks Pending</source>
      <translation>待处理块</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[blocksLoaded].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/TerrainFactGroup.json"/>
      <source>Blocks Loaded</source>
      <translation>已加载块</translation>
    </message>
  </context>
  <context>
    <name>EFIFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[health].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Health</source>
      <translation>健康状态</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ecuIndex].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Ecu Index</source>
      <translation>ECU 索引</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpm].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Rpm</source>
      <translation>RPM</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fuelConsumed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Fuel Consumed</source>
      <translation>已耗燃油</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[fuelFlow].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Fuel Flow</source>
      <translation>燃油流量</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[engineLoad].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Engine Load</source>
      <translation>发动机负载</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[throttlePos].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Throttle Position</source>
      <translation>油门位置</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[sparkTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Spark dwell time</source>
      <translation>点火驻留时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[baroPress].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>BarometricPressure</source>
      <translation>气压</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[intakePress].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Intake mainfold pressure</source>
      <translation>进气歧管压力</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[intakeTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Intake mainfold temperature</source>
      <translation>进气歧管温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[cylinderTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Cylinder head temperature</source>
      <translation>气缸盖温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ignTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Ignition Timing</source>
      <translation>点火正时</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[injTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Injection Time</source>
      <translation>喷油时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[exGasTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Exhaust gas Temperature</source>
      <translation>排气温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[throttleOut].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Throttle Out</source>
      <translation>油门输出</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ptComp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EFIFact.json"/>
      <source>Pt Compensation</source>
      <translation>Pt 补偿</translation>
    </message>
  </context>
  <context>
    <name>TemperatureFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[temperature1].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/TemperatureFact.json"/>
      <source>Temperature (1)</source>
      <translation>温度 (1)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[temperature2].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/TemperatureFact.json"/>
      <source>Temperature (2)</source>
      <translation>温度 (2)</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[temperature3].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/TemperatureFact.json"/>
      <source>Temperature (3)</source>
      <translation>温度 (3)</translation>
    </message>
  </context>
  <context>
    <name>SubmarineFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[cameraTilt].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Camera Tilt</source>
      <translation>相机倾斜</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[tetherTurns].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Tether Turns</source>
      <translation>缆绳圈数</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lights1].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Lights 1 level</source>
      <translation>灯光 1 亮度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[lights2].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Lights 2 level</source>
      <translation>灯光 2 亮度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[pilotGain].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Pilot Gain</source>
      <translation>驾驶员增益</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[inputHold].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Input Hold</source>
      <translation>输入保持</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[inputHold].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Disabled,Enabled</source>
      <translation>禁用,启用</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rangefinderDistance].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Rangefinder</source>
      <translation>测距仪</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rangefinderTarget].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>RFTarget</source>
      <translation>RF 目标</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rollPitchToggle].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Roll/Pitch Toggle</source>
      <translation>横滚/俯仰切换</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rollPitchToggle].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/Vehicle/FactGroups/SubmarineFact.json"/>
      <source>Disabled,Enabled,Unavailable</source>
      <translation>禁用,启用,不可用</translation>
    </message>
  </context>
  <context>
    <name>VehicleFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[roll].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Roll</source>
      <translation>横滚</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[pitch].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Pitch</source>
      <translation>俯仰</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[heading].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Heading</source>
      <translation>航向</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rollRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Roll Rate</source>
      <translation>横滚角速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[pitchRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Pitch Rate</source>
      <translation>俯仰角速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[yawRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Yaw Rate</source>
      <translation>偏航角速度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[groundSpeed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Ground Speed</source>
      <translation>地速</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[airSpeed].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Air Speed</source>
      <translation>空速</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[climbRate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Climb Rate</source>
      <translation>爬升率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeRelative].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Alt (Rel)</source>
      <translation>高度（相对）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeAMSL].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Alt (AMSL)</source>
      <translation>高度（海拔）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[altitudeAboveTerr].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Alt (Above Terrain)</source>
      <translation>高度（距地形）</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[flightDistance].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Flight Distance</source>
      <translation>飞行距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[distanceToHome].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Distance to Home</source>
      <translation>距返航点距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[timeToHome].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Time to Home</source>
      <translation>到返航点时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[headingToHome].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Heading to Home</source>
      <translation>返航点方位</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[distanceToGCS].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Distance to GCS</source>
      <translation>距地面站距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[missionItemIndex].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Mission Item Index</source>
      <translation>任务项索引</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[headingToNextWP].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Next WP Heading</source>
      <translation>下一航点航向</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[distanceToNextWP].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Next WP distance</source>
      <translation>下一航点距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[flightTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Flight Time</source>
      <translation>飞行时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[hobbs].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Hobbs Meter</source>
      <translation>霍布斯计时器</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[throttlePct].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Throttle %</source>
      <translation>油门 %</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[imuTemp].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/VehicleFact.json"/>
      <source>Imu temperature</source>
      <translation>IMU 温度</translation>
    </message>
  </context>
  <context>
    <name>ClockFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/ClockFact.json"/>
      <source>Time</source>
      <translation>时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentUTCTime].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/ClockFact.json"/>
      <source>UTC Time</source>
      <translation>UTC 时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentDate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/ClockFact.json"/>
      <source>Date</source>
      <translation>日期</translation>
    </message>
  </context>
  <context>
    <name>LocalPositionFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[x].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[y].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[z].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vx].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>VX</source>
      <translation>VX</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vy].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>Vy</source>
      <translation>Vy</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vz].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/LocalPositionFact.json"/>
      <source>Vz</source>
      <translation>Vz</translation>
    </message>
  </context>
  <context>
    <name>HygrometerFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[temperature].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/HygrometerFact.json"/>
      <source>Temperature</source>
      <translation>温度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[humidity].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/HygrometerFact.json"/>
      <source>Humidity %</source>
      <translation>湿度 %</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[hygrometerid].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/HygrometerFact.json"/>
      <source>ID</source>
      <translation>ID</translation>
    </message>
  </context>
  <context>
    <name>RPMFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpm1].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/RPMFact.json"/>
      <source>RPM 1</source>
      <translation>转速 1</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpm2].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/RPMFact.json"/>
      <source>RPM 2</source>
      <translation>转速 2</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpm3].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/RPMFact.json"/>
      <source>RPM 3</source>
      <translation>转速 3</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[rpm4].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/RPMFact.json"/>
      <source>RPM 4</source>
      <translation>转速 4</translation>
    </message>
  </context>
  <context>
    <name>EstimatorStatusFactGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodAttitudeEsimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Attitude Esimate</source>
      <translation>良好的姿态估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodHorizVelEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Horiz Vel Estimate</source>
      <translation>良好的水平速度估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodVertVelEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Vert Vel Estimate</source>
      <translation>良好的垂直速度估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodHorizPosRelEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Horiz Pos Rel Estimate</source>
      <translation>良好的相对水平位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodHorizPosAbsEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Horiz Pos Abs Estimate</source>
      <translation>良好的绝对水平位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodVertPosAbsEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Vert Pos Abs Estimate</source>
      <translation>良好的绝对垂直位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodVertPosAGLEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Vert Pos AGL Estimate</source>
      <translation>良好的 AGL 垂直位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodConstPosModeEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Const Pos Mode Estimate</source>
      <translation>良好的恒定位置模式估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodPredHorizPosRelEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Pred Horiz Pos Rel Estimate</source>
      <translation>良好的预测相对水平位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[goodPredHorizPosAbsEstimate].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Good Pred Horiz Pos Abs Estimate</source>
      <translation>良好的预测绝对水平位置估计</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[gpsGlitch].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Gps Glitch</source>
      <translation>GPS 异常</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[accelError].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Accel Error</source>
      <translation>加速度计错误</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[velRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Vel Ratio</source>
      <translation>速度比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[horizPosRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Horiz Pos Ratio</source>
      <translation>水平位置比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vertPosRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Vert Pos Ratio</source>
      <translation>垂直位置比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[magRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Mag Ratio</source>
      <translation>磁力计比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[haglRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>HAGL Ratio</source>
      <translation>HAGL 比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[tasRatio].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>TAS Ratio</source>
      <translation>TAS 比率</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[horizPosAccuracy].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Horiz Pos Accuracy</source>
      <translation>水平位置精度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[vertPosAccuracy].shortDesc, </extracomment>
      <location filename="../src/Vehicle/FactGroups/EstimatorStatusFactGroup.json"/>
      <source>Vert Pos Accuracy</source>
      <translation>垂直位置精度</translation>
    </message>
  </context>
  <context>
    <name>GPSRTKFact.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[connected].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Connected</source>
      <translation>已连接</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentAccuracy].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Current Survey-In Accuracy</source>
      <translation>当前 Survey-In 精度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentLatitude].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Current Survey-In Latitude</source>
      <translation>当前 Survey-In 纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentLongitude].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Current Survey-In Longitude</source>
      <translation>当前 Survey-In 经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentAltitude].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Current Survey-In Altitude</source>
      <translation>当前 Survey-In 高度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[currentDuration].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Current Survey-In Duration</source>
      <translation>当前 Survey-In 持续时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[valid].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Survey-In Valid</source>
      <translation>Survey-In 有效</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[active].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Survey-In Active</source>
      <translation>Survey-In 活动中</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[numSatellites].shortDesc, </extracomment>
      <location filename="../src/GPS/GPSRTKFact.json"/>
      <source>Number of Satellites</source>
      <translation>卫星数量</translation>
    </message>
  </context>
  <context>
    <name>RCToParamDialog.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Scale].shortDesc, </extracomment>
      <location filename="../src/QmlControls/RCToParamDialog.FactMetaData.json"/>
      <source>Scale the RC range</source>
      <translation>缩放 RC 范围</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CenterValue].shortDesc, </extracomment>
      <location filename="../src/QmlControls/RCToParamDialog.FactMetaData.json"/>
      <source>Parameter value when RC output is 0</source>
      <translation>RC 输出为 0 时的参数值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[MinValue].shortDesc, </extracomment>
      <location filename="../src/QmlControls/RCToParamDialog.FactMetaData.json"/>
      <source>Minimum parameter value</source>
      <translation>最小参数值</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[MaxValue].shortDesc, </extracomment>
      <location filename="../src/QmlControls/RCToParamDialog.FactMetaData.json"/>
      <source>Maximum parameter value</source>
      <translation>最大参数值</translation>
    </message>
  </context>
  <context>
    <name>EditPositionDialog.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Latitude].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>Latitude of item position</source>
      <translation>项目位置纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Longitude].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>Longitude of item position</source>
      <translation>项目位置经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Easting].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>Easting of item position</source>
      <translation>项目位置东坐标</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Northing].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>Northing of item position</source>
      <translation>项目位置北坐标</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Zone].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>UTM zone</source>
      <translation>UTM 分区</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Hemisphere].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>Hemisphere for position</source>
      <translation>位置所在半球</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Hemisphere].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>North,South</source>
      <translation>北,南</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[MGRS].shortDesc, </extracomment>
      <location filename="../src/QmlControls/EditPositionDialog.FactMetaData.json"/>
      <source>MGRS coordinate</source>
      <translation>MGRS 坐标</translation>
    </message>
  </context>
  <context>
    <name>QGCMapCircle.Facts.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Radius].shortDesc, </extracomment>
      <location filename="../src/QmlControls/QGCMapCircle.Facts.json"/>
      <source>Radius for geofence circle.</source>
      <translation>地理围栏圆半径。</translation>
    </message>
  </context>
  <context>
    <name>APMFollowComponent.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[angle].shortDesc, </extracomment>
      <location filename="../src/AutoPilotPlugins/APM/APMFollowComponent.FactMetaData.json"/>
      <source>Angle from ground station to vehicle</source>
      <translation>地面站到飞行器的角度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[distance].shortDesc, </extracomment>
      <location filename="../src/AutoPilotPlugins/APM/APMFollowComponent.FactMetaData.json"/>
      <source>Horizontal distance from ground station to vehicle</source>
      <translation>地面站到飞行器的水平距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[height].shortDesc, </extracomment>
      <location filename="../src/AutoPilotPlugins/APM/APMFollowComponent.FactMetaData.json"/>
      <source>Vertical distance from Launch (home) position to vehicle</source>
      <translation>起飞（Home）位置到飞行器的垂直距离</translation>
    </message>
  </context>
  <context>
    <name>TransectStyle.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[TurnAroundDistance].shortDesc, .QGC.MetaData.Facts[TurnAroundDistanceMultiRotor].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>Amount of additional distance to add outside the survey area for vehicle turn around.</source>
      <translation>为飞行器转弯在测绘区域外额外增加的距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraTriggerInTurnAround].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>Camera continues taking images in turn arounds.</source>
      <translation>相机在转弯过程中继续拍照。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[HoverAndCapture].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>Stop and Hover at each image point before taking image</source>
      <translation>拍照前在每个成像点停止并悬停</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Refly90Degrees].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>Refly the pattern at a 90 degree angle</source>
      <translation>以 90 度角重新飞行该航线模式</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[TerrainAdjustTolerance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>Additional waypoints within the transect will be added if the terrain altitude difference grows larger than this tolerance.</source>
      <translation>如果航线条带内的地形高度差超过此容差，将添加额外航点。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[TerrainAdjustMaxClimbRate].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>The maximum climb rate from one waypoint to another when adjusting for terrain. Set to 0 for no max.</source>
      <translation>地形调整时，从一个航点到另一个航点的最大爬升率。设为 0 表示不限制最大值。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[TerrainAdjustMaxDescentRate].shortDesc, </extracomment>
      <location filename="../src/MissionManager/TransectStyle.SettingsGroup.json"/>
      <source>The maximum descent rate from one waypoint to another when adjusting for terrain. Set to 0 for no max.</source>
      <translation>地形调整时，从一个航点到另一个航点的最大下降率。设为 0 表示不限制最大值。</translation>
    </message>
  </context>
  <context>
    <name>VTOLLandingPattern.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Distance between landing and approach points.</source>
      <translation>着陆点与进近点之间的距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingHeading].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Heading from approach to land point.</source>
      <translation>从进近点到着陆点的航向。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FinalApproachAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Altitude to begin landing approach from.</source>
      <translation>开始着陆进近的高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LoiterRadius].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Loiter radius.</source>
      <translation>盘旋半径。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LoiterClockwise].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Loiter clockwise around the final approach point.</source>
      <translation>绕最终进近点顺时针盘旋。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Altitude for landing point on ground.</source>
      <translation>地面着陆点高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[UseLoiterToAlt].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Use a loiter to altitude item for final appoach. Otherwise use a regular waypoint.</source>
      <translation>最终进近使用盘旋到高度项，否则使用普通航点。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StopTakingPhotos].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Stop taking photos</source>
      <translation>停止拍照</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StopTakingVideo].shortDesc, </extracomment>
      <location filename="../src/MissionManager/VTOLLandingPattern.FactMetaData.json"/>
      <source>Stop taking video</source>
      <translation>停止录像</translation>
    </message>
  </context>
  <context>
    <name>CorridorScan.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Altitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CorridorScan.SettingsGroup.json"/>
      <source>Altitude for the bottom layer of the structure scan.</source>
      <translation>结构扫描底层高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CorridorWidth].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CorridorScan.SettingsGroup.json"/>
      <source>Corridor width. Specify 0 width for a single pass scan.</source>
      <translation>走廊宽度。指定宽度为 0 表示单次通过扫描。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Trigger distance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CorridorScan.SettingsGroup.json"/>
      <source>Distance between each triggering of the camera. 0 specifies not camera trigger.</source>
      <translation>每次触发相机之间的距离。0 表示不触发相机。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[GridSpacing].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CorridorScan.SettingsGroup.json"/>
      <source>Amount of spacing in between parallel grid lines.</source>
      <translation>平行网格线之间的间距。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[TurnaroundDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CorridorScan.SettingsGroup.json"/>
      <source>Amount of additional distance to add outside the survey area for vehicle turnaround.</source>
      <translation>为飞行器转弯在测绘区域外额外增加的距离。</translation>
    </message>
  </context>
  <context>
    <name>StructureScan.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[GimbalPitch].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Gimbal pitch rotation.</source>
      <translation>云台俯仰旋转。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[EntranceAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Vehicle will fly to/from the structure at this altitude.</source>
      <translation>飞行器将以此高度飞往/飞离结构。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ScanBottomAlt].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Altitude for the bottomost covered area of the scan. You can adjust this value such that the Bottom Layer Alt will fly above obstacles on the ground.</source>
      <translation>扫描覆盖区域最底部的高度。可调整此值，使“底层高度”高于地面障碍物。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Layers].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Number of scan layers.</source>
      <translation>扫描层数。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StructureHeight].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Height of structure being scanned.</source>
      <translation>被扫描结构的高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StartFromTop].shortDesc, </extracomment>
      <location filename="../src/MissionManager/StructureScan.SettingsGroup.json"/>
      <source>Start scanning from top of structure.</source>
      <translation>从结构顶部开始扫描。</translation>
    </message>
  </context>
  <context>
    <name>MavCmdInfoMultiRotor.json</name>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoMultiRotor.json"/>
      <source>Yaw</source>
      <translation>Yaw</translation>
    </message>
  </context>
  <context>
    <name>MissionSettings.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[PlannedHomePositionAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/MissionSettings.FactMetaData.json"/>
      <source>Launch position altitude</source>
      <translation>起飞位置高度</translation>
    </message>
  </context>
  <context>
    <name>CameraSpec.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Name].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Camera name.</source>
      <translation>相机名称。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[SensorWidth].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Width of camera image sensor.</source>
      <translation>相机图像传感器宽度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[SensorHeight].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Height of camera image sensor.</source>
      <translation>相机图像传感器高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ImageWidth].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Camera image resolution width.</source>
      <translation>相机图像分辨率宽度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ImageHeight].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Camera image resolution height.</source>
      <translation>相机图像分辨率高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FocalLength].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Focal length of camera lens.</source>
      <translation>相机镜头焦距。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Landscape].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Camera on vehicle is in landscape orientation.</source>
      <translation>飞行器上的相机为横向安装。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FixedOrientation].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Camera orientation ix fixed and cannot be changed.</source>
      <translation>相机方向固定，无法更改。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[MinTriggerInterval].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSpec.FactMetaData.json"/>
      <source>Minimum amount of time between each camera trigger.</source>
      <translation>每次相机触发之间的最短时间。</translation>
    </message>
  </context>
  <context>
    <name>MavCmdInfoFixedWing.json</name>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoFixedWing.json"/>
      <source>Pitch</source>
      <translation>Pitch</translation>
    </message>
  </context>
  <context>
    <name>CameraCalc.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraName].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Camera name.</source>
      <translation>相机名称。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ValueSetIsDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Value specified is distance to surface.</source>
      <translation>指定值为到表面的距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[DistanceToSurface].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Distance vehicle is away from surface.</source>
      <translation>飞行器距表面的距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ImageDensity].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Image desity at surface.</source>
      <translation>表面图像密度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FrontalOverlap].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Amount of overlap between images in the forward facing direction.</source>
      <translation>前向方向图像之间的重叠量。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[SideOverlap].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraCalc.FactMetaData.json"/>
      <source>Amount of overlap between images in the side facing direction.</source>
      <translation>侧向方向图像之间的重叠量。</translation>
    </message>
  </context>
  <context>
    <name>SpeedSection.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[FlightSpeed].shortDesc, </extracomment>
      <location filename="../src/MissionManager/SpeedSection.FactMetaData.json"/>
      <source>Set the current flight speed</source>
      <translation>设置当前飞行速度</translation>
    </message>
  </context>
  <context>
    <name>FWLandingPattern.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Distance between approach and land points.</source>
      <translation>进近点与着陆点之间的距离。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingHeading].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Heading from approach to land point.</source>
      <translation>从进近点到着陆点的航向。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FinalApproachAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Altitude to begin landing approach from.</source>
      <translation>开始着陆进近的高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[UseDoChangeSpeed].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Command a specific speed for the approach, useful for reducing energy before the glide slope.</source>
      <translation>为进近命令指定速度，有助于在下滑道前降低能量。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FinalApproachSpeed].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Speed to perform the approach at.</source>
      <translation>执行进近的速度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LoiterRadius].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Loiter radius.</source>
      <translation>盘旋半径。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LoiterClockwise].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Loiter clockwise around the final approach point.</source>
      <translation>绕最终进近点顺时针盘旋。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[LandingAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Altitude for landing point.</source>
      <translation>着陆点高度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[GlideSlope].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>The glide slope between the loiter and landing point.</source>
      <translation>盘旋点与着陆点之间的下滑道。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[ValueSetIsDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Value controller approach point is distance</source>
      <translation>控制器进近点值为距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[UseLoiterToAlt].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Use a loiter to altitude item for final appoach. Otherwise use a regular waypoint.</source>
      <translation>最终进近使用盘旋到高度项，否则使用普通航点。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StopTakingPhotos].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Stop taking photos</source>
      <translation>停止拍照</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[StopTakingVideo].shortDesc, </extracomment>
      <location filename="../src/MissionManager/FWLandingPattern.FactMetaData.json"/>
      <source>Stop taking video</source>
      <translation>停止录像</translation>
    </message>
  </context>
  <context>
    <name>RallyPoint.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Latitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/RallyPoint.FactMetaData.json"/>
      <source>Latitude of rally point position</source>
      <translation>集结点位置纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Longitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/RallyPoint.FactMetaData.json"/>
      <source>Longitude of rally point position</source>
      <translation>集结点位置经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[RelativeAltitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/RallyPoint.FactMetaData.json"/>
      <source>Altitude of rally point position (home relative)</source>
      <translation>集结点位置高度（相对 Home）</translation>
    </message>
  </context>
  <context>
    <name>BreachReturn.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[Latitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/BreachReturn.FactMetaData.json"/>
      <source>Latitude of breach return point position</source>
      <translation>突破返回点位置纬度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Longitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/BreachReturn.FactMetaData.json"/>
      <source>Longitude of breach return point position</source>
      <translation>突破返回点位置经度</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[Altitude].shortDesc, </extracomment>
      <location filename="../src/MissionManager/BreachReturn.FactMetaData.json"/>
      <source>Altitude of breach return point position (Rel)</source>
      <translation>突破返回点位置高度（相对）</translation>
    </message>
  </context>
  <context>
    <name>MavCmdInfoCommon.json</name>
    <message>
      <extracomment>.mavCmdInfo[HomeRaw].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Home Position</source>
      <translation>起始位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[HomeRaw].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Planned home position for mission.</source>
      <translation>计划执行任务的起始位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[HomeRaw].category, .mavCmdInfo[MAV_CMD_NAV_WAYPOINT].category, .mavCmdInfo[MAV_CMD_NAV_RETURN_TO_LAUNCH].category, .mavCmdInfo[MAV_CMD_NAV_LAND].category, .mavCmdInfo[MAV_CMD_NAV_TAKEOFF].category, .mavCmdInfo[MAV_CMD_NAV_SPLINE_WAYPOINT].category, .mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].category, .mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].category, .mavCmdInfo[MAV_CMD_CONDITION_DELAY].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Basic</source>
      <translation>常规</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[HomeRaw].param5.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Latitude</source>
      <translation>纬度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[HomeRaw].param6.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Longitude</source>
      <translation>经度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Waypoint</source>
      <translation>航点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Travel to a position in 3D space.</source>
      <translation>前往三维空间中的某个位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].param1.label, .mavCmdInfo[MAV_CMD_NAV_SPLINE_WAYPOINT].param1.label, .mavCmdInfo[MAV_CMD_NAV_DELAY].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Hold</source>
      <translation>保持</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Acceptance</source>
      <translation>接受</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Pass Radius</source>
      <translation>通过半径</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_WAYPOINT].param4.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_UNLIM].param4.label, .mavCmdInfo[MAV_CMD_NAV_LAND].param4.label, .mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].param4.label, .mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].param4.label, .mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Yaw</source>
      <translation>偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_UNLIM].friendlyName, .mavCmdInfo[MAV_CMD_NAV_LOITER_UNLIM].category, .mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].category, .mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].category, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter</source>
      <translation>盘旋</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_UNLIM].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Travel to a position and Loiter around the specified position indefinitely.</source>
      <translation>移动到一个位置，在指定的半径周围盘旋。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_UNLIM].param3.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param3.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param3.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter (turns)</source>
      <translation>盘旋 (旋转)</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Travel to a position and Loiter around the specified position for a number of turns.</source>
      <translation>移动到一个位置，在指定的半径周围盘旋指定圈數。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Turns</source>
      <translation>转到</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param2.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param2.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Leave Loiter</source>
      <translation>离开盤旋模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param2.enumStrings, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Direction of next waypoint,Any direction</source>
      <translation>Direction of next waypoint,Any direction</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param4.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param4.label, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Exit loiter from</source>
      <translation>退出盘旋</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TURNS].param4.enumStrings, .mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param4.enumStrings, .mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].param4.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Center,Tangent</source>
      <translation>中心,切线</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter (time)</source>
      <translation>悬停(时间)</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Travel to a position and Loiter around the specified position for an amount of time.</source>
      <translation>Travel to a position and Loiter around the specified position for an amount of time.</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter Time</source>
      <translation>Loiter Time</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TIME].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Direction of next waypoint,Current direction</source>
      <translation>Direction of next waypoint,Current direction</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_RETURN_TO_LAUNCH].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Return To Launch</source>
      <translation>返回起飞点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_RETURN_TO_LAUNCH].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Send the vehicle back to the launch position.</source>
      <translation>将无人机送回起飞位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Land</source>
      <translation>降落</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Land vehicle at the specified location.</source>
      <translation>在指定位置降落无人机。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Abort Alt</source>
      <translation>中止高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Precision Land</source>
      <translation>Precision Land</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Disabled,Opportunistic,Required</source>
      <translation>Disabled,Opportunistic,Required</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Takeoff</source>
      <translation>起飞</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Launch from the ground and travel towards the specified takeoff position.</source>
      <translation>从地面发射并前往指定的起飞位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LAND_LOCAL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Land local</source>
      <translation>降落地点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF_LOCAL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Takeoff local</source>
      <translation>本地起飞</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_FOLLOW].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Nav follow</source>
      <translation>跟随导航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Change Altitude</source>
      <translation>改变高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Continue on the current course and climb/descend to specified altitude. When the altitude is reached continue to the next command.</source>
      <translation>沿当前路线继续，然后爬升/下降到指定的高度。 到达高度后，继续执行下一个命令。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT].category, .mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].category, .mavCmdInfo[MAV_CMD_DO_LAND_START].category, .mavCmdInfo[MAV_CMD_DO_INVERTED_FLIGHT].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Flight control</source>
      <translation>飞行控制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT].param1.label, .mavCmdInfo[MAV_CMD_DO_SET_MODE].param1.label, .mavCmdInfo[MAV_CMD_DO_SET_HOME].param1.label, .mavCmdInfo[MAV_CMD_DO_SET_ROI].param1.label, .mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param1.label, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param1.label, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param7.label, .mavCmdInfo[MAV_CMD_SET_CAMERA_MODE].param2.label, .mavCmdInfo[MAV_CMD_DO_VTOL_TRANSITION].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Climb,Neutral,Descend</source>
      <translation>爬升,中立,下降</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter (altitude)</source>
      <translation>盘旋（高度）</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_LOITER_TO_ALT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Loiter at specified position until altitude reached.</source>
      <translation>在指定位置盘旋，直到达到高度为止。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FOLLOW].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Follow Me</source>
      <translation>跟随模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FOLLOW_REPOSITION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Vehicle reposition</source>
      <translation>无人机重置位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Path planning</source>
      <translation>路线规划</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control autonomous path planning.</source>
      <translation>控制自主规划路径。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].category, .mavCmdInfo[MAV_CMD_DO_SET_MODE].category, .mavCmdInfo[MAV_CMD_DO_JUMP].category, .mavCmdInfo[MAV_CMD_DO_SET_HOME].category, .mavCmdInfo[MAV_CMD_DO_SET_REVERSE].category, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].category, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].category, .mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].category, .mavCmdInfo[MAV_CMD_DO_GRIPPER].category, .mavCmdInfo[MAV_CMD_DO_AUTOTUNE_ENABLE].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Advanced</source>
      <translation>高级</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Local planning</source>
      <translation>本地规划</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Disable,Enable,Enable+reset</source>
      <translation>禁用,启用,启用+重置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Full planning</source>
      <translation>全面规划</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Disable,Enable,Enable+reset,Enable+reset route only</source>
      <translation>禁用,启用,启用+重置,仅启用+重置路线</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_PATHPLANNING].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Heading goal</source>
      <translation>标题目标</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_SPLINE_WAYPOINT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Spline waypoint</source>
      <translation>样条航路点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_SPLINE_WAYPOINT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Travel to a position in 3D space using spline path.</source>
      <translation>使用样条曲线路径移动到三维空间中的某个位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_ALTITUDE_WAIT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Altitude wait</source>
      <translation>高度等待</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>VTOL takeoff</source>
      <translation>垂直起降</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Hover straight up to specified altitude, transition to fixed-wing and fly to the specified takeoff location.</source>
      <translation>悬停在指定的高度上，过渡到固定翼并飞到指定的起飞位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Transition Heading</source>
      <translation>Transition Heading</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Default,Next waypoint,Takeoff,Specified,Any</source>
      <translation>Default,Next waypoint,Takeoff,Specified,Any</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>VTOL land</source>
      <translation>垂起着陆</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Fly to specified location at current altitude, transition to multi-rotor and land.</source>
      <translation>飞行到当前高度的指定位置，过渡到多旋翼并着陆。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Approach Alt</source>
      <translation>Approach Alt</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_GUIDED_ENABLE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Guided enable</source>
      <translation>启用引用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_GUIDED_ENABLE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable/Disabled guided mode.</source>
      <translation>启用/禁用引导模式。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_GUIDED_ENABLE].param1.label, .mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].param1.label, .mavCmdInfo[MAV_CMD_DO_AUTOTUNE_ENABLE].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable</source>
      <translation>启用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_GUIDED_ENABLE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>On,Off</source>
      <translation>开启,关闭</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_DELAY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay until</source>
      <translation>延迟至</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_DELAY].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay until the specified time is reached.</source>
      <translation>延迟直到到达指定时间。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_DELAY].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Hour (utc)</source>
      <translation>小时(utc)</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_DELAY].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Min (utc)</source>
      <translation>分（utc）</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_DELAY].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Sec (utc)</source>
      <translation>秒（utc）</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_DELAY].friendlyName, .mavCmdInfo[MAV_CMD_CONDITION_DELAY].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay</source>
      <translation>延时</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_DELAY].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay the mission for the number of seconds.</source>
      <translation>延迟任务秒数。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_CHANGE_ALT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Wait for altitude</source>
      <translation>等待高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_CHANGE_ALT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay the mission until the specified altitide is reached.</source>
      <translation>将任务推迟到达到指定的高度为止。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_CHANGE_ALT].category, .mavCmdInfo[MAV_CMD_CONDITION_DISTANCE].category, .mavCmdInfo[MAV_CMD_CONDITION_YAW].category, .mavCmdInfo[MAV_CMD_CONDITION_GATE].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Conditionals</source>
      <translation>条件</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_CHANGE_ALT].param1.label, .mavCmdInfo[MAV_CMD_CONDITION_YAW].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Rate</source>
      <translation>速率</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_DISTANCE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Wait for distance</source>
      <translation>等待距离</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_DISTANCE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay the mission until within the specified distance of the next waypoint.</source>
      <translation>将任务推迟到下一个航点的指定距离以内。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_DISTANCE].param1.label, .mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Distance</source>
      <translation>距离</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Wait for Yaw</source>
      <translation>等待偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay the mission until the specified heading is reached.</source>
      <translation>Delay the mission until the specified heading is reached.</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Heading</source>
      <translation>Heading</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].param3.label, .mavCmdInfo[MAV_CMD_DO_SET_REVERSE].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].param3.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Clockwise,Shortest,Counter-Clockwise</source>
      <translation>顺时针,最短路径,逆时针</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].param4.label, .mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Offset</source>
      <translation>偏移量</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_YAW].param4.enumStrings, .mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param4.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Relative,Absolute</source>
      <translation>相对,绝对</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_MODE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set mode</source>
      <translation>设置模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_MODE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set flight mode</source>
      <translation>设置飞行模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_MODE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set flight mode.</source>
      <translation>设置飞行模式。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_MODE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Custom Mode</source>
      <translation>自定义模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_MODE].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Sub Mode</source>
      <translation>子模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_JUMP].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Jump to item</source>
      <translation>跳转到项目</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_JUMP].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Mission will continue at the specified item.</source>
      <translation>任务将在指定的项目继续。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_JUMP].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Item #</source>
      <translation>项目 #</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_JUMP].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Repeat</source>
      <translation>重复</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Change speed</source>
      <translation>更改速度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Change speed and/or throttle set points.</source>
      <translation>更改速度和/或节点设定。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Airspeed,Ground Speed,Ascend Speed,Descend Speed</source>
      <translation>空速,地速,爬升速度,下降速度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Speed</source>
      <translation>速度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CHANGE_SPEED].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Throttle</source>
      <translation>油门</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_HOME].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set launch location</source>
      <translation>设置启动位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_HOME].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Changes the launch location either to the current location or a specified location.</source>
      <translation>更改起飞位置到当前位置或指定位置。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_HOME].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Vehicle position,Specified position</source>
      <translation>飞行器位置,指定位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_PARAMETER].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set Parameter</source>
      <translation>设置参数</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set relay</source>
      <translation>设置继电器</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set relay to a condition.</source>
      <translation>将继电器设置为条件。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].param1.label, .mavCmdInfo[MAV_CMD_DO_REPEAT_RELAY].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Relay #</source>
      <translation>继电器 #</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Value</source>
      <translation>值</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPEAT_RELAY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cycle relay</source>
      <translation>循环继电器</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPEAT_RELAY].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cycle relay on/off for desired cycles/time.</source>
      <translation>循环继电器开/关以达到预期的周期/时间。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPEAT_RELAY].param2.label, .mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cycles</source>
      <translation>循环</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPEAT_RELAY].param3.label, .mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Time</source>
      <translation>时间</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_SERVO].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set servo</source>
      <translation>设置舵机</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_SERVO].description, .mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set servo to specified PWM value.</source>
      <translation>设置舵机为指定的 PWM 值。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_SERVO].param1.label, .mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Servo</source>
      <translation>舵机</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_SERVO].param2.label, .mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>PWM</source>
      <translation>PWM</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPEAT_SERVO].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cycle servo</source>
      <translation>循环伺服</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set actuator</source>
      <translation>设置执行器</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set actuator to specified output value (range [-1, 1]).</source>
      <translation>将执行器设置为指定输出值（范围 [-1, 1]）。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Actuator 1</source>
      <translation>执行器 1</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Actuator 2</source>
      <translation>执行器 2</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Actuator 3</source>
      <translation>执行器 3</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ACTUATOR].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Actuator 4</source>
      <translation>执行器 4</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FLIGHTTERMINATION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Flight termination</source>
      <translation>飞行终止</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_LAND_START].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Land start</source>
      <translation>开始着陆</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_LAND_START].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Marker to indicate start of landing sequence.</source>
      <translation>指示着陆顺序开始的标记。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_RALLY_LAND].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Rally land</source>
      <translation>集合地</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GO_AROUND].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Go around</source>
      <translation>盘旋</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_REPOSITION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Reposition</source>
      <translation>更改位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_PAUSE_CONTINUE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Pause/Continue</source>
      <translation>暂停/继续</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_REVERSE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set moving direction</source>
      <translation>设置移动方向</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_REVERSE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set moving direction to forward or reverse.</source>
      <translation>将移动方向设置为向前或向后方向。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_REVERSE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Forward,Reverse</source>
      <translation>向前,向后</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_LOCATION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Region of interest (ROI)</source>
      <translation>目标兴趣范围 (ROI)</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_LOCATION].description, .mavCmdInfo[MAV_CMD_DO_SET_ROI].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Sets the region of interest for cameras.</source>
      <translation>设置摄像头目标区域。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_LOCATION].category, .mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].category, .mavCmdInfo[MAV_CMD_DO_SET_ROI_NONE].category, .mavCmdInfo[MAV_CMD_DO_SET_ROI].category, .mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].category, .mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].category, .mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].category, .mavCmdInfo[MAV_CMD_SET_CAMERA_MODE].category, .mavCmdInfo[MAV_CMD_IMAGE_START_CAPTURE].category, .mavCmdInfo[MAV_CMD_IMAGE_STOP_CAPTURE].category, .mavCmdInfo[MAV_CMD_VIDEO_START_CAPTURE].category, .mavCmdInfo[MAV_CMD_VIDEO_STOP_CAPTURE].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Camera</source>
      <translation>相机</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>ROI to next waypoint</source>
      <translation>兴趣区到下一个航点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Sets the region of interest to point towards the next waypoint with optional offsets.</source>
      <translation>设置兴趣区来用可选补偿指向下一个航点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].param5.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Pitch offset</source>
      <translation>俯仰补偿</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].param6.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Roll offset</source>
      <translation>滚转补偿</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET].param7.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Yaw offset</source>
      <translation>偏航补偿</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_NONE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cancel ROI</source>
      <translation>取消 ROI</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI_NONE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cancels the region of interest.</source>
      <translation>取消感兴趣的区域。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_CONTROL_VIDEO].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control video</source>
      <translation>控制视频</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Region of interest</source>
      <translation>感兴趣区域</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>None,Next waypoint,Mission item,Location,ROI item</source>
      <translation>无,下一个航点,任务项目,位置,ROI 项目</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Mission Index</source>
      <translation>任务索引</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_ROI].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>ROI Index</source>
      <translation>ROI 索引</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Camera config</source>
      <translation>相机配置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Configure onboard camera controller.</source>
      <translation>配置板载摄像头控制器。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Shutter spd</source>
      <translation>快门速度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Aperture</source>
      <translation>光圈</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>ISO</source>
      <translation>感光度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param5.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Exposure</source>
      <translation>曝光</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param6.label, .mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param5.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Command</source>
      <translation>指令</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONFIGURE].param7.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Cut off</source>
      <translation>关闭</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Camera control</source>
      <translation>相机控制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control onboard camera.</source>
      <translation>控制机载摄像头。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Session</source>
      <translation>Session会话</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Zoom</source>
      <translation>缩放</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Step</source>
      <translation>步骤</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Focus lock</source>
      <translation>聚焦锁</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_DIGICAM_CONTROL].param6.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Id</source>
      <translation>ID</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Configure Mount</source>
      <translation>配置挂载</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Configure the vehicle mount (e.g. gimbal).</source>
      <translation>配置无人机挂载（例如云台）。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param1.enumStrings, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param7.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Retract,Neutral,Mavlink Targeting,RC Targeting,GPS Point</source>
      <translation>收回,中立,MAVLink 定位,RC 定位,GPS 点</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stabilize Roll</source>
      <translation>滚转稳定</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param2.enumStrings, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param3.enumStrings, .mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param4.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>No,Yes</source>
      <translation>否,是</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stabilize Pitch</source>
      <translation>俯仰稳定</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONFIGURE].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stabilize Yaw</source>
      <translation>偏航稳定</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control Mount</source>
      <translation>控制挂载</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control the vehicle mount (e.g. gimbal).</source>
      <translation>控制无人机支架（例如云台）。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Lat/Pitch</source>
      <translation>纬度/俯仰</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Lon/Roll</source>
      <translation>经度/滚转</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Alt/Yaw</source>
      <translation>高度/偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Gimbal Manager PitchYaw</source>
      <translation>云台管理器俯仰/偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control the gimbal during the mission</source>
      <translation>在任务期间控制云台</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Pitch</source>
      <translation>俯仰</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Pitch rate</source>
      <translation>俯仰速率</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Yaw rate</source>
      <translation>偏航速率</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param5.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Follow yaw</source>
      <translation>跟随偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param5.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Follow yaw, Lock yaw</source>
      <translation>跟随偏航,锁定偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param7.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Gimbal</source>
      <translation>云台</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW].param7.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Primary,first gimbal,second gimbal</source>
      <translation>主云台,第一云台,第二云台</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Camera trigger distance</source>
      <translation>相机定距触发的间距</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set camera trigger distance.</source>
      <translation>设置相机触发距离。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Shutter</source>
      <translation>快门</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].param3.label, .mavCmdInfo[MAV_CMD_DO_PARACHUTE].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Trigger</source>
      <translation>触发源</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_CAM_TRIGG_DIST].param3.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>No Trigger,Once Immediately</source>
      <translation>无触发,立即触发</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable geofence</source>
      <translation>启用地理围栏</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable/Disable geofence.</source>
      <translation>启用/禁用地理围栏。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].category, .mavCmdInfo[MAV_CMD_DO_PARACHUTE].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Safety</source>
      <translation>安全</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Disable,Disable floor only,Enable</source>
      <translation>禁用,仅禁用地面,启用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_PARACHUTE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Trigger parachute</source>
      <translation>触发降落伞</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_PARACHUTE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable/Disable auto-release or Release a parachute</source>
      <translation>启用/禁用自动释放或释放降落伞</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_PARACHUTE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Disable,Enable,Release</source>
      <translation>禁用,启用,释放</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOTOR_TEST].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Motor test</source>
      <translation>电机测试</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_INVERTED_FLIGHT].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Inverted flight</source>
      <translation>倒飞</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_INVERTED_FLIGHT].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Change to/from inverted flight.</source>
      <translation>更改到/从反向飞行。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_INVERTED_FLIGHT].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Inverted</source>
      <translation>反转</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_INVERTED_FLIGHT].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Normal,Inverted</source>
      <translation>正常,反转</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GRIPPER].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Gripper Mechanism</source>
      <translation>夹爪机构</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GRIPPER].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control a gripper mechanism.</source>
      <translation>控制夹爪机构。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GRIPPER].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Gripper id</source>
      <translation>夹持器 id</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GRIPPER].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Action</source>
      <translation>执行</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GRIPPER].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Release,Grab</source>
      <translation>释放,抓取</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_AUTOTUNE_ENABLE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>AutoTune Enable</source>
      <translation>自动调试启用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_AUTOTUNE_ENABLE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>AutoTune Enable.</source>
      <translation>自动调试已启用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_AUTOTUNE_ENABLE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Enable,Disable</source>
      <translation>启用,禁用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Guided limits</source>
      <translation>指导限制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set limits for external control</source>
      <translation>设置外部控制限制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].param1.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Timeout</source>
      <translation>超时</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Min Alt</source>
      <translation>最小高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Max Alt</source>
      <translation>最大高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_GUIDED_LIMITS].param4.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>H Limit</source>
      <translation>H 限制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PREFLIGHT_CALIBRATION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Calibration</source>
      <translation>校准</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PREFLIGHT_SET_SENSOR_OFFSETS].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set sensor offsets</source>
      <translation>设置传感器偏移</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PREFLIGHT_UAVCAN].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>UAVCAN configure</source>
      <translation>UAVCAN 配置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PREFLIGHT_STORAGE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Store parameters</source>
      <translation>存储参数</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Reboot/Shutdown vehicle</source>
      <translation>重启/关闭载具</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_OVERRIDE_GOTO].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Override goto</source>
      <translation>覆写 goto</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_MISSION_START].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Mission start</source>
      <translation>任务开始</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_COMPONENT_ARM_DISARM].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Arm/Disarm</source>
      <translation>上锁/解锁</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_GET_HOME_POSITION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Get launch position</source>
      <translation>获取启动位置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_START_RX_PAIR].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Bind Spektrum receiver</source>
      <translation>绑定频谱接收机</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_GET_MESSAGE_INTERVAL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Get message interval</source>
      <translation>获取消息间隔</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_SET_MESSAGE_INTERVAL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set message interval</source>
      <translation>设置消息间隔</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Get capabilities</source>
      <translation>获取能力</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_SET_CAMERA_MODE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set camera modes</source>
      <translation>设置相机模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_SET_CAMERA_MODE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Set camera photo, video modes.</source>
      <translation>设置相机照片，视频模式。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_SET_CAMERA_MODE].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Take photos,Record video,Survey photo mode</source>
      <translation>拍照,录制视频,测绘照片模式</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_START_CAPTURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Start image capture</source>
      <translation>开始抓取图像</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_START_CAPTURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Start taking one or more photos.</source>
      <translation>开始拍摄一张或多张照片。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_START_CAPTURE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Interval</source>
      <translation>间隔</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_START_CAPTURE].param3.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Photo count</source>
      <translation>拍摄张数</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_STOP_CAPTURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stop image capture</source>
      <translation>停止抓取图像</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_IMAGE_STOP_CAPTURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stop taking photos.</source>
      <translation>停止拍照。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_TRIGGER_CONTROL].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Trigger control</source>
      <translation>触发器控制</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_VIDEO_START_CAPTURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Start video capture</source>
      <translation>开始录制视频</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_VIDEO_START_CAPTURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Start video capture.</source>
      <translation>开始录制视频。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_VIDEO_START_CAPTURE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Status Frequency</source>
      <translation>状态频率</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_VIDEO_STOP_CAPTURE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stop video capture</source>
      <translation>停止视频捕获。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_VIDEO_STOP_CAPTURE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Stop video capture.</source>
      <translation>停止视频捕获。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONTROL_HIGH_LATENCY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Control high latency link</source>
      <translation>Control high latency link</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PANORAMA_CREATE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Create panorama</source>
      <translation>创建全景</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_VTOL_TRANSITION].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>VTOL Transition</source>
      <translation>垂直起降转换</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_VTOL_TRANSITION].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Perform flight mode transition.</source>
      <translation>执行飞行模式转换。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_VTOL_TRANSITION].category, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>VTOL</source>
      <translation>垂直起降</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_VTOL_TRANSITION].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Multi Rotor,Fixed Wing</source>
      <translation>多旋翼,固定翼</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_GATE].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Condition Gate</source>
      <translation>条件门</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_GATE].description, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Delay mission state machine until gate has been reached.</source>
      <translation>延迟任务状态机直到到达门。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_GATE].param2.label, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Ignore Alt</source>
      <translation>忽略高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_CONDITION_GATE].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>False,True</source>
      <translation>否,是</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PAYLOAD_PREPARE_DEPLOY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Payload prepare deploy</source>
      <translation>有效载荷准备部署</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_PAYLOAD_CONTROL_DEPLOY].friendlyName, </extracomment>
      <location filename="../src/MissionManager/MavCmdInfoCommon.json"/>
      <source>Payload control deploy</source>
      <translation>载荷控制部署</translation>
    </message>
  </context>
  <context>
    <name>CameraSection.FactMetaData.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraAction].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Specify whether the camera should take photos or video</source>
      <translation>指定相机应拍照还是录像</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraAction].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>No change,Take photo,Take photos (time),Take photos (distance),Stop taking photos,Start recording video,Stop recording video</source>
      <translation>无变动,拍照,按时间拍照,按距离拍照,停止拍照,开始录像,停止录像</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraPhotoIntervalDistance].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Specify the distance between each photo</source>
      <translation>指定每张照片之间的距离</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraPhotoIntervalTime].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Specify the time between each photo</source>
      <translation>指定每张照片之间的时间</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[GimbalPitch].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Gimbal pitch rotation.</source>
      <translation>云台俯仰旋转。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[GimbalYaw].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Gimbal yaw rotation.</source>
      <translation>云台偏航旋转。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraMode].shortDesc, </extracomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Specify whether the camera should switch to Photo, Video or Survey mode</source>
      <translation>指定相机应切换到拍照、录像还是测绘模式</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[CameraMode].enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/MissionManager/CameraSection.FactMetaData.json"/>
      <source>Photo,Video,Survey</source>
      <translation>照片,视频,测绘</translation>
    </message>
  </context>
  <context>
    <name>Survey.SettingsGroup.json</name>
    <message>
      <extracomment>.QGC.MetaData.Facts[GridAngle].shortDesc, </extracomment>
      <location filename="../src/MissionManager/Survey.SettingsGroup.json"/>
      <source>Angle for parallel lines of grid.</source>
      <translation>网格平行线角度。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[FlyAlternateTransects].shortDesc, </extracomment>
      <location filename="../src/MissionManager/Survey.SettingsGroup.json"/>
      <source>Fly every other transect in each pass.</source>
      <translation>每次航过飞行隔一条航线条带。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[SplitConcavePolygons].shortDesc, </extracomment>
      <location filename="../src/MissionManager/Survey.SettingsGroup.json"/>
      <source>Split mission concave polygons into separate regular, convex polygons.</source>
      <translation>将任务中的凹多边形拆分为单独的规则凸多边形。</translation>
    </message>
  </context>
  <context>
    <name>APM-MavCmdInfoCommon.json</name>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_TAKEOFF].description, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Take off from the ground and ascend to specified altitude.</source>
      <translation>从地面起飞并升至特定高度。</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].description, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Takeoff to specified altitude.</source>
      <translation>起飞至指定高度</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_TAKEOFF].category, .mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].category, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>VTOL</source>
      <translation>垂直起降</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_NAV_VTOL_LAND].description, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Land using VTOL mode.</source>
      <translation>使用VTOL模式降落</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].param2.label, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Setting</source>
      <translation>设置</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_SET_RELAY].param2.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>On,Off</source>
      <translation>开启,关闭</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param1.label, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Pitch</source>
      <translation>俯仰</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param2.label, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Roll</source>
      <translation>滚转</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_MOUNT_CONTROL].param3.label, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Yaw</source>
      <translation>偏航</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].param1.label, </extracomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Enable</source>
      <translation>启用</translation>
    </message>
    <message>
      <extracomment>.mavCmdInfo[MAV_CMD_DO_FENCE_ENABLE].param1.enumStrings, </extracomment>
      <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>
      <location filename="../src/FirmwarePlugin/APM/APM-MavCmdInfoCommon.json"/>
      <source>Enable,Disable</source>
      <translation>启用/禁用</translation>
    </message>
  </context>
</TS>
