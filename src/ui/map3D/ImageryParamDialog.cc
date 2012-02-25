#include "ImageryParamDialog.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

ImageryParamDialog::ImageryParamDialog(QWidget* parent)
 : QDialog(parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    setWindowTitle(tr("Imagery Parameters"));
    setModal(true);

    buildLayout(layout);
}

void
ImageryParamDialog::getImageryParams(GlobalViewParamsPtr &globalViewParams)
{
    ImageryParamDialog dialog;

    switch (globalViewParams->imageryType())
    {
    case Imagery::BLANK_MAP:
        dialog.mImageryTypeComboBox->setCurrentIndex(0);
        break;
    case Imagery::GOOGLE_MAP:
        dialog.mImageryTypeComboBox->setCurrentIndex(1);
        break;
    case Imagery::GOOGLE_SATELLITE:
        dialog.mImageryTypeComboBox->setCurrentIndex(2);
        break;
    case Imagery::OFFLINE_SATELLITE:
        dialog.mImageryTypeComboBox->setCurrentIndex(3);
        break;
    }

    dialog.mPathLineEdit->setText(globalViewParams->imageryPath());

    QVector3D& imageryOffset = globalViewParams->imageryOffset();

    dialog.mXOffsetSpinBox->setValue(imageryOffset.x());
    dialog.mYOffsetSpinBox->setValue(imageryOffset.y());
    dialog.mZOffsetSpinBox->setValue(imageryOffset.z());

    if (dialog.exec() == QDialog::Accepted)
    {
        switch (dialog.mImageryTypeComboBox->currentIndex())
        {
        case 0:
            globalViewParams->imageryType() = Imagery::BLANK_MAP;
            break;
        case 1:
            globalViewParams->imageryType() = Imagery::GOOGLE_MAP;
            break;
        case 2:
            globalViewParams->imageryType() = Imagery::GOOGLE_SATELLITE;
            break;
        case 3:
            globalViewParams->imageryType() = Imagery::OFFLINE_SATELLITE;
            break;
        }

        globalViewParams->imageryPath() = dialog.mPathLineEdit->text();

        imageryOffset.setX(dialog.mXOffsetSpinBox->value());
        imageryOffset.setY(dialog.mYOffsetSpinBox->value());
        imageryOffset.setZ(dialog.mZOffsetSpinBox->value());

        globalViewParams->signalImageryParamsChanged();
    }
}

void
ImageryParamDialog::selectPath(void)
{
    QString filename = QFileDialog::getExistingDirectory(this, "Imagery path",
                                                         QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
    if (filename.isNull())
    {
        return;
    }
    else
    {
        mPathLineEdit->setText(filename);
    }
}

void
ImageryParamDialog::closeWithSaving(void)
{
    done(QDialog::Accepted);
}

void
ImageryParamDialog::closeWithoutSaving(void)
{
    done(QDialog::Rejected);
}

void
ImageryParamDialog::buildLayout(QVBoxLayout* layout)
{
    QFormLayout* formLayout = new QFormLayout;

    mImageryTypeComboBox = new QComboBox(this);
    mImageryTypeComboBox->addItem("None");
    mImageryTypeComboBox->addItem("Map (Google)");
    mImageryTypeComboBox->addItem("Satellite (Google)");
    mImageryTypeComboBox->addItem("Satellite (Offline)");

    mPathLineEdit = new QLineEdit(this);
    mPathLineEdit->setReadOnly(true);

    QPushButton* pathButton = new QPushButton(this);
    pathButton->setText(tr(".."));

    QHBoxLayout* pathLayout = new QHBoxLayout;
    pathLayout->addWidget(mPathLineEdit);
    pathLayout->addWidget(pathButton);

    formLayout->addRow(tr("Imagery Type"), mImageryTypeComboBox);
    formLayout->addRow(tr("Path"), pathLayout);

    layout->addLayout(formLayout);

    QGroupBox* offsetGroupBox = new QGroupBox(tr("Offset"), this);

    mXOffsetSpinBox = new QDoubleSpinBox(this);
    mXOffsetSpinBox->setDecimals(1);
    mXOffsetSpinBox->setRange(-1000.0, 1000.0);
    mXOffsetSpinBox->setValue(0.0);

    mYOffsetSpinBox = new QDoubleSpinBox(this);
    mYOffsetSpinBox->setDecimals(1);
    mYOffsetSpinBox->setRange(-1000.0, 1000.0);
    mYOffsetSpinBox->setValue(0.0);

    mZOffsetSpinBox = new QDoubleSpinBox(this);
    mZOffsetSpinBox->setDecimals(1);
    mZOffsetSpinBox->setRange(-1000.0, 1000.0);
    mZOffsetSpinBox->setValue(0.0);

    formLayout = new QFormLayout;
    formLayout->addRow(tr("x (m)"), mXOffsetSpinBox);
    formLayout->addRow(tr("y (m)"), mYOffsetSpinBox);
    formLayout->addRow(tr("z (m)"), mZOffsetSpinBox);

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

    connect(pathButton, SIGNAL(clicked()), this, SLOT(selectPath()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(closeWithoutSaving()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(closeWithSaving()));
}
