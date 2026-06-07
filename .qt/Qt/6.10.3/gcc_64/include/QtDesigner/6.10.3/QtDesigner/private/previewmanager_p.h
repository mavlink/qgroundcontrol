// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef PREVIEWMANAGER_H
#define PREVIEWMANAGER_H

#include "shared_global_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;
class QWidget;
class QPixmap;
class QAction;
class QActionGroup;
class QMenu;
class QWidget;
class QDesignerSettingsInterface;

namespace qdesigner_internal {

// ----------- PreviewConfiguration

class PreviewConfigurationData;

class QDESIGNER_SHARED_EXPORT PreviewConfiguration {
public:
    PreviewConfiguration();
    explicit PreviewConfiguration(const QString &style,
                                  const QString &applicationStyleSheet = QString(),
                                  const QString &deviceSkin = QString());

    PreviewConfiguration(const PreviewConfiguration&);
    PreviewConfiguration& operator=(const PreviewConfiguration&);
    ~PreviewConfiguration();

    QString style() const;
    void setStyle(const QString &);

    // Style sheet to prepend (to simulate the effect od QApplication::setSyleSheet()).
    QString applicationStyleSheet() const;
    void setApplicationStyleSheet(const QString &);

    QString deviceSkin() const;
    void setDeviceSkin(const QString &);

    void clear();
    void toSettings(const QString &prefix, QDesignerSettingsInterface *settings) const;
    void fromSettings(const QString &prefix, const QDesignerSettingsInterface *settings);

private:
    QSharedDataPointer<PreviewConfigurationData> m_d;
};

QDESIGNER_SHARED_EXPORT bool operator<(const PreviewConfiguration &pc1, const PreviewConfiguration &pc2);
QDESIGNER_SHARED_EXPORT bool operator==(const PreviewConfiguration &pc1, const PreviewConfiguration &pc2);
QDESIGNER_SHARED_EXPORT bool operator!=(const PreviewConfiguration &pc1, const PreviewConfiguration &pc2);

// ----------- Preview window manager.
// Maintains a list of preview widgets with their associated form windows and configuration.

class PreviewManagerPrivate;

class QDESIGNER_SHARED_EXPORT PreviewManager : public QObject
{
    Q_OBJECT
public:

    enum PreviewMode {
        // Modal preview. Do not use on Macs as dialogs would have no close button
        ApplicationModalPreview,
        // Non modal previewing of one form in different configurations (closes if form window changes)
        SingleFormNonModalPreview,
        // Non modal previewing of several forms in different configurations
        MultipleFormNonModalPreview };

    explicit PreviewManager(PreviewMode mode, QObject *parent);
    ~PreviewManager() override;

    // Show preview. Raise existing preview window if there is one with a matching
    // configuration, else create a new preview.
    QWidget *showPreview(const QDesignerFormWindowInterface *, const PreviewConfiguration &pc, int deviceProfileIndex /*=-1*/, QString *errorMessage);
    // Convenience that creates a preview using a configuration taken from the settings.
    QWidget *showPreview(const QDesignerFormWindowInterface *, const QString &style, int deviceProfileIndex /*=-1*/, QString *errorMessage);
    QWidget *showPreview(const QDesignerFormWindowInterface *, const QString &style, QString *errorMessage);

    int previewCount() const;

    // Create a pixmap for printing.
    QPixmap createPreviewPixmap(const QDesignerFormWindowInterface *fw, const PreviewConfiguration &pc, int deviceProfileIndex /*=-1*/, QString *errorMessage);
    // Convenience that creates a pixmap using a configuration taken from the settings.
    QPixmap createPreviewPixmap(const QDesignerFormWindowInterface *fw, const QString &style, int deviceProfileIndex /*=-1*/, QString *errorMessage);
    QPixmap createPreviewPixmap(const QDesignerFormWindowInterface *fw, const QString &style, QString *errorMessage);

    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void closeAllPreviews();

signals:
    void firstPreviewOpened();
    void lastPreviewClosed();

private slots:
    void slotZoomChanged(int);

private:

    virtual Qt::WindowFlags previewWindowFlags(const QWidget *widget) const;
    virtual QWidget *createDeviceSkinContainer(const QDesignerFormWindowInterface *) const;

    QWidget *raise(const QDesignerFormWindowInterface *, const PreviewConfiguration &pc);
    QWidget *createPreview(const QDesignerFormWindowInterface *,
                           const PreviewConfiguration &pc,
                           int deviceProfileIndex /* = -1 */,
                           QString *errorMessage,
                           /*Disabled by default, <0 */
                           int initialZoom = -1);

    void updatePreviewClosed(QWidget *w);

    PreviewManagerPrivate *d;

    PreviewManager(const PreviewManager &other);
    PreviewManager &operator =(const PreviewManager &other);
};
}

QT_END_NAMESPACE

#endif // PREVIEWMANAGER_H
