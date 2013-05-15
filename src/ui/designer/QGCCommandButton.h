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
    void readSettings(const QString& pre,const QVariantMap& settings);
signals:
    void showLabel(QString name, int num);
private:
    int responsenum;
    int responsecount;
    QString showlabelname;
    Ui::QGCCommandButton *ui;
    UASInterface* uas;
};

#endif // QGCCOMMANDBUTTON_H
