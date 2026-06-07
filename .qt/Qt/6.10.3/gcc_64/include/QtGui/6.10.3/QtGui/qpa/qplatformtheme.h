// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMTHEME_H
#define QPLATFORMTHEME_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#if QT_CONFIG(shortcut)
#  include <QtGui/QKeySequence>
#endif

QT_BEGIN_NAMESPACE

class QIcon;
class QIconEngine;
class QMenu;
class QMenuBar;
class QPlatformMenuItem;
class QPlatformMenu;
class QPlatformMenuBar;
class QPlatformDialogHelper;
class QPlatformSystemTrayIcon;
class QPlatformThemePrivate;
class QVariant;
class QPalette;
class QFont;
class QPixmap;
class QSizeF;
class QFileInfo;

class Q_GUI_EXPORT QPlatformTheme
{
    Q_GADGET
    Q_DECLARE_PRIVATE(QPlatformTheme)

public:
    Q_DISABLE_COPY_MOVE(QPlatformTheme)

    enum ThemeHint {
        CursorFlashTime,
        KeyboardInputInterval,
        MouseDoubleClickInterval,
        StartDragDistance,
        StartDragTime,
        KeyboardAutoRepeatRate,
        PasswordMaskDelay,
        StartDragVelocity,
        TextCursorWidth,
        DropShadow,
        MaximumScrollBarDragDistance,
        ToolButtonStyle,
        ToolBarIconSize,
        ItemViewActivateItemOnSingleClick,
        SystemIconThemeName,
        SystemIconFallbackThemeName,
        IconThemeSearchPaths,
        StyleNames,
        WindowAutoPlacement,
        DialogButtonBoxLayout,
        DialogButtonBoxButtonsHaveIcons,
        UseFullScreenForPopupMenu,
        KeyboardScheme,
        UiEffects,
        SpellCheckUnderlineStyle,
        TabFocusBehavior,
        IconPixmapSizes,
        PasswordMaskCharacter,
        DialogSnapToDefaultButton,
        ContextMenuOnMouseRelease,
        MousePressAndHoldInterval,
        MouseDoubleClickDistance,
        WheelScrollLines,
        TouchDoubleTapDistance,
        ShowShortcutsInContextMenus,
        IconFallbackSearchPaths,
        MouseQuickSelectionThreshold,
        InteractiveResizeAcrossScreens,
        ShowDirectoriesFirst,
        PreselectFirstFileInDirectory,
        ButtonPressKeys,
        SetFocusOnTouchRelease,
        FlickStartDistance,
        FlickMaximumVelocity,
        FlickDeceleration,
        MenuBarFocusOnAltPressRelease,
        MouseCursorTheme,
        MouseCursorSize,
        UnderlineShortcut,
        ShowIconsInMenus,
        PreferFileIconFromTheme,
        MenuSelectionWraps,
        ScrollSingleStepDistance,
    };
    Q_ENUM(ThemeHint)

    enum DialogType {
        FileDialog,
        ColorDialog,
        FontDialog,
        MessageDialog
    };
    Q_ENUM(DialogType)

    enum Palette {
        SystemPalette,
        ToolTipPalette,
        ToolButtonPalette,
        ButtonPalette,
        CheckBoxPalette,
        RadioButtonPalette,
        HeaderPalette,
        ComboBoxPalette,
        ItemViewPalette,
        MessageBoxLabelPelette,
        MessageBoxLabelPalette = MessageBoxLabelPelette,
        TabBarPalette,
        LabelPalette,
        GroupBoxPalette,
        MenuPalette,
        MenuBarPalette,
        TextEditPalette,
        TextLineEditPalette,
        NPalettes
    };
    Q_ENUM(Palette)

    enum Font {
        SystemFont,
        MenuFont,
        MenuBarFont,
        MenuItemFont,
        MessageBoxFont,
        LabelFont,
        TipLabelFont,
        StatusBarFont,
        TitleBarFont,
        MdiSubWindowTitleFont,
        DockWidgetTitleFont,
        PushButtonFont,
        CheckBoxFont,
        RadioButtonFont,
        ToolButtonFont,
        ItemViewFont,
        ListViewFont,
        HeaderViewFont,
        ListBoxFont,
        ComboMenuItemFont,
        ComboLineEditFont,
        SmallFont,
        MiniFont,
        FixedFont,
        GroupBoxTitleFont,
        TabButtonFont,
        EditorFont,
        NFonts
    };
    Q_ENUM(Font)

    enum StandardPixmap {  // Keep in sync with QStyle::StandardPixmap
        TitleBarMenuButton,
        TitleBarMinButton,
        TitleBarMaxButton,
        TitleBarCloseButton,
        TitleBarNormalButton,
        TitleBarShadeButton,
        TitleBarUnshadeButton,
        TitleBarContextHelpButton,
        DockWidgetCloseButton,
        MessageBoxInformation,
        MessageBoxWarning,
        MessageBoxCritical,
        MessageBoxQuestion,
        DesktopIcon,
        TrashIcon,
        ComputerIcon,
        DriveFDIcon,
        DriveHDIcon,
        DriveCDIcon,
        DriveDVDIcon,
        DriveNetIcon,
        DirOpenIcon,
        DirClosedIcon,
        DirLinkIcon,
        DirLinkOpenIcon,
        FileIcon,
        FileLinkIcon,
        ToolBarHorizontalExtensionButton,
        ToolBarVerticalExtensionButton,
        FileDialogStart,
        FileDialogEnd,
        FileDialogToParent,
        FileDialogNewFolder,
        FileDialogDetailedView,
        FileDialogInfoView,
        FileDialogContentsView,
        FileDialogListView,
        FileDialogBack,
        DirIcon,
        DialogOkButton,
        DialogCancelButton,
        DialogHelpButton,
        DialogOpenButton,
        DialogSaveButton,
        DialogCloseButton,
        DialogApplyButton,
        DialogResetButton,
        DialogDiscardButton,
        DialogYesButton,
        DialogNoButton,
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        ArrowBack,
        ArrowForward,
        DirHomeIcon,
        CommandLink,
        VistaShield,
        BrowserReload,
        BrowserStop,
        MediaPlay,
        MediaStop,
        MediaPause,
        MediaSkipForward,
        MediaSkipBackward,
        MediaSeekForward,
        MediaSeekBackward,
        MediaVolume,
        MediaVolumeMuted,
        LineEditClearButton,
        DialogYesToAllButton,
        DialogNoToAllButton,
        DialogSaveAllButton,
        DialogAbortButton,
        DialogRetryButton,
        DialogIgnoreButton,
        RestoreDefaultsButton,
        TabCloseButton,
        NStandardPixmap, // assertion value for sync with QStyle::StandardPixmap

        // do not add any values below/greater than this
        CustomBase = 0xf0000000
    };
    Q_ENUM(StandardPixmap)

    enum KeyboardSchemes
    {
        WindowsKeyboardScheme,
        MacKeyboardScheme,
        X11KeyboardScheme,
        KdeKeyboardScheme,
        GnomeKeyboardScheme,
        CdeKeyboardScheme
    };
    Q_ENUM(KeyboardSchemes)

    enum UiEffect
    {
        GeneralUiEffect = 0x1,
        AnimateMenuUiEffect = 0x2,
        FadeMenuUiEffect = 0x4,
        AnimateComboUiEffect = 0x8,
        AnimateTooltipUiEffect = 0x10,
        FadeTooltipUiEffect = 0x20,
        AnimateToolBoxUiEffect = 0x40,
        HoverEffect = 0x80
    };
    Q_ENUM(UiEffect)

    enum IconOption {
        DontUseCustomDirectoryIcons = 0x01
    };
    Q_DECLARE_FLAGS(IconOptions, IconOption)

    explicit QPlatformTheme();
    virtual ~QPlatformTheme();

    virtual QPlatformMenuItem* createPlatformMenuItem() const;
    virtual QPlatformMenu* createPlatformMenu() const;
    virtual QPlatformMenuBar* createPlatformMenuBar() const;
    virtual void showPlatformMenuBar() {}

    virtual bool usePlatformNativeDialog(DialogType type) const;
    virtual QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const;

#ifndef QT_NO_SYSTEMTRAYICON
    virtual QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const;
#endif

    virtual Qt::ColorScheme colorScheme() const;

    virtual const QPalette *palette(Palette type = SystemPalette) const;

    virtual const QFont *font(Font type = SystemFont) const;

    virtual QVariant themeHint(ThemeHint hint) const;

    virtual QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const;
    virtual QIcon fileIcon(const QFileInfo &fileInfo,
                           QPlatformTheme::IconOptions iconOptions = { }) const;
    virtual QIconEngine *createIconEngine(const QString &iconName) const;

#if QT_CONFIG(shortcut)
    virtual QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const;
#endif

    virtual QString standardButtonText(int button) const;
#if QT_CONFIG(shortcut)
    virtual QKeySequence standardButtonShortcut(int button) const;
#endif
    virtual void requestColorScheme(Qt::ColorScheme scheme);
    virtual Qt::ContrastPreference contrastPreference() const;

    static QVariant defaultThemeHint(ThemeHint hint);
    static QString defaultStandardButtonText(int button);
    static QString removeMnemonics(const QString &original);
    QString name() const;

protected:
    explicit QPlatformTheme(QPlatformThemePrivate *priv);
    QScopedPointer<QPlatformThemePrivate> d_ptr;

private:
    friend class QPlatformThemeFactory;
};

QT_END_NAMESPACE

#endif // QPLATFORMTHEME_H
