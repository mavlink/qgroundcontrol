// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMESSAGEBOX_H
#define QMESSAGEBOX_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qdialogbuttonbox.h>

QT_REQUIRE_CONFIG(messagebox);

QT_BEGIN_NAMESPACE

class QAnyStringView;
class QLabel;
class QMessageBoxPrivate;
class QAbstractButton;
class QCheckBox;

class Q_WIDGETS_EXPORT QMessageBox : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(Icon icon READ icon WRITE setIcon)
    Q_PROPERTY(QPixmap iconPixmap READ iconPixmap WRITE setIconPixmap)
    Q_PROPERTY(Qt::TextFormat textFormat READ textFormat WRITE setTextFormat)
    Q_PROPERTY(StandardButtons standardButtons READ standardButtons WRITE setStandardButtons)
#if QT_CONFIG(textedit)
    Q_PROPERTY(QString detailedText READ detailedText WRITE setDetailedText)
#endif
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText)
    Q_PROPERTY(Qt::TextInteractionFlags textInteractionFlags READ textInteractionFlags
               WRITE setTextInteractionFlags)
    Q_PROPERTY(Options options READ options WRITE setOptions)
public:
    // Keep in sync with MessageBoxOption in qplatformdialoghelper.h
    enum class Option {
        DontUseNativeDialog = 0x00000001
    };
    Q_FLAG(Option)

    enum Icon {
        // keep this in sync with QMessageDialogOptions::StandardIcon
        NoIcon = 0,
        Information = 1,
        Warning = 2,
        Critical = 3,
        Question = 4
    };
    Q_ENUM(Icon)

    enum ButtonRole {
        // keep this in sync with QDialogButtonBox::ButtonRole and QPlatformDialogHelper::ButtonRole
        InvalidRole = -1,
        AcceptRole,
        RejectRole,
        DestructiveRole,
        ActionRole,
        HelpRole,
        YesRole,
        NoRole,
        ResetRole,
        ApplyRole,

        NRoles
    };
    Q_ENUM(ButtonRole)

    enum StandardButton {
        // keep this in sync with QDialogButtonBox::StandardButton and QPlatformDialogHelper::StandardButton
        NoButton           = 0x00000000,
        Ok                 = 0x00000400,
        Save               = 0x00000800,
        SaveAll            = 0x00001000,
        Open               = 0x00002000,
        Yes                = 0x00004000,
        YesToAll           = 0x00008000,
        No                 = 0x00010000,
        NoToAll            = 0x00020000,
        Abort              = 0x00040000,
        Retry              = 0x00080000,
        Ignore             = 0x00100000,
        Close              = 0x00200000,
        Cancel             = 0x00400000,
        Discard            = 0x00800000,
        Help               = 0x01000000,
        Apply              = 0x02000000,
        Reset              = 0x04000000,
        RestoreDefaults    = 0x08000000,

        FirstButton        = Ok,                // internal
        LastButton         = RestoreDefaults,   // internal

        YesAll             = YesToAll,          // obsolete
        NoAll              = NoToAll,           // obsolete

        Default            = 0x00000100,        // obsolete
        Escape             = 0x00000200,        // obsolete
        FlagMask           = 0x00000300,        // obsolete
        ButtonMask         = ~FlagMask          // obsolete
    };
    Q_ENUM(StandardButton)

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    typedef StandardButton Button;
#endif
    Q_DECLARE_FLAGS(Options, Option)
    Q_DECLARE_FLAGS(StandardButtons, StandardButton)

    Q_FLAG(StandardButtons)

    explicit QMessageBox(QWidget *parent = nullptr);
    QMessageBox(Icon icon, const QString &title, const QString &text,
                StandardButtons buttons = NoButton, QWidget *parent = nullptr,
                Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    ~QMessageBox();

    void addButton(QAbstractButton *button, ButtonRole role);
    QPushButton *addButton(const QString &text, ButtonRole role);
    QPushButton *addButton(StandardButton button);
    void removeButton(QAbstractButton *button);

    using QDialog::open;
    void open(QObject *receiver, const char *member);

    QList<QAbstractButton *> buttons() const;
    ButtonRole buttonRole(QAbstractButton *button) const;

    void setStandardButtons(StandardButtons buttons);
    StandardButtons standardButtons() const;
    StandardButton standardButton(QAbstractButton *button) const;
    QAbstractButton *button(StandardButton which) const;

    QPushButton *defaultButton() const;
    void setDefaultButton(QPushButton *button);
    void setDefaultButton(StandardButton button);

    QAbstractButton *escapeButton() const;
    void setEscapeButton(QAbstractButton *button);
    void setEscapeButton(StandardButton button);

    QAbstractButton *clickedButton() const;

    QString text() const;
    void setText(const QString &text);

    Icon icon() const;
    void setIcon(Icon);

    QPixmap iconPixmap() const;
    void setIconPixmap(const QPixmap &pixmap);

    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat format);

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    void setCheckBox(QCheckBox *cb);
    QCheckBox* checkBox() const;

    void setOption(Option option, bool on = true);
    bool testOption(Option option) const;
    void setOptions(Options options);
    Options options() const;

    static StandardButton information(QWidget *parent, const QString &title,
         const QString &text, StandardButtons buttons = Ok,
         StandardButton defaultButton = NoButton);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) // needed as long as we have int overloads
    inline static StandardButton information(QWidget *parent, const QString &title,
                                  const QString& text,
                                  StandardButton button0, StandardButton button1 = NoButton)
    { return information(parent, title, text, StandardButtons(button0), button1); }
#endif

    static StandardButton question(QWidget *parent, const QString &title,
         const QString &text, StandardButtons buttons = StandardButtons(Yes | No),
         StandardButton defaultButton = NoButton);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    inline static int question(QWidget *parent, const QString &title,
                               const QString& text,
                               StandardButton button0, StandardButton button1)
    { return question(parent, title, text, StandardButtons(button0), button1); }
#endif

    static StandardButton warning(QWidget *parent, const QString &title,
         const QString &text, StandardButtons buttons = Ok,
         StandardButton defaultButton = NoButton);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    inline static int warning(QWidget *parent, const QString &title,
                              const QString& text,
                              StandardButton button0, StandardButton button1)
    { return warning(parent, title, text, StandardButtons(button0), button1); }
#endif

    static StandardButton critical(QWidget *parent, const QString &title,
         const QString &text, StandardButtons buttons = Ok,
         StandardButton defaultButton = NoButton);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    inline static int critical(QWidget *parent, const QString &title,
                               const QString& text,
                               StandardButton button0, StandardButton button1)
    { return critical(parent, title, text, StandardButtons(button0), button1); }
#endif

    static void about(QWidget *parent, const QString &title, const QString &text);
    static void aboutQt(QWidget *parent, const QString &title = QString());

#if QT_DEPRECATED_SINCE(6,2)
    // the following functions are obsolete:
    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    QMessageBox(const QString &title, const QString &text, Icon icon,
                  int button0, int button1, int button2,
                  QWidget *parent = nullptr,
                  Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int information(QWidget *parent, const QString &title,
                           const QString& text,
                           int button0, int button1 = 0, int button2 = 0);
    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int information(QWidget *parent, const QString &title,
                           const QString& text,
                           const QString& button0Text,
                           const QString& button1Text = QString(),
                           const QString& button2Text = QString(),
                           int defaultButtonNumber = 0,
                           int escapeButtonNumber = -1);

    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int question(QWidget *parent, const QString &title,
                        const QString& text,
                        int button0, int button1 = 0, int button2 = 0);
    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int question(QWidget *parent, const QString &title,
                        const QString& text,
                        const QString& button0Text,
                        const QString& button1Text = QString(),
                        const QString& button2Text = QString(),
                        int defaultButtonNumber = 0,
                        int escapeButtonNumber = -1);

    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int warning(QWidget *parent, const QString &title,
                       const QString& text,
                       int button0, int button1, int button2 = 0);
    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int warning(QWidget *parent, const QString &title,
                       const QString& text,
                       const QString& button0Text,
                       const QString& button1Text = QString(),
                       const QString& button2Text = QString(),
                       int defaultButtonNumber = 0,
                       int escapeButtonNumber = -1);

    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int critical(QWidget *parent, const QString &title,
                        const QString& text,
                        int button0, int button1, int button2 = 0);
    QT_DEPRECATED_VERSION_X_6_2("Use the overload taking StandardButtons instead.")
    static int critical(QWidget *parent, const QString &title,
                        const QString& text,
                        const QString& button0Text,
                        const QString& button1Text = QString(),
                        const QString& button2Text = QString(),
                        int defaultButtonNumber = 0,
                        int escapeButtonNumber = -1);

    QT_DEPRECATED_VERSION_X_6_2("Use button() and QPushButton::text() instead.")
    QString buttonText(int button) const;
    QT_DEPRECATED_VERSION_X_6_2("Use addButton() instead.")
    void setButtonText(int button, const QString &text);
#endif

    QString informativeText() const;
    void setInformativeText(const QString &text);

#if QT_CONFIG(textedit)
    QString detailedText() const;
    void setDetailedText(const QString &text);
#endif

    void setWindowTitle(const QString &title);
    void setWindowModality(Qt::WindowModality windowModality);

#if QT_DEPRECATED_SINCE(6,2)
    QT_DEPRECATED_VERSION_X_6_2("Use QStyle::standardIcon() instead.")
    static QPixmap standardIcon(Icon icon);
#endif

Q_SIGNALS:
    void buttonClicked(QAbstractButton *button);

#ifdef Q_QDOC
public Q_SLOTS:
    int exec() override;
#endif

protected:
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    Q_DISABLE_COPY(QMessageBox)
    Q_DECLARE_PRIVATE(QMessageBox)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMessageBox::StandardButtons)

Q_WIDGETS_EXPORT void qRequireVersion(int argc, char *argv[], QAnyStringView req);

#define QT_REQUIRE_VERSION(argc, argv, str) qRequireVersion(argc, argv, str);

QT_END_NAMESPACE

#endif // QMESSAGEBOX_H
