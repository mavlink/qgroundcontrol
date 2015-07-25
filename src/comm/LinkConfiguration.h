/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#ifndef LINKCONFIGURATION_H
#define LINKCONFIGURATION_H

#include <QSettings>

class LinkInterface;

/// Interface holding link specific settings.

class LinkConfiguration
{
public:
    LinkConfiguration(const QString& name);
    LinkConfiguration(LinkConfiguration* copy);
    virtual ~LinkConfiguration() {}

    ///  The link types supported by QGC
    enum {
#ifndef __ios__
        TypeSerial,     ///< Serial Link
#endif
        TypeUdp,        ///< UDP Link
        TypeTcp,        ///< TCP Link
#if 0
        // TODO Below is not yet implemented
        TypeForwarding, ///< Forwarding Link
        TypeXbee,       ///< XBee Proprietary Link
        TypeOpal,       ///< Opal-RT Link
#endif
        TypeMock,       ///< Mock Link for Unitesting
        TypeLogReplay,
        TypeLast        // Last type value (type >= TypeLast == invalid)
    };

    /*!
     * @brief Get configuration name
     *
     * This is the user friendly name shown in the connection drop down box and the name used to save the configuration in the settings.
     * @return The name of this link.
     */
    const QString name()  { return _name; }

    /*!
     * @brief Set the name of this link configuration.
     *
     * This is the user friendly name shown in the connection drop down box and the name used to save the configuration in the settings.
     * @param[in] name The configuration name
     */
    void setName(const QString name)  {_name = name; }

    /*!
     * @brief Set the link this configuration is currently attched to.
     *
     * @param[in] link The pointer to the current LinkInterface instance (if any)
     */
    void setLink(LinkInterface* link) { _link = link; }

    /*!
     * @brief Get the link this configuration is currently attched to.
     *
     * @return The pointer to the current LinkInterface instance (if any)
     */
    LinkInterface* getLink() { return _link; }

    /*!
     *
     * Is this a preferred configuration? (decided at runtime)
     * @return True if this is a known configuration (PX4, etc.)
     */
    bool isPreferred() { return _preferred; }

    /*!
     * Set if this is this a preferred configuration. (decided at runtime)
    */
    void setPreferred(bool preferred = true) { _preferred = preferred; }

    /*!
     *
     * Is this a dynamic configuration? (non persistent)
     * @return True if this is an automatically added configuration.
     */
    bool isDynamic() { return _dynamic; }

    /*!
     * Set if this is this a dynamic configuration. (decided at runtime)
    */
    void setDynamic(bool dynamic = true) { _dynamic = dynamic; }

    /// Virtual Methods

    /*!
     * @brief Connection type
     *
     * Pure virtual method returning one of the -TypeXxx types above.
     * @return The type of links these settings belong to.
     */
    virtual int type() = 0;

    /*!
     * @brief Load settings
     *
     * Pure virtual method telling the instance to load its configuration.
     * @param[in] settings The QSettings instance to use
     * @param[in] root The root path of the setting.
     */
    virtual void loadSettings(QSettings& settings, const QString& root) = 0;

    /*!
     * @brief Save settings
     *
     * Pure virtual method telling the instance to save its configuration.
     * @param[in] settings The QSettings instance to use
     * @param[in] root The root path of the setting.
     */
    virtual void saveSettings(QSettings& settings, const QString& root) = 0;

    /*!
     * @brief Update settings
     *
     * After editing the settings, use this method to tell the connected link (if any) to reload its configuration.
     */
    virtual void updateSettings() {}

    /*!
     * @brief Copy instance data
     *
     * When manipulating data, you create a copy of the configuration using the copy constructor,
     * edit it and then transfer its content to the original using this method.
     * @param[in] source The source instance (the edited copy)
     */
    virtual void copyFrom(LinkConfiguration* source);

    /// Helper static methods

    /*!
     * @brief Root path for QSettings
     *
     * @return The root path of the settings.
     */
    static const QString settingsRoot();

    /*!
     * @brief Create new link configuration instance
     *
     * Configuration Factory. Creates an appropriate configuration instance based on the given type.
     * @return A new instance of the given type
     */
    static LinkConfiguration* createSettings(int type, const QString& name);

    /*!
     * @brief Duplicate configuration instance
     *
     * Helper method to create a new instance copy for editing.
     * @return A new copy of the given settings instance
     */
    static LinkConfiguration* duplicateSettings(LinkConfiguration *source);

protected:
    LinkInterface* _link; ///< Link currently using this configuration (if any)
private:
    QString _name;
    bool    _preferred;  ///< Determined internally if this is a preferred connection. It comes up first in the drop down box.
    bool    _dynamic;    ///< A connection added automatically and not persistent (unless it's edited).
};

#endif // LINKCONFIGURATION_H
