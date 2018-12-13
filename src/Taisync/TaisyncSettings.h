/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TaisyncHandler.h"

Q_DECLARE_LOGGING_CATEGORY(TaisyncSettingsLog)

class TaisyncSettings : public TaisyncHandler
{
    Q_OBJECT
public:

    explicit TaisyncSettings            (QObject* parent = nullptr);
    void    start                       () override;

protected slots:
    void    _readBytes                  () override;

};
