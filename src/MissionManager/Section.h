/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(SectionLog)

// A Section encapsulates a set of mission commands which can be associated with another simple mission item.
class Section : public QObject
{
    Q_OBJECT

public:
    Section(Vehicle* vehicle, QObject* parent = nullptr)
        : QObject(parent)
        , _vehicle(vehicle)
    {

    }

    Q_PROPERTY(bool     available           READ available          WRITE setAvailable  NOTIFY availableChanged)
    Q_PROPERTY(bool     settingsSpecified   READ settingsSpecified                      NOTIFY settingsSpecifiedChanged)
    Q_PROPERTY(bool     dirty               READ dirty              WRITE setDirty      NOTIFY availableChanged)

    virtual bool available          (void) const = 0;
    virtual bool settingsSpecified  (void) const = 0;
    virtual bool dirty              (void) const = 0;

    virtual void setAvailable       (bool available) = 0;
    virtual void setDirty           (bool dirty) = 0;

    /// Scans the loaded items for the section items
    ///     @param visualItems Item list
    ///     @param scanIndex Index to start scanning from
    /// @return true: section found, items added, scanIndex updated
    virtual bool scanForSection(QmlObjectListModel* visualItems, int scanIndex) = 0;

    /// Appends the mission items associated with this section
    ///     @param items List to append to
    ///     @param missionItemParent QObject parent for created MissionItems
    ///     @param nextSequenceNumber[in,out] Sequence number for first item, updated as items are added
    virtual void appendSectionItems(QList<MissionItem*>& items, QObject* missionItemParent, int& nextSequenceNumber) = 0;

    /// Returns the number of mission items represented by this section.
    ///     Signals: itemCountChanged
    virtual int itemCount(void) const = 0;

signals:
    void availableChanged           (bool available);
    void settingsSpecifiedChanged   (bool settingsSpecified);
    void dirtyChanged               (bool dirty);
    void itemCountChanged           (int itemCount);

protected:
    Vehicle* _vehicle;
};
