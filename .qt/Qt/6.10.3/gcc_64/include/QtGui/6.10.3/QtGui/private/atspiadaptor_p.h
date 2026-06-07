// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef ATSPIADAPTOR_H
#define ATSPIADAPTOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <atspi/atspi.h>

#include <QtGui/private/qtguiglobal_p.h>
#include <QtDBus/qdbusvirtualobject.h>
#include <QtGui/qaccessible.h>

#include "dbusconnection_p.h"
#include "qspi_struct_marshallers_p.h"

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class QAccessibleInterface;
class QSpiApplicationAdaptor;


class AtSpiAdaptor :public QDBusVirtualObject
{
    Q_OBJECT

public:
    explicit AtSpiAdaptor(QAtSpiDBusConnection *connection, QObject *parent = nullptr);
    ~AtSpiAdaptor();

    void registerApplication();
    QString introspect(const QString &path) const override;
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;
    void notify(QAccessibleEvent *event);

public Q_SLOTS:
    void eventListenerRegistered(const QString &bus, const QString &path);
    void eventListenerDeregistered(const QString &bus, const QString &path);
    void windowActivated(QObject* window, bool active);

private:
    void updateEventListeners();
    void setBitFlag(const QString &flag);

    // sending messages
    static QVariantList packDBusSignalArguments(const QString &type, int data1, int data2, const QVariant &variantData);
    bool sendDBusSignal(const QString &path, const QString &interface, const QString &name, const QVariantList &arguments) const;
    QVariant variantForPath(const QString &path) const;

    void sendFocusChanged(QAccessibleInterface *interface) const;
    void notifyAboutCreation(QAccessibleInterface *interface) const;
    void notifyAboutDestruction(QAccessibleInterface *interface) const;
    void childrenChanged(QAccessibleInterface *interface) const;

    // handlers for the different accessible interfaces
    bool applicationInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool accessibleInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool componentInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool actionInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool textInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool editableTextInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool valueInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool selectionInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool tableInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);
    bool tableCellInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection);

    void sendReply(const QDBusConnection &connection, const QDBusMessage &message, const QVariant &argument) const;

    QAccessibleInterface *interfaceFromPath(const QString& dbusPath) const;
    QString pathForInterface(QAccessibleInterface *interface) const;
    QString pathForObject(QObject *object) const;

    void notifyStateChange(QAccessibleInterface *interface, const QString& state, int value);

    void sendAnnouncement(QAccessibleAnnouncementEvent *event);

    // accessible helper functions
    AtspiRole getRole(QAccessibleInterface *interface) const;
    QSpiAttributeSet getAttributes(QAccessibleInterface *) const;
    QSpiRelationArray relationSet(QAccessibleInterface *interface, const QDBusConnection &connection) const;
    QStringList accessibleInterfaces(QAccessibleInterface *interface) const;

    // component helper functions
    static QRect getExtents(QAccessibleInterface *interface, uint coordType);
    static bool isValidCoordType(uint coordType);
    static QRect translateFromScreenCoordinates(QAccessibleInterface *interface, const QRect &rect, uint targetCoordType);
    static QPoint translateToScreenCoordinates(QAccessibleInterface *interface, const QPoint &pos, uint fromCoordType);

    // action helper functions
    QSpiActionArray getActions(QAccessibleInterface *interface) const;

    // text helper functions
    QVariantList getAttributes(QAccessibleInterface *, int offset, bool includeDefaults) const;
    QString getAttributeValue(QAccessibleInterface *, int offset, const QString &attributeName) const;
    QList<QVariant> getCharacterExtents(QAccessibleInterface *, int offset, uint coordType) const;
    QList<QVariant> getRangeExtents(QAccessibleInterface *, int startOffset, int endOffset, uint coordType) const;
    static QAccessible::TextBoundaryType qAccessibleBoundaryTypeFromAtspiBoundaryType(int atspiTextBoundaryType);
    static bool isValidAtspiTextGranularity(uint coordType);
    static QAccessible::TextBoundaryType qAccessibleBoundaryTypeFromAtspiTextGranularity(uint atspiTextGranularity);
    static bool inheritsQAction(QObject *object);

    // private vars
    QSpiObjectReference m_accessibilityRegistry;
    QAtSpiDBusConnection *m_dbus;
    QSpiApplicationAdaptor *m_applicationAdaptor;

    /// Assigned from the accessibility registry.
    int m_applicationId;

    // Bit fields - which updates to send

    // AT-SPI has some events that we do not care about:
    // document
    // document-load-complete
    // document-load-stopped
    // document-reload
    uint sendFocus : 1;
    // mouse abs/rel/button

    // all of object
    uint sendObject : 1;
    uint sendObject_active_descendant_changed : 1;
    uint sendObject_announcement : 1;
    uint sendObject_attributes_changed : 1;
    uint sendObject_bounds_changed : 1;
    uint sendObject_children_changed : 1;
//    uint sendObject_children_changed_add : 1;
//    uint sendObject_children_changed_remove : 1;
    uint sendObject_column_deleted : 1;
    uint sendObject_column_inserted : 1;
    uint sendObject_column_reordered : 1;
    uint sendObject_link_selected : 1;
    uint sendObject_model_changed : 1;
    uint sendObject_property_change : 1;
    uint sendObject_property_change_accessible_description : 1;
    uint sendObject_property_change_accessible_name : 1;
    uint sendObject_property_change_accessible_parent : 1;
    uint sendObject_property_change_accessible_role : 1;
    uint sendObject_property_change_accessible_table_caption : 1;
    uint sendObject_property_change_accessible_table_column_description : 1;
    uint sendObject_property_change_accessible_table_column_header : 1;
    uint sendObject_property_change_accessible_table_row_description : 1;
    uint sendObject_property_change_accessible_table_row_header : 1;
    uint sendObject_property_change_accessible_table_summary : 1;
    uint sendObject_property_change_accessible_value : 1;
    uint sendObject_row_deleted : 1;
    uint sendObject_row_inserted : 1;
    uint sendObject_row_reordered : 1;
    uint sendObject_selection_changed : 1;
    uint sendObject_state_changed : 1;
    uint sendObject_text_attributes_changed : 1;
    uint sendObject_text_bounds_changed : 1;
    uint sendObject_text_caret_moved : 1;
    uint sendObject_text_changed : 1;
//    uint sendObject_text_changed_delete : 1;
//    uint sendObject_text_changed_insert : 1;
    uint sendObject_text_selection_changed : 1;
    uint sendObject_value_changed : 1;
    uint sendObject_visible_data_changed : 1;

    // we don't implement terminal
    // terminal-application_changed/charwidth_changed/columncount_changed/line_changed/linecount_changed
    uint sendWindow : 1;
    uint sendWindow_activate : 1;
    uint sendWindow_close: 1;
    uint sendWindow_create : 1;
    uint sendWindow_deactivate : 1;
//    uint sendWindow_desktop_create : 1;
//    uint sendWindow_desktop_destroy : 1;
    uint sendWindow_lower : 1;
    uint sendWindow_maximize : 1;
    uint sendWindow_minimize : 1;
    uint sendWindow_move : 1;
    uint sendWindow_raise : 1;
    uint sendWindow_reparent : 1;
    uint sendWindow_resize : 1;
    uint sendWindow_restore : 1;
    uint sendWindow_restyle : 1;
    uint sendWindow_shade : 1;
    uint sendWindow_unshade : 1;
};

QT_END_NAMESPACE

#endif
