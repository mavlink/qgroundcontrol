/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _SERIALSIMULATIONLINK_H_
#define _SERIALSIMULATIONLINK_H_

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QTimer>
#include <QTextStream>
#include <QMutex>
#include <SerialLinkInterface.h>
#include <SerialLink.h>

/**
 * The serial simulation link class simulates a robot connected to the groundstation.
 * It can be either file-based by reading telemetry messages from a file or realtime
 * by communicating with a simulation.
 *
 **/
class SerialSimulationLink : public SerialLinkInterface {
	Q_OBJECT

public:
        SerialSimulationLink(QFile* inputFile=NULL, QFile* outputFile=NULL, int sendrate=20);
	~SerialSimulationLink();

	bool isConnected();
	qint64 bytesAvailable();

	void run();

	/* Extensive statistics for scientific purposes */
	qint64 getNominalDataRate();
	qint64 getTotalUpstream();
	qint64 getShortTermUpstream();
	qint64 getCurrentUpstream();
	qint64 getMaxUpstream();
	qint64 getTotalDownstream();
	qint64 getShortTermDownstream();
	qint64 getCurrentDownstream();
	qint64 getMaxDownstream();
	qint64 getBitsSent();
	qint64 getBitsReceived();

	void enableLoopBackMode(SerialLink* loop);
	QString getPortName();
	int getBaudRate();
	int getBaudRateType();
	int getFlowType();
	int getParityType();
	int getDataBitsType();
	int getStopBitsType();

	int getLinkQuality();
	bool isFullDuplex();

public slots:
	bool setPortName(QString portName);
	bool setBaudRateType(int rateIndex);
	bool setBaudRate(int rate);
	bool setFlowType(int flow);
	bool setParityType(int parity);
	bool setDataBitsType(int dataBits);
	bool setStopBitsType(int stopBits);

	void readLine();
        void writeBytes(char *bytes, qint64 length);
        void readBytes();

	bool connect();
	bool disconnect();

	void setMaximumTimeNoise(int milliseconds);

protected:
	QTimer* timer;
	SerialLink* loopBack;
	/** File which contains the input data (simulated robot messages) **/
	QFile* simulationFile;
	/** File where the commands sent by the groundstation are stored **/
	QFile* receiveFile;
	QTextStream stream;
	QTextStream* fileStream;
	QTextStream* outStream;
	/** Buffer for next line. The next line is read ahead */
	QByteArray lineBuffer;
	/** Buffer which can be read from connected protocols through readBytes(). **/
	QByteArray readyBuffer;
	QMutex readyBufferMutex;
	bool _isConnected;
	quint64 rate;
	int maxTimeNoise;
	quint64 lastSent;

	void addTimeNoise();

signals:
	//void connected();
	//void disconnected();
	//void bytesReady(LinkInterface *link);

};

#endif // _SERIALSIMULATIONLINK_H_
