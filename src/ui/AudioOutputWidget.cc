#include "AudioOutputWidget.h"
#include "ui_AudioOutputWidget.h"

AudioOutputWidget::AudioOutputWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::AudioOutputWidget)
{
    m_ui->setupUi(this);
}

AudioOutputWidget::~AudioOutputWidget()
{
    delete m_ui;
}

void AudioOutputWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
