#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QPixmap>

namespace Ui {
class boardWidget;
}

class BoardWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit BoardWidget(QWidget *parent = 0);
    ~BoardWidget();

    void setBoardInfo(int board_id, const QString &boardName, const QString &bootLoader);

    void setBoardImage(const QString &path);

public slots:
    virtual void resizeEvent(QResizeEvent* event);

    void updateStatus(const QString &status);

signals:
    void flashFirmwareURL(QString url);
    void cancelFirmwareUpload();

protected slots:
    void flashFirmware();
    
private:
    Ui::boardWidget *ui;
    QPixmap boardIcon;
};

#endif // BOARDWIDGET_H
