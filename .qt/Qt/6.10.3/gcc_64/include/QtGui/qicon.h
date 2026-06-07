// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QICON_H
#define QICON_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qsize.h>
#include <QtCore/qlist.h>
#include <QtGui/qpixmap.h>

QT_BEGIN_NAMESPACE


class QIconPrivate;
class QIconEngine;
class QPainter;

class Q_GUI_EXPORT QIcon
{
public:
    enum Mode { Normal, Disabled, Active, Selected };
    enum State { On, Off };

    enum class ThemeIcon {
        AddressBookNew,
        ApplicationExit,
        AppointmentNew,
        CallStart,
        CallStop,
        ContactNew,
        DocumentNew,
        DocumentOpen,
        DocumentOpenRecent,
        DocumentPageSetup,
        DocumentPrint,
        DocumentPrintPreview,
        DocumentProperties,
        DocumentRevert,
        DocumentSave,
        DocumentSaveAs,
        DocumentSend,
        EditClear,
        EditCopy,
        EditCut,
        EditDelete,
        EditFind,
        EditPaste,
        EditRedo,
        EditSelectAll,
        EditUndo,
        FolderNew,
        FormatIndentLess,
        FormatIndentMore,
        FormatJustifyCenter,
        FormatJustifyFill,
        FormatJustifyLeft,
        FormatJustifyRight,
        FormatTextDirectionLtr,
        FormatTextDirectionRtl,
        FormatTextBold,
        FormatTextItalic,
        FormatTextUnderline,
        FormatTextStrikethrough,
        GoDown,
        GoHome,
        GoNext,
        GoPrevious,
        GoUp,
        HelpAbout,
        HelpFaq,
        InsertImage,
        InsertLink,
        InsertText,
        ListAdd,
        ListRemove,
        MailForward,
        MailMarkImportant,
        MailMarkRead,
        MailMarkUnread,
        MailMessageNew,
        MailReplyAll,
        MailReplySender,
        MailSend,
        MediaEject,
        MediaPlaybackPause,
        MediaPlaybackStart,
        MediaPlaybackStop,
        MediaRecord,
        MediaSeekBackward,
        MediaSeekForward,
        MediaSkipBackward,
        MediaSkipForward,
        ObjectRotateLeft,
        ObjectRotateRight,
        ProcessStop,
        SystemLockScreen,
        SystemLogOut,
        SystemSearch,
        SystemReboot,
        SystemShutdown,
        ToolsCheckSpelling,
        ViewFullscreen,
        ViewRefresh,
        ViewRestore,
        WindowClose,
        WindowNew,
        ZoomFitBest,
        ZoomIn,
        ZoomOut,

        AudioCard,
        AudioInputMicrophone,
        Battery,
        CameraPhoto,
        CameraVideo,
        CameraWeb,
        Computer,
        DriveHarddisk,
        DriveOptical,
        InputGaming,
        InputKeyboard,
        InputMouse,
        InputTablet,
        MediaFlash,
        MediaOptical,
        MediaTape,
        MultimediaPlayer,
        NetworkWired,
        NetworkWireless,
        Phone,
        Printer,
        Scanner,
        VideoDisplay,

        AppointmentMissed,
        AppointmentSoon,
        AudioVolumeHigh,
        AudioVolumeLow,
        AudioVolumeMedium,
        AudioVolumeMuted,
        BatteryCaution,
        BatteryLow,
        DialogError,
        DialogInformation,
        DialogPassword,
        DialogQuestion,
        DialogWarning,
        FolderDragAccept,
        FolderOpen,
        FolderVisiting,
        ImageLoading,
        ImageMissing,
        MailAttachment,
        MailUnread,
        MailRead,
        MailReplied,
        MediaPlaylistRepeat,
        MediaPlaylistShuffle,
        NetworkOffline,
        PrinterPrinting,
        SecurityHigh,
        SecurityLow,
        SoftwareUpdateAvailable,
        SoftwareUpdateUrgent,
        SyncError,
        SyncSynchronizing,
        UserAvailable,
        UserOffline,
        WeatherClear,
        WeatherClearNight,
        WeatherFewClouds,
        WeatherFewCloudsNight,
        WeatherFog,
        WeatherShowers,
        WeatherSnow,
        WeatherStorm,

        NThemeIcons
    };

    QIcon() noexcept;
    QIcon(const QPixmap &pixmap);
    QIcon(const QIcon &other);
    QIcon(QIcon &&other) noexcept
        : d(std::exchange(other.d, nullptr))
    {}
    explicit QIcon(const QString &fileName); // file or resource name
    explicit QIcon(QIconEngine *engine);
    ~QIcon();
    QIcon &operator=(const QIcon &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QIcon)
    inline void swap(QIcon &other) noexcept
    { qt_ptr_swap(d, other.d); }
    bool operator==(const QIcon &) const = delete;
    bool operator!=(const QIcon &) const = delete;

    operator QVariant() const;

    QPixmap pixmap(const QSize &size, Mode mode = Normal, State state = Off) const;
    inline QPixmap pixmap(int w, int h, Mode mode = Normal, State state = Off) const
        { return pixmap(QSize(w, h), mode, state); }
    inline QPixmap pixmap(int extent, Mode mode = Normal, State state = Off) const
        { return pixmap(QSize(extent, extent), mode, state); }
    QPixmap pixmap(const QSize &size, qreal devicePixelRatio, Mode mode = Normal, State state = Off) const;
#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_VERSION_X_6_0("Use pixmap(size, devicePixelRatio) instead")
    QPixmap pixmap(QWindow *window, const QSize &size, Mode mode = Normal, State state = Off) const;
#endif

    QSize actualSize(const QSize &size, Mode mode = Normal, State state = Off) const;
#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_VERSION_X_6_0("Use actualSize(size) instead")
    QSize actualSize(QWindow *window, const QSize &size, Mode mode = Normal, State state = Off) const;
#endif

    QString name() const;

    void paint(QPainter *painter, const QRect &rect, Qt::Alignment alignment = Qt::AlignCenter, Mode mode = Normal, State state = Off) const;
    inline void paint(QPainter *painter, int x, int y, int w, int h, Qt::Alignment alignment = Qt::AlignCenter, Mode mode = Normal, State state = Off) const
        { paint(painter, QRect(x, y, w, h), alignment, mode, state); }

    bool isNull() const;
    bool isDetached() const;
    void detach();

    qint64 cacheKey() const;

    void addPixmap(const QPixmap &pixmap, Mode mode = Normal, State state = Off);
    void addFile(const QString &fileName, const QSize &size = QSize(), Mode mode = Normal, State state = Off);

    QList<QSize> availableSizes(Mode mode = Normal, State state = Off) const;

    void setIsMask(bool isMask);
    bool isMask() const;

    static QIcon fromTheme(const QString &name);
    static QIcon fromTheme(const QString &name, const QIcon &fallback);
    static bool hasThemeIcon(const QString &name);

    static QIcon fromTheme(ThemeIcon icon);
    static QIcon fromTheme(ThemeIcon icon, const QIcon &fallback);
    static bool hasThemeIcon(ThemeIcon icon);

    static QStringList themeSearchPaths();
    static void setThemeSearchPaths(const QStringList &searchpath);

    static QStringList fallbackSearchPaths();
    static void setFallbackSearchPaths(const QStringList &paths);

    static QString themeName();
    static void setThemeName(const QString &path);

    static QString fallbackThemeName();
    static void setFallbackThemeName(const QString &name);

private:
    QIconPrivate *d;
#if !defined(QT_NO_DATASTREAM)
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QIcon &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QIcon &);
#endif

    friend class QIconPrivate;

public:
    typedef QIconPrivate * DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QIcon)

#if !defined(QT_NO_DATASTREAM)
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QIcon &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QIcon &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QIcon &);
#endif

Q_GUI_EXPORT QString qt_findAtNxFile(const QString &baseFileName, qreal targetDevicePixelRatio,
                                     qreal *sourceDevicePixelRatio = nullptr);

QT_END_NAMESPACE

#endif // QICON_H
