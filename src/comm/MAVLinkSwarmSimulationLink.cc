#include "MAVLinkSwarmSimulationLink.h"

MAVLinkSwarmSimulationLink::MAVLinkSwarmSimulationLink(QString readFile, QString writeFile, int rate, QObject *parent) :
    MAVLinkSimulationLink(readFile, writeFile, rate, parent)
{
}


void MAVLinkSwarmSimulationLink::mainloop()
{

}
