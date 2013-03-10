#include <QSettings>

#include "HUD2.h"

#include "HUD2InstrumentDialog.h"
#include "ui_HUD2InstrumentDialog.h"

HUD2InstrumentDialog::HUD2InstrumentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HUD2InstrumentDialog)
{
    ui->setupUi(this);

    QSettings settings;
    settings.beginGroup("QGC_HUD2");

//    // antialiasing
//    connect(ui->aacheckBox, SIGNAL(clicked(bool)), parent, SLOT(toggleAntialising(bool)));
//    bool aa = settings.value("ANTIALIASING", true).toBool();
//    if (aa)
//        ui->aacheckBox->setCheckState(Qt::Checked);
//    else
//        ui->aacheckBox->setCheckState(Qt::Unchecked);

//    // render type
//    int renderType = settings.value("RENDER_TYPE", 0).toInt();
//    ui->renderComboBox->addItem("Native");
//    ui->renderComboBox->addItem("OpenGL");
//    ui->renderComboBox->setCurrentIndex(renderType);
//    connect(ui->renderComboBox, SIGNAL(currentIndexChanged(int)), parent, SLOT(switchRender(int)));

//    // fps limit
//    ui->fpsSpinBox->setRange(HUD2_FPS_MIN, HUD2_FPS_MAX);
//    int fpsLimit = settings.value("FPS_LIMIT", HUD2_FPS_DEFAULT).toInt();
//    ui->fpsSpinBox->setValue(fpsLimit);
//    connect(ui->fpsSpinBox, SIGNAL(valueChanged(int)), parent, SLOT(setFpsLimit(int)));

    settings.endGroup();
}

HUD2InstrumentDialog::~HUD2InstrumentDialog()
{
    delete ui;
}
