#ifndef MAVLINKSWARMSIMULATIONLINK_H
#define MAVLINKSWARMSIMULATIONLINK_H

#include "MAVLinkSimulationLink.h"

class MAVLinkSwarmSimulationLink : public MAVLinkSimulationLink
{
    Q_OBJECT
public:
    MAVLinkSwarmSimulationLink(QString readFile="", QString writeFile="", int rate=5, QObject *parent = 0);

signals:

public slots:
    void mainloop();

};

#endif // MAVLINKSWARMSIMULATIONLINK_H
