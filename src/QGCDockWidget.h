/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCDockWidget_h
#define QGCDockWidget_h

#include <QDockWidget>
#include <QAction>
#include <QPointer>

class QGCDockWidget : public QWidget {
    Q_OBJECT
    
public:
    /// Pass in title = QString() and action = NULL when just using as a regular widget
    QGCDockWidget(const QString& title, QAction* action, QWidget *parent = 0);
    
    void loadSettings(void);
    void saveSettings(void);

    void closeEvent(QCloseEvent* event);

protected:
    QString             _title;
    QPointer<QAction>   _action;
    static const char*  _settingsGroup;
};


#endif
