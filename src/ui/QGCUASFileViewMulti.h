#ifndef QGCUASFILEVIEWMULTI_H
#define QGCUASFILEVIEWMULTI_H

#include <QWidget>
#include <QMap>

#include "QGCUASFileView.h"
#include "UASInterface.h"

namespace Ui
{
class QGCUASFileViewMulti;
}

class QGCUASFileViewMulti : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileViewMulti(QWidget *parent = 0);
    ~QGCUASFileViewMulti();

public slots:
    void systemDeleted(QObject* uas);
    void systemCreated(UASInterface* uas);
    void systemSetActive(UASInterface* uas);

protected:
    void changeEvent(QEvent *e);
    QMap<UASInterface*, QGCUASFileView*> lists;

private:
    Ui::QGCUASFileViewMulti *ui;
};

#endif // QGCUASFILEVIEWMULTI_H
