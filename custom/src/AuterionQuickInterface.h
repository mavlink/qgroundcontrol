/*!
 * @file
 *   @brief Auterion QtQuick Interface
 *   @author Gus Grubba <gus@grubba.com>
 */

#pragma once

#include "Vehicle.h"

#include <QObject>
#include <QTimer>

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class AuterionQuickInterface : public QObject
{
    Q_OBJECT
public:
    AuterionQuickInterface(QObject* parent = NULL);
    ~AuterionQuickInterface();

    void    init            ();

};
