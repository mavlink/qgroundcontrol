// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

/*
 * This file contains AT-SPI constants and mappings between QAccessible
 * and AT-SPI constants such as 'role' and 'state' enumerations.
 */

#ifndef Q_SPI_CONSTANT_MAPPINGS_H
#define Q_SPI_CONSTANT_MAPPINGS_H

#include "qspi_struct_marshallers_p.h"

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/QAccessible>
#include <atspi/atspi-constants.h>

QT_REQUIRE_CONFIG(accessibility);

// missing from at-spi2-core:
#define ATSPI_DBUS_INTERFACE_EVENT_WINDOW "org.a11y.atspi.Event.Window"
#define ATSPI_DBUS_INTERFACE_EVENT_FOCUS  "org.a11y.atspi.Event.Focus"

#define QSPI_OBJECT_PATH_ACCESSIBLE  "/org/a11y/atspi/accessible"
#define QSPI_OBJECT_PATH_PREFIX      "/org/a11y/atspi/accessible/"

QT_BEGIN_NAMESPACE

struct RoleNames {
    RoleNames() {}
    RoleNames(AtspiRole r, const QString& n, const QString& ln)
        :m_spiRole(r), m_name(n), m_localizedName(ln)
    {}

    AtspiRole spiRole() const {return m_spiRole;}
    QString name() const {return m_name;}
    QString localizedName() const {return m_localizedName;}

private:
    AtspiRole m_spiRole = ATSPI_ROLE_INVALID;
    QString m_name;
    QString m_localizedName;
};

inline void setSpiStateBit(quint64* state, AtspiStateType spiState)
{
    *state |= quint64(1) << spiState;
}

inline void unsetSpiStateBit(quint64* state, AtspiStateType spiState)
{
    *state &= ~(quint64(1) << spiState);
}

quint64 spiStatesFromQState(QAccessible::State state);
QSpiUIntList spiStateSetFromSpiStates(quint64 states);

AtspiRelationType qAccessibleRelationToAtSpiRelation(QAccessible::Relation relation);

QT_END_NAMESPACE

#endif /* Q_SPI_CONSTANT_MAPPINGS_H */
