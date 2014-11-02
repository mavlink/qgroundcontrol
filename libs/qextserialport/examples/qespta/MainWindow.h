/**
 * @file MainWindow.h
 * @brief Application's Main Window.
 * @see MainWindow
 * @author Micha? Policht
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>

class QMenu;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QMenu *fileMenu;
    QAction *exitAct;
    QMenu *helpMenu;
    QAction *aboutAct;

private:
    void createMenus();
    void createActions();

private slots:
    void about();

public:
    MainWindow();

};

#endif /*MAINWINDOW_H_*/

