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

private:
    Ui::QGCSettingsWidget *ui;
	void selectStylesheet();
	bool updateStyle();
};

#endif // QGCSETTINGSWIDGET_H
