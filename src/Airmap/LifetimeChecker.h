/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <memory>

//-----------------------------------------------------------------------------
/**
 * @class LifetimeChecker
 * Base class which helps to check if an object instance still exists.
 * A subclass can take a weak pointer from _instance and then check if the object was deleted.
 * This is used in callbacks that access 'this', but the instance might already be deleted (e.g. vehicle disconnect).
 */
class LifetimeChecker
{
public:
    LifetimeChecker() : _instance(this, [](void*){}) { }
    virtual ~LifetimeChecker() = default;

protected:
    std::shared_ptr<LifetimeChecker> _instance;
};
