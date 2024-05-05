#include "DeviceInfo.h"

#include <QtNetwork/QNetworkInformation>
#ifdef QGC_ENABLE_BLUETOOTH
#	include <QtBluetooth/QBluetoothLocalDevice>
#endif

namespace QGCDeviceInfo {

	//  TODO:
	//	- reachabilityChanged()
	//	- Allow to select by transportMedium()

	bool isInternetAvailable() {
		if(QNetworkInformation::availableBackends().isEmpty()) return false;

		// Note: Qt6.7 will do this automatically
		// if(!QNetworkInformation::loadDefaultBackend()) return false;
		// if(!QNetworkInformation::instance()->supports(QNetworkInformation::Feature::Reachability)) return false;

		if(!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) return false;

		return (QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online);
	}

	bool isBluetoothAvailable() {
		#ifdef QGC_ENABLE_BLUETOOTH
            const QList<QBluetoothHostInfo> devices = QBluetoothLocalDevice::allDevices();
            return !devices.isEmpty();
		#else
			return false;
		#endif
	}
}
