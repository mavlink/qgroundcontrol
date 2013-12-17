#ifndef QGCLOGDOWNLOADER_H
#define QGCLOGDOWNLOADER_H

#include <QWidget>
#include "uas/UASInterface.h"

namespace Ui {
class QGCLogDownloader;
}

class QGCLogDownloader : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCLogDownloader(QWidget *parent = 0);
    ~QGCLogDownloader();

public slots:
    void setActiveUAS(UASInterface* uas);
    void forgetUAS(UASInterface* uas);

protected:
    UASInterface* mav;
    
private:
    Ui::QGCLogDownloader *ui;

};

#endif // QGCLOGDOWNLOADER_H
