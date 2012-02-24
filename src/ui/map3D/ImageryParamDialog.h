#ifndef IMAGERYPARAMDIALOG_H
#define IMAGERYPARAMDIALOG_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>

#include "GlobalViewParams.h"

class ImageryParamDialog : public QDialog
{
    Q_OBJECT

public:
    ImageryParamDialog(QWidget* parent = 0);

    static void getImageryParams(GlobalViewParamsPtr& globalViewParams);

private slots:
    void selectPath(void);
    void closeWithSaving(void);
    void closeWithoutSaving(void);

private:
    void buildLayout(QVBoxLayout* layout);

    QComboBox* mImageryTypeComboBox;
    QLineEdit* mPathLineEdit;
    QDoubleSpinBox* mXOffsetSpinBox;
    QDoubleSpinBox* mYOffsetSpinBox;
    QDoubleSpinBox* mZOffsetSpinBox;
};

#endif // IMAGERYPARAMDIALOG_H
