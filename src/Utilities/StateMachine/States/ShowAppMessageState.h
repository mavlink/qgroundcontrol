#pragma once

#include "QGCState.h"

#include <QtCore/QString>

/// Display an application message to the user

class ShowAppMessageState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(ShowAppMessageState)

public:
    ShowAppMessageState(QState* parentState, const QString& appMessage);

private:
    QString _appMessage;
};
