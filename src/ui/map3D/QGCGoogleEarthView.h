#ifndef QGCGOOGLEEARTHVIEW_H
#define QGCGOOGLEEARTHVIEW_H

#include <QWidget>
#include <QTimer>
#include <UASInterface.h>

#if (defined Q_OS_MAC)
#include <QWebView>
#endif

#if (defined Q_OS_WIN) & (defined _MSVC_VER)
    QGCWebAxWidget* webViewWin;
#include <ActiveQt/QAxWidget>
#include "windows.h"

class WebAxWidget : public QAxWidget
{
public:

    WebAxWidget(QWidget* parent = 0, Qt::WindowFlags f = 0)
        : QAxWidget(parent, f)
    {
    }
protected:
    virtual bool translateKeyEvent(int message, int keycode) const
    {
        if (message >= WM_KEYFIRST && message <= WM_KEYLAST)
            return true;
        else
            return QAxWidget::translateKeyEvent(message, keycode);
    }

};
#else
namespace Ui {
    class QGCGoogleEarthControls;
#if (defined Q_OS_WIN) & (defined _MSVC_VER)
    class QGCGoogleEarthViewWin;
#else
    class QGCGoogleEarthView;
#endif
}
#endif

class QGCGoogleEarthView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCGoogleEarthView(QWidget *parent = 0);
    ~QGCGoogleEarthView();

public slots:
    /** @brief Update the internal state. Does not trigger a redraw */
    void updateState();
    /** @brief Set the currently selected UAS */
    void setActiveUAS(UASInterface* uas);
    /** @brief Show the vehicle trail */
    void showTrail(bool state);
    /** @brief Show the waypoints */
    void showWaypoints(bool state);
    /** @brief Follow the aircraft during flight */
    void follow(bool follow);

protected:
    void changeEvent(QEvent *e);
    QTimer* updateTimer;
    UASInterface* mav;
    bool followCamera;
    bool trailEnabled;
#if (defined Q_OS_WIN) & (defined _MSVC_VER)
    WebAxWidget* webViewWin;
#endif
#if (defined Q_OS_MAC)
    QWebView* webViewMac;
#endif

private:
    Ui::QGCGoogleEarthControls* controls;
#if (defined Q_OS_WIN) && !(defined __MINGW32__)
    Ui::QGCGoogleEarthViewWin* ui;
#else
    Ui::QGCGoogleEarthView* ui;
#endif
};

#endif // QGCGOOGLEEARTHVIEW_H
