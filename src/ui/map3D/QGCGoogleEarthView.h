#ifndef QGCGOOGLEEARTHVIEW_H
#define QGCGOOGLEEARTHVIEW_H

#include <QWidget>
#include <QTimer>
#include <UASInterface.h>

#if (defined Q_OS_MAC)
#include <QWebView>
#endif

#ifdef _MSC_VER
#include <ActiveQt/QAxWidget>
#include "windows.h"

class QGCWebAxWidget : public QAxWidget
{
public:

    QGCWebAxWidget(QWidget* parent = 0, Qt::WindowFlags f = 0)
        : QAxWidget(parent, f)
    {
		// Set web browser control
		setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");
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
#endif

namespace Ui {
#ifdef _MSC_VER
    class QGCGoogleEarthView;
#else
    class QGCGoogleEarthView;
#endif
}

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
    /** @brief Hide and deactivate */
    void hide();
    /** @brief Show and activate */
    void show();

protected:
    void changeEvent(QEvent *e);
    QTimer* updateTimer;
    int refreshRateMs;
    UASInterface* mav;
    bool followCamera;
    bool trailEnabled;
    bool webViewInitialized;
#ifdef _MSC_VER
    QGCWebAxWidget* webViewWin;
#endif
#if (defined Q_OS_MAC)
    QWebView* webViewMac;
#endif

private:
#ifdef _MSC_VER
    Ui::QGCGoogleEarthView* ui;
#else
    Ui::QGCGoogleEarthView* ui;
#endif
};

#endif // QGCGOOGLEEARTHVIEW_H
