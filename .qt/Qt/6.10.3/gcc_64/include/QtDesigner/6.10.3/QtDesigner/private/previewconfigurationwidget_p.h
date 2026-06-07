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

#ifndef PREVIEWCONFIGURATIONWIDGET_H
#define PREVIEWCONFIGURATIONWIDGET_H

#include "shared_global_p.h"

#include <QtWidgets/qgroupbox.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerSettingsInterface;

namespace qdesigner_internal {

// ----------- PreviewConfigurationWidget: Widget to edit the preview configuration.

class QDESIGNER_SHARED_EXPORT PreviewConfigurationWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit PreviewConfigurationWidget(QDesignerFormEditorInterface *core,
                                        QWidget *parent = nullptr);
    ~PreviewConfigurationWidget() override;
    void saveState();

private slots:
    void slotEditAppStyleSheet();
    void slotDeleteSkinEntry();
    void slotSkinChanged(int);

private:
    class PreviewConfigurationWidgetPrivate;
    PreviewConfigurationWidgetPrivate *m_impl;

    PreviewConfigurationWidget(const PreviewConfigurationWidget &other);
    PreviewConfigurationWidget &operator =(const PreviewConfigurationWidget &other);
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // PREVIEWCONFIGURATIONWIDGET_H
