#pragma once

#include "QGCState.h"

#include <QString>

/// Display an application message to the user

class ShowAppMessageState : public QGCState
{
    Q_OBJECT

public:
    ShowAppMessageState(QState* parentState, const QString& appMessage);

private:
    QString _appMessage;
};
