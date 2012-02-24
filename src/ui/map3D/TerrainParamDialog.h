#ifndef TERRAINPARAMDIALOG_H
#define TERRAINPARAMDIALOG_H

#include <QDoubleSpinBox>
#include <QDialog>
#include <QVBoxLayout>

#include "GlobalViewParams.h"

class TerrainParamDialog : public QDialog
{
    Q_OBJECT

public:
    TerrainParamDialog(QWidget* parent = 0);

    static void getTerrainParams(GlobalViewParamsPtr& globalViewParams);

private slots:
    void closeWithSaving(void);
    void closeWithoutSaving(void);

private:
    void buildLayout(QVBoxLayout* layout);

    QDoubleSpinBox* mXOffsetSpinBox;
    QDoubleSpinBox* mYOffsetSpinBox;
    QDoubleSpinBox* mZOffsetSpinBox;
    QDoubleSpinBox* mRollOffsetSpinBox;
    QDoubleSpinBox* mPitchOffsetSpinBox;
    QDoubleSpinBox* mYawOffsetSpinBox;
};

#endif // TERRAINPARAMDIALOG_H
