// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef NEWFORMWIDGET_H
#define NEWFORMWIDGET_H

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

#include "shared_global_p.h"
#include "deviceprofile_p.h"

#include <QtDesigner/abstractnewformwidget.h>

#include <QtWidgets/qwidget.h>

#include <QtGui/qpixmap.h>

#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class QTreeWidgetItem;

namespace qdesigner_internal {

namespace Ui {
    class NewFormWidget;
}

class QDESIGNER_SHARED_EXPORT NewFormWidget : public QDesignerNewFormWidgetInterface
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(NewFormWidget)

public:
    using DeviceProfileList = QList<qdesigner_internal::DeviceProfile>;

    explicit NewFormWidget(QDesignerFormEditorInterface *core, QWidget *parentWidget);
    ~NewFormWidget() override;

    bool hasCurrentTemplate() const override;
    QString currentTemplate(QString *errorMessage = nullptr) override;

    // Convenience for implementing file dialogs with preview
    static QImage grabForm(QDesignerFormEditorInterface *core,
                           QIODevice &file,
                           const QString &workingDir,
                           const qdesigner_internal::DeviceProfile &dp);

private slots:
    void treeWidgetItemActivated(QTreeWidgetItem *item);
    void treeWidgetCurrentItemChanged(QTreeWidgetItem *current);
    void treeWidgetItemPressed(QTreeWidgetItem *item);
    void slotDeviceProfileIndexChanged(int idx);

private:
    QPixmap formPreviewPixmap(const QString &fileName) const;
    QPixmap formPreviewPixmap(QIODevice &file, const QString &workingDir = QString()) const;
    QPixmap formPreviewPixmap(const QTreeWidgetItem *item);

    void loadFrom(const QString &path, bool resourceFile, const QString &uiExtension,
                  const QString &selectedItem, QTreeWidgetItem *&selectedItemFound);
    void loadFrom(const QString &title, const QStringList &nameList,
                  const QString &selectedItem, QTreeWidgetItem *&selectedItemFound);

private:
    QString itemToTemplate(const QTreeWidgetItem *item, QString *errorMessage) const;
    QString currentTemplateI(QString *ptrToErrorMessage);

    QSize templateSize() const;
    void setTemplateSize(const QSize &s);
    int profileComboIndex() const;
    qdesigner_internal::DeviceProfile currentDeviceProfile() const;
    bool showCurrentItemPixmap();

    // Pixmap cache (item, profile combo index)
    using ItemPixmapCacheKey = std::pair<const QTreeWidgetItem *, int>;
    using ItemPixmapCache = QMap<ItemPixmapCacheKey, QPixmap>;
    ItemPixmapCache m_itemPixmapCache;

    QDesignerFormEditorInterface *m_core;
    Ui::NewFormWidget *m_ui;
    QTreeWidgetItem *m_currentItem;
    QTreeWidgetItem *m_acceptedItem;
    DeviceProfileList m_deviceProfiles;
};

}

QT_END_NAMESPACE

#endif // NEWFORMWIDGET_H
