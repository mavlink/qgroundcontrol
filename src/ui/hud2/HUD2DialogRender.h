#ifndef HUD2RENDERDIALOG_H
#define HUD2RENDERDIALOG_H

#include <QDialog>

namespace Ui {
class HUD2DialogRender;
}

class HUD2DialogRender : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2DialogRender(QWidget *parent = 0);
    ~HUD2DialogRender();
    
private slots:

private:
    Ui::HUD2DialogRender *ui;
};

#endif // HUD2RENDERDIALOG_H
