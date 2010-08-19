/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
This file is part of the PIXHAWK project
 
    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.
 
======================================================================*/
 
/**
 * @file
 *   @brief Manage communication links
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _LINKMANAGER_H_
#define _LINKMANAGER_H_

#include <QThread>
#include <QList>
#include <QMap>
#include <LinkInterface.h>
#include <ProtocolInterface.h>

/**
 * The Link Manager organizes the physical Links. It can manage arbitrary
 * links and takes care of connecting them as well assigning the correct
 * protocol instance to transport the link data into the application.
 *
 **/
class LinkManager : public QObject
{
    Q_OBJECT

public:
    static LinkManager* instance();
    ~LinkManager();

    void run();

    QList<LinkInterface*> getLinksForProtocol(ProtocolInterface* protocol);

    /** @brief Get the link for this id */
    LinkInterface* getLinkForId(int id);

    /** @brief Get a list of all links */
    const QList<LinkInterface*> getLinks();

public slots:

    void add(LinkInterface* link);
    void addProtocol(LinkInterface* link, ProtocolInterface* protocol);

    bool connectAll();
    bool connectLink(LinkInterface* link);

    bool disconnectAll();
    bool disconnectLink(LinkInterface* link);

protected:
    LinkManager();
    QList<LinkInterface*> links;
    QMap<ProtocolInterface*,LinkInterface*> protocolLinks;

private:
    static LinkManager* _instance;

signals:
    void newLink(LinkInterface* link);

};

#endif // _LINKMANAGER_H_
