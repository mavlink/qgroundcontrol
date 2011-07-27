#ifndef _XBEECONFIGURATIONWINDOW_H_
#define _XBEECONFIGURATIONWINDOW_H_

#include <QObject>
#include <QWidget>
#include <QAction>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include<QtGui\qcombobox.h>
#include<QtGui\qlabel.h>
#include<QtGui\qlayout.h>
#include <LinkInterface.h>
#include"XbeeLinkInterface.h"


class XbeeConfigurationWindow : public QWidget
{
	Q_OBJECT

public:
	XbeeConfigurationWindow(LinkInterface* link, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
	~XbeeConfigurationWindow();

	QAction* getAction();

public slots:
	void configureCommunication();
	void setPortName(QString port);
	void setBaudRateString(QString baud);
	void setupPortList();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    bool userConfigured; ///< Switch to detect if current values are user-selected and shouldn't be overriden

private:
	QLabel *portLabel;
	QLabel *baudLabel;
	QComboBox *portBox;
	QComboBox *baudBox;
	QGridLayout *actionLayout;
	QHBoxLayout *xbeeLayout;
	QVBoxLayout *tmpLayout;
	XbeeLinkInterface* link;
    QAction* action;
    QTimer* portCheckTimer;
};


#endif //_XBEECONFIGURATIONWINDOW_H_