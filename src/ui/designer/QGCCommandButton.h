#ifndef QGCCOMMANDBUTTON_H
#define QGCCOMMANDBUTTON_H

#include "QGCToolWidgetItem.h"

namespace Ui
{
class QGCCommandButton;
}

class UASInterface;

class QGCCommandButton : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCCommandButton(QWidget *parent = 0);
    ~QGCCommandButton();

public slots:
    void sendCommand();
    void setCommandButtonName(QString text);
    void startEditMode();
    void endEditMode();
    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);

private:
    Ui::QGCCommandButton *ui;
    UASInterface* uas;
};

#endif // QGCCOMMANDBUTTON_H
