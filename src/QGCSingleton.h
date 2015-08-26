/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGCSINGLETON_H
#define QGCSINGLETON_H

#include "QGCApplication.h"

#include <QObject>
#include <QMutex>

/// @def DECLARE_QGC_SINGLETON
/// Include this macro in your Derived Class definition
///     @param className Derived Class name
///     @param interfaceName If your class is accessed through an interface specify that, if not specify Derived class name.
///             For example DECLARE_QGC_SINGLETON(UASManager, UASManagerInterface)
#define DECLARE_QGC_SINGLETON(className, interfaceName) \
    public: \
        static interfaceName* instance(bool nullOk = false); \
        static void setMockInstance(interfaceName* mock); \
    private: \
        static interfaceName* _createSingleton(void); \
        static void _deleteSingleton(void); \
        static interfaceName* _instance; \
        static interfaceName* _mockInstance; \
        static interfaceName* _realInstance; \
        friend class QGCApplication; \
        friend class UnitTest; \

/// @def IMPLEMENT_QGC_SINGLETON
/// Include this macro in your Derived Class implementation
///     @param className Derived Class name
///     @param interfaceName If your class is accessed through an interface specify that, if not specify Derived class name.
///             For example DECLARE_QGC_SINGLETON(UASManager, UASManagerInterface)
#define IMPLEMENT_QGC_SINGLETON(className, interfaceName) \
    interfaceName* className::_instance = NULL; \
    interfaceName* className::_mockInstance = NULL; \
    interfaceName* className::_realInstance = NULL; \
    \
    interfaceName* className::_createSingleton(void) \
    { \
        Q_ASSERT(_instance == NULL); \
        _instance = new className(qgcApp()); \
        return _instance; \
    } \
    \
    void className::_deleteSingleton(void) \
    { \
        if (className::_instance) { \
            className* instance = qobject_cast<className*>(className::_instance); \
            Q_ASSERT_X(instance != NULL, "QGCSingleton", "If you hit this assert you may have forgotten to clear a Mock instance"); \
            className::_instance = NULL; \
            delete instance; \
        } \
    } \
    \
    interfaceName* className::instance(bool nullOk) \
    { \
        if (!nullOk) { \
            Q_ASSERT_X(_instance, "QGCSingleton", "Request for singleton that is NULL. If you hit this, then you have likely run into a startup or shutdown sequence bug (possibly intermittent)."); \
        } \
        return _instance; \
    } \
    \
    void className::setMockInstance(interfaceName* mock) \
    { \
        if (mock) { \
            Q_ASSERT(_instance); \
            Q_ASSERT(!_realInstance); \
            \
            _realInstance = _instance; \
            _instance = dynamic_cast<interfaceName*>(mock); \
            Q_ASSERT(_instance); \
            _mockInstance = mock; \
        } else { \
            Q_ASSERT(_instance); \
            Q_ASSERT(_realInstance); \
            \
            _instance = _realInstance; \
            _realInstance = NULL; \
            _mockInstance = NULL; \
        } \
    }

class QGCApplication;
class UnitTest;

/// This is the base class for all app global singletons
///
/// All global singletons are created/destroyed at boot time by QGCApplication::_createSingletons and destroyed by QGC::Application::_destroySingletons.
/// This is done in order to make sure they are all created on the main thread. As such no other code other than Unit Test
/// code has access to the constructor/destructor. QGCSingleton supports replacing singletons with a mock implementation.
/// In this case your object must derive from an interface which in turn derives from QGCSingleton. Youu can then use
/// the setMock method to add and remove you mock implementation. See UASManager example usage. In order to provide the
/// appropriate methods to make all this work you need to use the DECLARE_QGC_SINGLETON and IMPLEMENT_QGC_SINGLETON
/// macros as follows:
/// @code{.unparsed}
///     // Header file
///
///     class MySingleton : public QGCSingleton {
///         Q_OBJECT
///
///         DECLARE_QGC_SINGLETON(MySingleton, MySingleton)
///
///         ...
///
///     private:
///         // Constructor/Desctructor private since all access is through the singleton methods
///         MySingleton(QObject* parent == NULL);
///        ~MySingleton();
///
///         ...
///     }
///
///     // Code file
///
///     IMPLEMENT_QGC_SINGLETON(MySingleton, MySingleton)
///
///     MySingleton::MySingleton(QObject* parent) :
///         QGCSigleton(parent)
///     {
///     }
///
///     // Other class methods...
///
/// @endcode
/// The example above does not use an inteface so the second parameter to the macro is the class name as well.

class QGCSingleton : public QObject
{
    Q_OBJECT
    
protected:
    /// Constructor is private since all creation is done through _createInstance
    ///     @param parent Parent object
    QGCSingleton(QObject* parent = NULL);
};

#endif
