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
    void styleChanged(int index);
    void setDefaultStyle();
    void selectStylesheet();

private:
    Ui::QGCSettingsWidget* ui;
    QString darkStyleSheet;
    QString lightStyleSheet;
    bool updateStyle(QString style);
};

#endif // QGCSETTINGSWIDGET_H
