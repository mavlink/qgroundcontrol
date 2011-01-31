/**
 * The bytes are copied by calling the LinkInterface::readBytes() method.
 * This method parses all incoming bytes and decodes GPS positions.
 * @param link The interface to read from
 * @see LinkInterface
 **/
void QGCNMEAProtocol::receiveBytes(LinkInterface* link, QByteArray b)
{
    receiveMutex.lock();



    receiveMutex.unlock();
}
