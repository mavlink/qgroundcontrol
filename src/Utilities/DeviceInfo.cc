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

        if(!QNetworkInformation::loadDefaultBackend()) return false;

        // Note: Qt6.7 will do this automatically
        if(!QNetworkInformation::instance()->supports(QNetworkInformation::Feature::Reachability))
        {
            if(!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) return false;
        }

        const QNetworkInformation::Reachability reachability = QNetworkInformation::instance()->reachability();

        return (reachability == QNetworkInformation::Reachability::Online);
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
