#ifndef QGCWELCOMEMAINWINDOW_H
#define QGCWELCOMEMAINWINDOW_H

#include <QMainWindow>
#include "MainWindow.h"
#include "QGCViewModeSelection.h"

namespace Ui {
class QGCWelcomeMainWindow;
}

class QGCWelcomeMainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit QGCWelcomeMainWindow(QWidget *parent = 0);
    ~QGCWelcomeMainWindow();

    enum MainWindow::CUSTOM_MODE getCustomMode();
    bool getStoreSettings()
    {
        return storeSettings;
    }

public slots:
    void setStoreSettings(bool settings)
    {
        storeSettings = settings;
    }

signals:
    void customViewModeSelected(enum MainWindow::CUSTOM_MODE mode);
    void settingsStorageRequested(bool requested);
    
private:
    Ui::QGCWelcomeMainWindow *ui;
    QGCViewModeSelection* viewModeSelection;
    bool storeSettings;
};

#endif // QGCWELCOMEMAINWINDOW_H
