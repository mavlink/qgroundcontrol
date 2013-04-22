#ifndef QGCCOMBOBOX_H
#define QGCCOMBOBOX_H

#include <QWidget>
#include <QAction>
#include <QtDesigner/QDesignerExportWidget>

#include "QGCToolWidgetItem.h"

namespace Ui
{
class QGCComboBox;
}

class QGCComboBox : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCComboBox(QWidget *parent = 0);
    ~QGCComboBox();

public slots:
    void startEditMode();
    void endEditMode();
    /** @brief Send the parameter to the MAV */
    void sendParameter();
    /** @brief Update the UI with the new parameter value */
    void setParameterValue(int uas, int component, int paramCount, int paramIndex, QString parameterName, const QVariant value);
    void writeSettings(QSettings& settings);
    void readSettings(const QString& pre,const QVariantMap& settings);
    void readSettings(const QSettings& settings);
    void refreshParamList();
    void setActiveUAS(UASInterface *uas);
    void selectComponent(int componentIndex);
    void selectParameter(int paramIndex);
    /** @brief Show descriptive text next to slider */
    void showInfo(bool enable);
    /** @brief Show tool tip of calling element */
    void showTooltip();

protected slots:
    /** @brief Request the parameter of this widget from the MAV */
    void requestParameter();
    /** @brief Button click to add a new item to the combobox */
    void addButtonClicked();
    /** @brief Button click to remove the currently selected item from the combobox */
    void delButtonClicked();
    /** @brief Updates current parameter based on new combobox value */
    void comboBoxIndexChanged(QString val);
protected:
    bool visibleEnabled;
    QString visibleParam;
    int visibleVal;
    QMap<QString,QString> comboBoxTextToParamMap;
    QMap<int,QPixmap> comboBoxIndexToPixmap;
    QMap<QString,int> comboBoxTextToValMap; ///< Comboboxtext/parameter value map
    QString parameterName;         ///< Key/Name of the parameter
    QVariant parameterValue;          ///< Value of the parameter
    double parameterScalingFactor; ///< Factor to scale the parameter between slider and true value
    float parameterMin;
    bool isDisabled;
    float parameterMax;
    int component;                 ///< ID of the MAV component to address
    //double scaledInt;
    void changeEvent(QEvent *e);

private:
    Ui::QGCComboBox *ui;
};

#endif // QGCCOMBOBOX_H
