#ifndef QGCSETTINGSWIDGET_H
#define QGCSETTINGSWIDGET_H

#include <QDialog>

namespace Ui
{
class QGCSettingsWidget;
}

class QGCSettingsWidget : public QDialog
{
    Q_OBJECT

public:
    QGCSettingsWidget(QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~QGCSettingsWidget();

public slots:

private:
    Ui::QGCSettingsWidget *ui;
};

#endif // QGCSETTINGSWIDGET_H
