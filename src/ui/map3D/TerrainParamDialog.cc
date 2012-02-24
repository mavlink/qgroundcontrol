#include "TerrainParamDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

TerrainParamDialog::TerrainParamDialog(QWidget* parent)
 : QDialog(parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    setWindowTitle(tr("Terrain Parameters"));
    setModal(true);

    buildLayout(layout);
}

void
TerrainParamDialog::getTerrainParams(GlobalViewParamsPtr &globalViewParams)
{
    QVector3D& positionOffset = globalViewParams->terrainPositionOffset();
    QVector3D& attitudeOffset = globalViewParams->terrainAttitudeOffset();

    TerrainParamDialog dialog;
    dialog.mXOffsetSpinBox->setValue(positionOffset.x());
    dialog.mYOffsetSpinBox->setValue(positionOffset.y());
    dialog.mZOffsetSpinBox->setValue(positionOffset.z());
    dialog.mRollOffsetSpinBox->setValue(osg::RadiansToDegrees(attitudeOffset.x()));
    dialog.mPitchOffsetSpinBox->setValue(osg::RadiansToDegrees(attitudeOffset.y()));
    dialog.mYawOffsetSpinBox->setValue(osg::RadiansToDegrees(attitudeOffset.z()));

    if (dialog.exec() == QDialog::Accepted)
    {
        positionOffset.setX(dialog.mXOffsetSpinBox->value());
        positionOffset.setY(dialog.mYOffsetSpinBox->value());
        positionOffset.setZ(dialog.mZOffsetSpinBox->value());
        attitudeOffset.setX(osg::DegreesToRadians(dialog.mRollOffsetSpinBox->value()));
        attitudeOffset.setY(osg::DegreesToRadians(dialog.mPitchOffsetSpinBox->value()));
        attitudeOffset.setZ(osg::DegreesToRadians(dialog.mYawOffsetSpinBox->value()));
    }
}

void
TerrainParamDialog::closeWithSaving(void)
{
    done(QDialog::Accepted);
}

void
TerrainParamDialog::closeWithoutSaving(void)
{
    done(QDialog::Rejected);
}

void
TerrainParamDialog::buildLayout(QVBoxLayout* layout)
{
    QGroupBox* offsetGroupBox = new QGroupBox(tr("Offset"), this);

    mXOffsetSpinBox = new QDoubleSpinBox(this);
    mXOffsetSpinBox->setDecimals(1);
    mXOffsetSpinBox->setRange(-100.0, 100.0);
    mXOffsetSpinBox->setValue(0.0);

    mYOffsetSpinBox = new QDoubleSpinBox(this);
    mYOffsetSpinBox->setDecimals(1);
    mYOffsetSpinBox->setRange(-100.0, 100.0);
    mYOffsetSpinBox->setValue(0.0);

    mZOffsetSpinBox = new QDoubleSpinBox(this);
    mZOffsetSpinBox->setDecimals(1);
    mZOffsetSpinBox->setRange(-100.0, 100.0);
    mZOffsetSpinBox->setValue(0.0);

    mRollOffsetSpinBox = new QDoubleSpinBox(this);
    mRollOffsetSpinBox->setDecimals(0);
    mRollOffsetSpinBox->setRange(-180.0, 180.0);
    mRollOffsetSpinBox->setValue(0.0);

    mPitchOffsetSpinBox = new QDoubleSpinBox(this);
    mPitchOffsetSpinBox->setDecimals(0);
    mPitchOffsetSpinBox->setRange(-180.0, 180.0);
    mPitchOffsetSpinBox->setValue(0.0);

    mYawOffsetSpinBox = new QDoubleSpinBox(this);
    mYawOffsetSpinBox->setDecimals(0);
    mYawOffsetSpinBox->setRange(-180.0, 180.0);
    mYawOffsetSpinBox->setValue(0.0);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(tr("x (m)"), mXOffsetSpinBox);
    formLayout->addRow(tr("y (m)"), mYOffsetSpinBox);
    formLayout->addRow(tr("z (m)"), mZOffsetSpinBox);
    formLayout->addRow(tr("Roll (deg)"), mRollOffsetSpinBox);
    formLayout->addRow(tr("Pitch (deg)"), mPitchOffsetSpinBox);
    formLayout->addRow(tr("Yaw (deg)"), mYawOffsetSpinBox);

    offsetGroupBox->setLayout(formLayout);

    layout->addWidget(offsetGroupBox);
    layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    QPushButton* cancelButton = new QPushButton(this);
    cancelButton->setText("Cancel");

    QPushButton* saveButton = new QPushButton(this);
    saveButton->setText("Save");

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    buttonLayout->addWidget(saveButton);

    layout->addLayout(buttonLayout);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(closeWithoutSaving()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(closeWithSaving()));
}
