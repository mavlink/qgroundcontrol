/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SysStatusManager_H
#define SysStatusManager_H

#include <QObject>

#include "QGCToolbox.h"

class SysStatusManager : public QGCTool
{
    Q_OBJECT

public:
    SysStatusManager  (QGCApplication* app, QGCToolbox* toolbox);
   ~SysStatusManager  ();

    Q_PROPERTY(QString boardTemputure READ boardTemputure WRITE setBoardTemputure NOTIFY boardTemputureChanged)

    QString boardTemputure() { return m_boardTemputure; }

    void setToolbox(QGCToolbox *toolbox);

public slots:
    void setBoardTemputure(const QString &boardTemputure);

signals:
    void boardTemputureChanged();

private:
    QString                m_boardTemputure;
};

#endif
