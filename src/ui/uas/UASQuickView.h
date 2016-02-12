#ifndef UASQUICKVIEW_H
#define UASQUICKVIEW_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include "uas/UASInterface.h"
#include "ui_UASQuickView.h"
#include "UASQuickViewItem.h"
#include "MAVLinkDecoder.h"
#include "UASQuickViewItemSelect.h"
#include "Vehicle.h"
class UASQuickView : public QWidget
{
    Q_OBJECT
public:
    UASQuickView(QWidget *parent = 0);
    ~UASQuickView();
    void addSource(MAVLinkDecoder *decoder);
private:
    UASInterface *uas;

    /** List of enabled properties */
    QList<QString> uasEnabledPropertyList;

    /** Maps from the property name to the current value */
    QMap<QString,double> uasPropertyValueMap;

    /** Maps from property name to the display item */
    QMap<QString,UASQuickViewItem*> uasPropertyToLabelMap;

    /** Timer for updating the UI */
    QTimer *updateTimer;

    /** Selection dialog for selectin/deselecting gauge items */
    UASQuickViewItemSelect *quickViewSelectDialog;

    /** Saves gauge layout to settings file */
    void saveSettings();

    /** Loads gauge layout from settings file */
    void loadSettings();

    void recalculateItemTextSizing();

    /** Column Count */
    int m_columnCount;

    QList<QVBoxLayout*> m_verticalLayoutList;
    void sortItems(int columncount);
    QList<int> m_verticalLayoutItemCount;
    int m_currentColumn;
    QMap<QString,int> m_PropertyToLayoutIndexMap;

    //FlowLayout *layout;
protected:
    Ui::Form ui;
    void resizeEvent(QResizeEvent *evt);
signals:
    
public slots:
    void valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant& value,const quint64 msecs);
    void actionTriggered(bool checked);
    void addActionTriggered();
    void updateTimerTick();
    void selectDialogClosed();
    void valueEnabled(QString value);
    void valueDisabled(QString value);
    void columnActionTriggered();
    
private slots:
    void _activeVehicleChanged(Vehicle* vehicle);
};

#endif // UASQUICKVIEW_H
