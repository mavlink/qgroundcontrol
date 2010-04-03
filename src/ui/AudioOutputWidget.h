#ifndef AUDIOOUTPUTWIDGET_H
#define AUDIOOUTPUTWIDGET_H

#include <QtGui/QWidget>

namespace Ui {
    class AudioOutputWidget;
}

class AudioOutputWidget : public QWidget {
    Q_OBJECT
public:
    AudioOutputWidget(QWidget *parent = 0);
    ~AudioOutputWidget();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AudioOutputWidget *m_ui;
};

#endif // AUDIOOUTPUTWIDGET_H
