#ifndef PLANCKLISTENER_H
#define PLANCKLISTENER_H

#include <QObject>
#include "QGCToolbox.h"
#include <mavlink.h>

class LinkInterface;

class PlanckListener : public QGCTool
{
    Q_OBJECT
public:
    PlanckListener(QGCApplication* app, QGCToolbox* toolbox);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

signals:
    void landingPadPosition(int lat, int lon);

public slots:
    void onMAVLinkMessage(LinkInterface* link, mavlink_message_t message);
};

#endif // PLANCKLISTENER_H
