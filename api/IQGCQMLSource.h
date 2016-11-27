#pragma once

#include <QString>

/// @file
///     @brief Core Plugin Interface for QGroundControl
///     @author Gus Grubba <mavlink@grubba.com>

class IQGCQMLSource
{
public:
    IQGCQMLSource() {}
    virtual ~IQGCQMLSource() {}
    virtual QString     pageUrl                () { return QString(); }
    virtual QString     pageTitle              () { return QString(); }
    virtual QString     pageIconUrl            () { return QString(); }
};
