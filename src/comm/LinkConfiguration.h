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

class LinkConfiguration : public QObject
{
    Q_OBJECT

public:
    LinkConfiguration(const QString& name);
    LinkConfiguration(LinkConfiguration* copy);
    virtual ~LinkConfiguration() {}

    Q_PROPERTY(QString name         READ name   WRITE setName   NOTIFY nameChanged)
    Q_PROPERTY(LinkInterface* link  READ link   WRITE setLink   NOTIFY linkChanged)

    // Property accessors

    const QString   name(void)  { return _name; }
    LinkInterface*  link(void)  { return _link; }

    void            setName(const QString name);
    void            setLink(LinkInterface* link);

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

signals:
    void nameChanged(const QString& name);
    void linkChanged(LinkInterface* link);

protected:
    LinkInterface* _link; ///< Link currently using this configuration (if any)
private:
    QString _name;
    bool    _dynamic;    ///< A connection added automatically and not persistent (unless it's edited).
};

#endif // LINKCONFIGURATION_H
