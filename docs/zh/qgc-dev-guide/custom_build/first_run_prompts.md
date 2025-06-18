# 首次运行提示

When QGC is started for the first time it prompts the user to specify some initial settings. At the time of writing this documentation those are:

- Unit Settings - What units does the user want to use for display.
- 离线载具设置 - 未连接车辆时用于创建计划的车辆信息。

The custom build architecure includes mechanisms for a custom build to both override the display of these prompts and/or create your own first run prompts.

## First Run Prompt Dialog

Each first run prompt is a simple dialog which can display ui to the user. Whether the specific dialog has already been show to the user or not is stored in a setting. Here is the code for the upstream first run prompt dialogs:

- [Units Settings](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirstRunPromptDialogs/UnitsFirstRunPrompt.qml)
- [离线载具设置](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirstRunPromptDialogs/OfflineVehicleFirstRunPrompt.qml)

## Standard First Run Prompt Dialogs

Each dialog has a unique ID associated with it. 当该对话框向用户显示ID已经被注册，所以它只会发生一次(除非您清除设置)。 The set of first run prompt which are included with upstream QGC are considered the "Standard" set. QGC gets the list of standard prompts to display from the `QGCCorePlugin::firstRunPromptStdIds` call.

```
    /// Returns the standard list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptStdIds(void);
```

You can override this method in your custom build if you want to hide some of those.

## Custom First Run Prompt Dialogs

Custom builds have the ability to create their own set of additional first run prompts as needed through the use of the following QGCCorePlugin method overrides:

```
    /// Returns the custom build list of first run prompt ids for possible display. Actual display is based on the
    /// current AppSettings::firstRunPromptIds value. The order of this list also determines the order the prompts
    /// will be displayed in.
    virtual QList<int> firstRunPromptCustomIds(void);
```

```
    /// Returns the resource which contains the specified first run prompt for display
    Q_INVOKABLE virtual QString firstRunPromptResource(int id);
```

Your QGCCorePlugin should override these two methods as well as provide static consts for the ids of your new first run prompts. Look at how the standard set is implemented for how to do this and take the same approach.

## Order Of Display

The set of first run prompts shown to the user are in the order returned by the `QGCCorePlugin::firstRunPromptStdIds` and `QGCCorePlugin::firstRunPromptCustomIds` with standard prompts shown before the custom prompts. 仅显示以前未显示给用户的提示。

## Always On Prompts

By setting the `markAsShownOnClose: false` property in your prompt ui implementation you can create a prompt which will show up each time QGC starts. This can be used for things like showing usage tips to your users. If you do this it is best to make sure that this is displayed last.
