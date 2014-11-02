/**
 * @file MainWindow.cpp
 * @brief MainWindow Implementation.
 * @see MainWindow.h
 * @author Micha? Policht
 */


#include <QMessageBox>
#include <QMenuBar>
#include "MainWindow.h"
#include "MessageWindow.h"
#include "QespTest.h"

MainWindow::MainWindow()
{
    //central widget
    QespTest *qespTest = new QespTest();
    setCentralWidget(qespTest);
    //bottom dock widget
    MessageWindow *msgWindow = new MessageWindow();
    addDockWidget(Qt::BottomDockWidgetArea, msgWindow);

    createActions();
    createMenus();

    setWindowTitle(tr("QextSerialPort Test Application"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About "),
                       tr("<B>""</B><BR>"
                          "author: Michal Policht<br>"
                          "<a href='mailto:xpolik@users.sourceforge.net'>xpolik@users.sourceforge.net</a>"));
}

void MainWindow::createActions()
{
    //File actions
    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("CTRL+D"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    //Help actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setShortcut(tr("CTRL+A"));
    aboutAct->setStatusTip(tr("About application"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}

