#ifndef QGCGOOGLEEARTHVIEW_H
#define QGCGOOGLEEARTHVIEW_H

#include <QWidget>
#include <QTimer>
#include <UASInterface.h>

#if (defined Q_OS_WIN) && !(defined __MINGW32__)
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
    class QGCGoogleEarthView;
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

protected:
    void changeEvent(QEvent *e);
    QTimer* updateTimer;
    UASInterface* mav;
    bool followCamera;

    #if (defined Q_OS_WIN) && !(defined __MINGW32__)
#else
private:
    Ui::QGCGoogleEarthView *ui;
#endif
};

#endif // QGCGOOGLEEARTHVIEW_H
