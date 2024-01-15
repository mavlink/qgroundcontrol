# 기체 연결 문제

## UI에 기체가 표시되지 않음

QGroundControl은 네트워크이 접속된 기체를 자동으로 연결됩니다(USB 또는 WiFi 등 사용).
QGroundControl UI에 네트워크에 접속한 기체가 표시되지 않는 경우 [콘솔 로깅](../settings_view/console_logging.md)을 사용하여 문제를 디버깅할 수 있습니다.

문제를 디버그하려면 다음 절차를 따라 해결하십시오.

- Start with the hardware vehicle link not connected.
  예를 들어, USB 연결을 연결하거나 OS에서 WiFi 링크를 설정하지 마십시오.

- QGroundControl에서 `LinkManagerLog` [콘솔 로깅](../settings_view/console_logging.md)을 켭니다.
  이것은 QGroundControl에서 연결하는 링크에 대한 출력을 기록합니다.

- Establish the hardware vehicle communication link.

- 콘솔 로그 출력은 다음과 같이 표시되어야 합니다.

  ```
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:563 - "Waiting for bootloader to finish "/dev/cu.usbmodem01""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:563 - "Waiting for bootloader to finish "/dev/cu.usbmodem01""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:563 - "Waiting for bootloader to finish "/dev/cu.usbmodem01""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:563 - "Waiting for bootloader to finish "/dev/cu.usbmodem01""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:572 - "Waiting for next autoconnect pass "/dev/cu.usbmodem4201""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:613 - "New auto-connect port added:  "ArduPilot ChibiOS on cu.usbmodem4201 (AutoConnect)" "/dev/cu.usbmodem4201""
  ```

- 처음 몇 줄은 QGroundControl에서 네트워크를 설정하고 마지막으로 자동 연결을 설정하였음을 나타냅니다.

이 항목이 표시되지 않으면 QGroundControl에서 네트워크를 인식하지 못하는 것입니다.
하드웨어가 OS 수준에서 인식되는지 확인하려면 다음을 수행하십시오.

- Start with the hardware vehicle link not connected.
  예를 들어, USB 연결을 연결하거나 OS에서 WiFi 링크를 설정하지 마십시오.
- QGroundControl에서 `LinkManagerVerboseLog` [콘솔 로깅](../settings_view/console_logging.md)을 켭니다.
  이것은 QGroundControl에서 인식하는 모든 직렬 하드웨어 연결에 대한 출력을 기록합니다.
- 장치에서 직렬 포트의 지속적인 출력을 볼 수 있습니다.
- USB 통신 장치를 연결합니다.
- 콘솔 출력에 새 장치가 표시되어야 합니다. 예:
  ```
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:520 - "-----------------------------------------------------"
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:521 - "portName:           "cu.usbmodem4201""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:522 - "systemLocation:     "/dev/cu.usbmodem4201""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:523 - "description:        "Pixhawk1""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:524 - "manufacturer:       "ArduPilot""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:525 - "serialNumber:       "1B0034000847323433353231""
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:526 - "vendorIdentifier:   1155"
  [D] at /Users/travis/build/mavlink/qgroundcontrol/src/comm/LinkManager.cc:527 - "productIdentifier:  22336"
  ```
- 그 후 첫 번째 예와 같이 해당 장치에 대한 연결을 계속 기록하여야 합니다.

연결했을 때 콘솔 출력에 새 직렬 포트가 표시되지 않으면 OS 수준에서 하드웨어에 문제가 있을 수 있습니다.

## 오류: 기체의 응답이 없음

이는 QGroundControl에서 기체를 네트워크으로 연결할 수 있었지만, 네트워크에서 앞뒤로 이동하는 원격 측정이 없음을 나타냅니다.
이는 불행히도 다음과 같은 여러 문제를 나타낼 수 있습니다.

- 하드웨어 통신 설정 문제
- 펌웨어 문제

마지막으로 QGroundControl에서 기체가 아닌 컴퓨터에 연결된 장치에 자동으로 연결을 시도하는 경우 발생할 수 있습니다.
위의 단계를 사용하고 QGroundControl에 연결을 시도하는 장치 정보를 기록하여 이 경우를 식별할 수 있습니다.
자동 연결이 작동하도록 하기 위하여 자동 연결을 시도하는 장치에서 사용하는 필터는 다소 광범위하고 정확하지 않을 수 있습니다.
이런 일이 발생하면, 일반 설정에서 자동 연결을 끄고 기체의 네트워크에 수동으로 연결하여야 합니다.
컴퓨터에서 문제를 일으키는 장치를 제거할 수도 있지만, 항상 가능한 것은 아닙니다.
