// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILEDIALOG_P_H
#define QFILEDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include "qfiledialog.h"
#include "private/qdialog_p.h"
#include "qplatformdefs.h"

#include <QtGui/private/qfilesystemmodel_p.h>
#include <qlistview.h>
#include <qtreeview.h>
#include <qcombobox.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qurl.h>
#include <qstackedwidget.h>
#include <qdialogbuttonbox.h>
#include <qabstractproxymodel.h>
#include <qfileiconprovider.h>
#if QT_CONFIG(completer)
#include <qcompleter.h>
#endif
#include <qpointer.h>
#include "qsidebar_p.h"
#if QT_CONFIG(fscompleter)
#include "qfscompleter_p.h"
#endif

#if defined (Q_OS_UNIX)
#include <unistd.h>
#endif

QT_REQUIRE_CONFIG(filedialog);

QT_BEGIN_NAMESPACE

class QFileDialogListView;
class QFileDialogTreeView;
class QFileDialogLineEdit;
class QGridLayout;
class QCompleter;
class QHBoxLayout;
class Ui_QFileDialog;
class QPlatformDialogHelper;

struct QFileDialogArgs
{
    QFileDialogArgs(const QUrl &url = {});

    QWidget *parent = nullptr;
    QString caption;
    QUrl directory;
    QString selection;
    QString filter;
    QFileDialog::FileMode mode = QFileDialog::AnyFile;
    QFileDialog::Options options = {};
};

class Q_WIDGETS_EXPORT QFileDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QFileDialog)

public:
    using PersistentModelIndexList = QList<QPersistentModelIndex>;

    struct HistoryItem
    {
        QString path;
        PersistentModelIndexList selection;
    };

    QFileDialogPrivate();

    QPlatformFileDialogHelper *platformFileDialogHelper() const
        { return static_cast<QPlatformFileDialogHelper *>(platformHelper()); }

    void createToolButtons();
    void createMenuActions();
    void createWidgets();

    void init(const QFileDialogArgs &args);
    bool itemViewKeyboardEvent(QKeyEvent *event);
    QString getEnvironmentVariable(const QString &string);
    QStringList typedFiles() const;
    QList<QUrl> userSelectedFiles() const;
    QStringList addDefaultSuffixToFiles(const QStringList &filesToFix) const;
    QList<QUrl> addDefaultSuffixToUrls(const QList<QUrl> &urlsToFix) const;
    bool removeDirectory(const QString &path);
    void setLabelTextControl(QFileDialog::DialogLabel label, const QString &text);
    inline void updateLookInLabel();
    inline void updateFileNameLabel();
    inline void updateFileTypeLabel();
    void updateOkButtonText(bool saveAsOnFolder = false);
    void updateCancelButtonText();

    inline QModelIndex mapToSource(const QModelIndex &index) const;
    inline QModelIndex mapFromSource(const QModelIndex &index) const;
    inline QModelIndex rootIndex() const;
    inline void setRootIndex(const QModelIndex &index) const;
    inline QModelIndex select(const QModelIndex &index) const;
    inline QString rootPath() const;

    QLineEdit *lineEdit() const;

    static long maxNameLength(const QString &path);

    QString basename(const QString &path) const
    {
        const qsizetype separator = QDir::toNativeSeparators(path).lastIndexOf(QDir::separator());
        if (separator != -1)
            return path.mid(separator + 1);
        return path;
    }

    QDir::Filters filterForMode(QDir::Filters filters) const
    {
        filters |= QDir::Drives | QDir::AllDirs | QDir::Dirs | QDir::Files;
        if (q_func()->testOption(QFileDialog::ShowDirsOnly))
            filters &= ~QDir::Files;
        return filters;
    }

    QAbstractItemView *currentView() const;

    static inline QString toInternal(const QString &path)
    {
#if defined(Q_OS_WIN)
        QString n(path);
        n.replace(u'\\', u'/');
        return n;
#else // the compile should optimize away this
        return path;
#endif
    }

#if QT_CONFIG(settings)
    void saveSettings();
    bool restoreFromSettings();
#endif

    bool restoreWidgetState(QStringList &history, int splitterPosition);
    static void setLastVisitedDirectory(const QUrl &dir);
    void retranslateWindowTitle();
    void retranslateStrings();
    void emitFilesSelected(const QStringList &files);
    void saveHistorySelection();

    void goHome();
    void pathChanged(const QString &);
    void navigate(HistoryItem &);
    void navigateBackward();
    void navigateForward();
    void navigateToParent();
    void createDirectory();
    void showListView();
    void showDetailsView();
    void showContextMenu(const QPoint &position);
    void renameCurrent();
    void deleteCurrent();
    void showHidden();
    void showHeader(QAction *);
    void updateOkButton();
    void currentChanged(const QModelIndex &index);
    void enterDirectory(const QModelIndex &index);
    void emitUrlSelected(const QUrl &file);
    void emitUrlsSelected(const QList<QUrl> &files);
    void nativeCurrentChanged(const QUrl &file);
    void nativeEnterDirectory(const QUrl &directory);
    void goToDirectory(const QString &);
    void useNameFilter(int index);
    void selectionChanged();
    void goToUrl(const QUrl &url);
    void autoCompleteFileName(const QString &);
    void rowsInserted(const QModelIndex & parent);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);

    // layout
#if QT_CONFIG(proxymodel)
    QAbstractProxyModel *proxyModel;
#endif

    // data
    QStringList watching;
    QFileSystemModel *model;

#if QT_CONFIG(fscompleter)
    QFSCompleter *completer;
#endif //QT_CONFIG(fscompleter)

    QString setWindowTitle;

    QList<HistoryItem> currentHistory;
    int currentHistoryLocation;

    QAction *renameAction;
    QAction *deleteAction;
    QAction *showHiddenAction;
    QAction *newFolderAction;

    bool useDefaultCaption;

    // setVisible_sys returns true if it ends up showing a native
    // dialog. Returning false means that a non-native dialog must be
    // used instead.
    bool canBeNativeDialog() const override;
    void setVisible(bool visible) override;
    inline bool usingWidgets() const;

    inline void setDirectory_sys(const QUrl &directory);
    inline QUrl directory_sys() const;
    inline void selectFile_sys(const QUrl &filename);
    inline QList<QUrl> selectedFiles_sys() const;
    inline void setFilter_sys();
    inline void selectMimeTypeFilter_sys(const QString &filter);
    inline QString selectedMimeTypeFilter_sys() const;
    inline void selectNameFilter_sys(const QString &filter);
    inline QString selectedNameFilter_sys() const;
    //////////////////////////////////////////////

    QScopedPointer<Ui_QFileDialog> qFileDialogUi;

    QString acceptLabel;

    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
    QByteArray signalToDisconnectOnClose;

    QSharedPointer<QFileDialogOptions> options;

    // Memory of what was read from QSettings in restoreState() in case widgets are not used
    QByteArray splitterState;
    QByteArray headerData;
    QList<QUrl> sidebarUrls;
    QFileIconProvider defaultIconProvider;

    ~QFileDialogPrivate();

private:
    virtual void initHelper(QPlatformDialogHelper *) override;
    virtual void helperPrepareShow(QPlatformDialogHelper *) override;
    virtual void helperDone(QDialog::DialogCode, QPlatformDialogHelper *) override;

    void itemNotFound(const QString &fileName, QFileDialog::FileMode mode);
    bool itemAlreadyExists(const QString &fileName);
    Q_DISABLE_COPY_MOVE(QFileDialogPrivate)
};

class QFileDialogLineEdit : public QLineEdit
{
public:
    QFileDialogLineEdit(QWidget *parent = nullptr) : QLineEdit(parent), d_ptr(nullptr){}
    void setFileDialogPrivate(QFileDialogPrivate *d_pointer) {d_ptr = d_pointer; }
    void keyPressEvent(QKeyEvent *e) override;
    bool hideOnEsc;
private:
    QFileDialogPrivate *d_ptr;
};

class QFileDialogComboBox : public QComboBox
{
public:
    QFileDialogComboBox(QWidget *parent = nullptr) :
        QComboBox(parent), urlModel(nullptr), d_ptr(nullptr) {}
    void setFileDialogPrivate(QFileDialogPrivate *d_pointer);
    void showPopup() override;
    void setHistory(const QStringList &paths);
    QStringList history() const { return m_history; }
    void paintEvent(QPaintEvent *) override;

private:
    QUrlModel *urlModel;
    QFileDialogPrivate *d_ptr;
    QStringList m_history;
};

class QFileDialogListView : public QListView
{
public:
    QFileDialogListView(QWidget *parent = nullptr) : QListView(parent), d_ptr(nullptr) {}
    void setFileDialogPrivate(QFileDialogPrivate *d_pointer);
    QSize sizeHint() const override;
protected:
    void keyPressEvent(QKeyEvent *e) override;
private:
    QFileDialogPrivate *d_ptr;
};

class QFileDialogTreeView : public QTreeView
{
public:
    QFileDialogTreeView(QWidget *parent) : QTreeView(parent), d_ptr(nullptr) {}
    void setFileDialogPrivate(QFileDialogPrivate *d_pointer);
    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *e) override;
private:
    QFileDialogPrivate *d_ptr;
};

QModelIndex QFileDialogPrivate::mapToSource(const QModelIndex &index) const {
#if QT_CONFIG(proxymodel)
    return proxyModel ? proxyModel->mapToSource(index) : index;
#else
    return index;
#endif
}
QModelIndex QFileDialogPrivate::mapFromSource(const QModelIndex &index) const {
#if QT_CONFIG(proxymodel)
    return proxyModel ? proxyModel->mapFromSource(index) : index;
#else
    return index;
#endif
}

QString QFileDialogPrivate::rootPath() const
{
    return (model ? model->rootPath() : QStringLiteral("/"));
}

void QFileDialogPrivate::setDirectory_sys(const QUrl &directory)
{
    QPlatformFileDialogHelper *helper = platformFileDialogHelper();

    if (!helper)
        return;

    if (helper->isSupportedUrl(directory))
        helper->setDirectory(directory);
}

QUrl QFileDialogPrivate::directory_sys() const
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        return helper->directory();
    return QUrl();
}

void QFileDialogPrivate::selectFile_sys(const QUrl &filename)
{
    QPlatformFileDialogHelper *helper = platformFileDialogHelper();

    if (!helper)
        return;

    if (helper->isSupportedUrl(filename))
        helper->selectFile(filename);
}

QList<QUrl> QFileDialogPrivate::selectedFiles_sys() const
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        return helper->selectedFiles();
    return QList<QUrl>();
}

void QFileDialogPrivate::setFilter_sys()
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        helper->setFilter();
}

void QFileDialogPrivate::selectMimeTypeFilter_sys(const QString &filter)
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        helper->selectMimeTypeFilter(filter);
}

QString QFileDialogPrivate::selectedMimeTypeFilter_sys() const
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        return helper->selectedMimeTypeFilter();

    return QString();
}

void QFileDialogPrivate::selectNameFilter_sys(const QString &filter)
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        helper->selectNameFilter(filter);
}

QString QFileDialogPrivate::selectedNameFilter_sys() const
{
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        return helper->selectedNameFilter();
    return QString();
}

QT_END_NAMESPACE

#endif // QFILEDIALOG_P_H
