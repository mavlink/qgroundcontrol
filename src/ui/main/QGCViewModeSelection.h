#ifndef QGCVIEWMODESELECTION_H
#define QGCVIEWMODESELECTION_H

#include <QWidget>
#include "MainWindow.h"

namespace Ui {
class QGCViewModeSelection;
}

class QGCViewModeSelection : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCViewModeSelection(QWidget *parent = 0);
    ~QGCViewModeSelection();

    enum MainWindow::CUSTOM_MODE waitForInput();

public slots:

    void selectGeneric();
    void selectPX4();
    void selectAPM();
    void selectWifi();

signals:
    void customViewModeSelected(enum MainWindow::CUSTOM_MODE mode);
    void settingsStorageRequested(bool requested);
    
private:
    Ui::QGCViewModeSelection *ui;
    bool selected;
    enum MainWindow::CUSTOM_MODE mode;
};

#endif // QGCVIEWMODESELECTION_H
