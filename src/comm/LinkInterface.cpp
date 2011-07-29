#include "LinkInterface.h"

LinkInterface::~LinkInterface()
{
	emit this->deleteLink(this);
}