#ifndef QGCLOGDOWNLOADER_H
#define QGCLOGDOWNLOADER_H

#include <QWidget>

namespace Ui {
class QGCLogDownloader;
}

class QGCLogDownloader : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCLogDownloader(QWidget *parent = 0);
    ~QGCLogDownloader();
    
private:
    Ui::QGCLogDownloader *ui;
};

#endif // QGCLOGDOWNLOADER_H
