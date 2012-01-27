#ifndef QGCMISSIONOTHER_H
#define QGCMISSIONOTHER_H

#include <QWidget>

namespace Ui {
    class QGCMissionOther;
}

class QGCMissionOther : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionOther(QWidget *parent = 0);
    ~QGCMissionOther();
    Ui::QGCMissionOther *ui;


private:

};

#endif // QGCMISSIONOTHER_H
