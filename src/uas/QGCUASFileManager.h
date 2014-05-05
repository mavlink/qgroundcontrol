#ifndef QGCUASFILEMANAGER_H
#define QGCUASFILEMANAGER_H

#include <QObject>
#include "UASInterface.h"

class QGCUASFileManager : public QObject
{
    Q_OBJECT
public:
    QGCUASFileManager(QObject* parent, UASInterface* uas);

signals:

public slots:
    void nothingMessage();

protected:
    UASInterface* _mav;

};

#endif // QGCUASFILEMANAGER_H
