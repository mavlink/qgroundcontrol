#include <QSettings>
#include <QColorDialog>
#include <QMessageBox>

#include "HUD2Drawer.h"
#include "HUD2ColorDialog.h"
#include "ui_HUD2ColorDialog.h"

HUD2ColorDialog::HUD2ColorDialog(HUD2IndicatorHorizon *horizon,
                                 HUD2IndicatorRoll *roll,
                                 HUD2IndicatorSpeed *speed,
                                 HUD2IndicatorClimb *climb,
                                 HUD2IndicatorCompass *compass,
                                 HUD2IndicatorFps *fps,
                                 QWidget *parent) :
    QDialog(parent),
    horizon(horizon),
    roll(roll),
    speed(speed),
    climb(climb),
    compass(compass),
    fps(fps),
    ui(new Ui::HUD2ColorDialog)
{
    ui->setupUi(this);

}

HUD2ColorDialog::~HUD2ColorDialog()
{
    delete ui;
}

void HUD2ColorDialog::on_instrumentsButton_clicked()
{
    QColorDialog *d;
    QColor old_color;
    QColor new_color;
    QSettings settings;
    int result;

    settings.beginGroup("QGC_HUD2");
    old_color = settings.value("INSTRUMENTS_COLOR", INSTRUMENTS_COLOR_DEFAULT).value<QColor>();

    d = new QColorDialog(old_color, this);
    connect(d, SIGNAL(currentColorChanged(QColor)), this->horizon,  SLOT(setColor(QColor)));
    connect(d, SIGNAL(currentColorChanged(QColor)), this->roll,     SLOT(setColor(QColor)));
    connect(d, SIGNAL(currentColorChanged(QColor)), this->speed,    SLOT(setColor(QColor)));
    connect(d, SIGNAL(currentColorChanged(QColor)), this->climb,    SLOT(setColor(QColor)));
    connect(d, SIGNAL(currentColorChanged(QColor)), this->compass,  SLOT(setColor(QColor)));
    connect(d, SIGNAL(currentColorChanged(QColor)), this->fps,      SLOT(setColor(QColor)));

    result = d->exec();

    if (QDialog::Accepted == result){
        new_color = d->selectedColor();
        settings.setValue("INSTRUMENTS_COLOR", new_color);
    }
    else{
        this->horizon->setColor(old_color);
        this->roll->setColor(old_color);
        this->speed->setColor(old_color);
        this->climb->setColor(old_color);
        this->compass->setColor(old_color);
        this->fps->setColor(old_color);
    }

    delete d;
    settings.endGroup();
}

void HUD2ColorDialog::on_skyButton_clicked()
{
    QColor old_color;
    QColor new_color;
    QSettings settings;
    int result;

    settings.beginGroup("QGC_HUD2");
    old_color = settings.value("SKY_COLOR", SKY_COLOR_DEFAULT).value<QColor>();

    QColorDialog *d;
    d = new QColorDialog(old_color, this);
    connect(d, SIGNAL(currentColorChanged(QColor)), this->horizon, SLOT(setSkyColor(QColor)));
    result = d->exec();

    if (QDialog::Accepted == result){
        new_color = d->selectedColor();
        settings.setValue("SKY_COLOR", new_color);
    }
    else{
        this->horizon->setSkyColor(old_color);
    }

    delete d;
    settings.endGroup();
}

void HUD2ColorDialog::on_gndButton_clicked()
{
    QColor old_color;
    QColor new_color;
    QSettings settings;
    QColorDialog *d;
    int result;

    settings.beginGroup("QGC_HUD2");
    old_color = settings.value("GND_COLOR", GND_COLOR_DEFAULT).value<QColor>();

    d = new QColorDialog(old_color, this);
    connect(d, SIGNAL(currentColorChanged(QColor)), this->horizon, SLOT(setGndColor(QColor)));
    result = d->exec();

    if (QDialog::Accepted == result){
        new_color = d->selectedColor();
        settings.setValue("GND_COLOR", new_color);
    }
    else{
        this->horizon->setGndColor(old_color);
    }

    delete d;
    settings.endGroup();
}

void HUD2ColorDialog::on_defaultsButton_clicked()
{
    QSettings settings;
    int r = QMessageBox::question(this, tr("Defaults"),
                    tr("Are you sure you want to\n"
                       "reset colors to default values?"),
                    QMessageBox::Yes | QMessageBox::Default,
                    QMessageBox::No  | QMessageBox::Escape);

    if (QMessageBox::Yes == r){
        settings.beginGroup("QGC_HUD2");
        settings.setValue("INSTRUMENTS_COLOR", INSTRUMENTS_COLOR_DEFAULT);
        settings.setValue("SKY_COLOR", SKY_COLOR_DEFAULT);
        settings.setValue("GND_COLOR", GND_COLOR_DEFAULT);

        this->horizon->setColor(INSTRUMENTS_COLOR_DEFAULT);
        this->roll->setColor(INSTRUMENTS_COLOR_DEFAULT);
        this->speed->setColor(INSTRUMENTS_COLOR_DEFAULT);
        this->climb->setColor(INSTRUMENTS_COLOR_DEFAULT);
        this->compass->setColor(INSTRUMENTS_COLOR_DEFAULT);
        this->fps->setColor(INSTRUMENTS_COLOR_DEFAULT);

        this->horizon->setSkyColor(SKY_COLOR_DEFAULT);
        this->horizon->setGndColor(GND_COLOR_DEFAULT);

        settings.endGroup();
    }
}
