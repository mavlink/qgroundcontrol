# 일반 설정(설정 화면)

일반 설정(**설정 화면> 일반 설정**)은 애플리케이션의 주요 항목들을 설정합니다.
Settable values include: display units, autoconnection devices, video display and storage, brand image, and other miscellaneous settings.

:::info
Values are settable even if no vehicle is connected. 차량 재시동이 필요한 설정은 UI에 표시됩니다.
:::

## 단위

이 섹션에서는 응용 프로그램의 표시 단위를 정의합니다.

설정은 다음과 같습니다:

- **거리**: 미터 | 피트
- **면적**: 평방미터 | 스퀘어피트 | 평방킬로미터 | 헥타르 | 에이커 | 스퀘어마일스
- **속도**: 미터/초 | 피트/초 | 마일/시간 | 킬로미터/시간 | 매듭
- **온도**: 섭씨 | 화씨

## 기타

이 섹션에서는 (완벽하지 않음)과 관련된 여러 가지 항목들을 설정합니다: 글꼴 크기, 색 구성표, 지도 제공자, 지도 유형, 원격 측정 로깅, 오디오 출력, 배터리 부족 알림 수준, 기본 임무 고도, [가상 조이스틱](../settings_view/virtual_joystick.md), 임무 자동 로드, 기본 애플리케이션 파일 로드/저장 경로 등

설정 항목들은 다음과 같습니다:

- <a id="language"></a>**Language**: System (System Language) | Bulgarian, Chinese, ...

  Translations are generally built into the application and selected automatically based on the system language.

  Metadata downloaded from the vehicle (such as parameter descriptions) might have translations as well.
  These are downloaded from the internet upon vehicle connection. The translations are then cached locally.
  This means an internet connection during vehicle connection is required at least once.

- <a id="colour_scheme"></a>**Color Scheme**: Indoor (Dark) | Outdoor (Light)

- **지도 제공자**: Google | 맵박스 | 빙 | 에어맵 | 브이월드 | 에니로 | 스탯카트

- **지도 유형**: 도로 | 하이브리드 | 위성

- **스트림 GCS 위치**: 안 함 | 항상 | 팔로우 미 플라이트 모드 시

- **UI 크기 조정**: UI 크기 비율(글꼴, 아이콘, 버튼 크기, 레이아웃 등에 영향)

- **모든 오디오 출력 음소거**: 모든 오디오 출력을 끕니다.

- **인터넷 연결 확인**: 지도 데이타의 다운로드 실패할 수 있는 환경에서 지도를 사용 가능하도록하려면 선택을 취소합니다(지도 타일 엔진이 인터넷 연결을 지속적으로 재확인하는 것을 중지함).

- <a id="autoload_missions"></a> **Autoload Missions**: If enabled, automatically upload a plan to the vehicle on connection.
  - 계획 파일의 이름은 **AutoLoad#.plan**이어야 하며, 여기서 `#`은 차량 Id로 대체됩니다.
  - 계획 파일은 [응용 프로그램 로드/저장 경로](#load_save_path)에 위치하여야 합니다.

- **다음 시작 시 모든 설정 지우기**: _QGroundContro&#x6C;_&#xC774; 다시 시작될 때 모든 설정을 기본값(이 항목 포함)으로 재설정합니다.

- **배터리 부족 알림**: _QGroundContro&#x6C;_&#xC774; 배터리 부족 알리는 배터리 수준입니다.

- <a id="load_save_path"></a>**Application Load/Save Path**: Default location for loading/saving application files, including: parameters, telemetry logs, and mission plans.

## 데이터 지속성 {#data_persistence}

설정 항목들은 다음과 같습니다:

- **모든 데이터 지속성 사용 안함**: 로그, 지도 타일 등 데이터가 저장되거나 캐시되지 않도록 확인합니다.

## 기체의 텔레메트리 로그 {#telemetry_logs}

설정 항목들은 다음과 같습니다:

- <a id="autosave_log"></a>**Save log after each flight**: Telemetry logs (`.tlog`) automatically saved to the _Application Load/Save Path_ ([above](#load_save_path)) after flight.
- **차량 비시동시 로그 저장**: 차량이 _QGroundContro&#x6C;_&#xC5D0; 연결시 기록을 저장합니다.
  차량 연결이 끊어지면 기록을 중지합니다.
- [**CSV 로깅**](csv.md): 원격 측정 데이터의 하위 집합을 CSV 파일에 기록합니다.

## 비행 화면 {#fly_view}

The settings are:

- **실행 전 체크리스트 사용**: 빗행도구 모음에서 실행 전 체크리스트를 활성화합니다.

- **실행 전 체크리스트 적용**: 체크리스트 완료는 무장을 위한 전제 조건입니다.

- **차량 중앙에 지도 유지**: 지도가 현재 선택된 차량의 중앙에 위치하도록 합니다.

- **텔레메트리 로그 재생 상태 표시줄 표시**: [비행 데이터 재생](../fly_view/replay_flight_data.md)에 대한 상태 표시줄을 표시합니다.

- **가상 조이스틱**: [가상 조이스틱](../settings_view/virtual_joystick.md) 활성화(PX4만 해당)

- **수직 계기판 사용**: 계기판을 수평이 아닌 수직으로 정렬합니다(기본값).

- **나침반에 추가 방향 표시기 표시**: 나침반에 추가 표시기를 표시합니다.

- _파란색 화살표_: 지면 위의 코스입니다.

- _백악관_: 집으로 돌아가는 방향.

- _녹색 선_: 다음 웨이포인트 방향.

- **Lock Compass Nose-Up**: 나침반 장미를 회전하려면 선택합니다(기본값은 나침반 표시기 내부에서 기체을 회전입니다).

- **안내식 최소 고도**: 안내식 작업 고도 슬라이더의 최소값입니다.

- **Guided Maximum Altitude**: Maximum value for guided actions altitude slider.

- **이동 위치 최대 거리**: 이동 위치가 현재 차량 위치에서 설정될 수 있는 최대 거리(안내 모드에서).

## 계획 화면 {#plan_view}

설정 항목들은 다음과 같습니다:

- **기본 임무 고도**: 임무 시작 패널 및 첫 번째 웨이포인트의 기본 고도입니다.

## 다음 장치에 자동으로 연결 {#auto_connect}

이 섹션은 _QGroundContro&#x6C;_&#xC774; 자동으로 연결할 기기 세트를 정의합니다.

설정 항목들은 다음과 같습니다:

- <strong>Pixhawk:</strong> Pixhawk 시리즈 기기에 자동 연결
- **SiK 라디오:** SiK(텔레메트리) 라디오에 자동 연결
- **PX4 Flow:** PX4Flow 기기에 자동 연결
- **LibrePilot:** Libre Pilot 자동조종장치에 자동 연결
- **UDP:** UDP에 자동 연결

GNSS receivers connect automatically with **Connect automatically** in the [GNSS Receiver](gnss_receiver.md#receiver) settings.

### Ground Station Location {#nmea_gps}

The ground station position, including from an external GNSS receiver, is configured in the [GNSS Receiver](gnss_receiver.md#gcs_position) settings.

## RTK GPS {#rtk_gps}

RTK base station receivers are configured in the [GNSS Receiver](gnss_receiver.md#receiver) settings, and NTRIP and UDP corrections in [RTK Corrections](ntrip_rtk.md).

## ADSB 서버 {#adsb_server}

설정 항목들은 다음과 같습니다:

- **ADSB SBS 서버에 연결**: 시작 시 ADSB 서버에 연결합니다.
- **호스트 주소**: ADSB 서버 호스트 주소
- **서버 포트**: ADSB 서버 포트

QGroundControl는 원격 또는 로컬 서버(지정된 IP 주소/포트)에서 SBS 형식의 ADSB 메시지를 사용하고 탐지된 기체를 비행화면의 지도에 표시할 수 있습니다.

::: tip
One way to get ADSB information from nearby vehicles is to use [dump1090](https://github.com/antirez/dump1090) to serve the data from a connected RTL-SDR dongle to QGC.

과정은 다음과 같습니다: 1.

1. RTL-SDR 동글(및 안테나)을 가져와 지상국 컴퓨터에 연결합니다(사용 중인 OS에 대해 호환되는 드라이버를 찾아야 할 수도 있음).
2. OS에 _dump109&#x30;_&#xC744; 설치합니다(사전 빌드 또는 소스에서 빌드).
3. `dump1090 --net`을 실행하여 TCP localhost 포트 30003(127.0.0.1:30003)에서 감지된 차량에 대한 메시지 브로드캐스트를 시작합니다.
4. 위의 QGroundControl 설정에 서버(`127.0.0.1`)와 포트(`30003`) 주소를 입력합니다.
5. 지도에서 지역 차량을 보려면 QGroundControl을 다시 시작하세요.

:::

## 비디오 {#video}

_비디오_ 섹션은 _비행 화&#xBA74;_&#xC5D0; 표시될 비디오의 소스 및 연결 설정을 정의합니다.

설정 항목들은 다음과 같습니다:

- **Video Source**: Video Stream Disabled | RTSP Video Stream | UDP h.264 Video Stream | UDP h.265 Video Stream | TCP-MPEG2 Video Stream | MPEG-TS Video Stream | Integrated Camera

  ::: info
  If no video source is specified then no other video or _video recording_ settings will be displayed.
  :::

- **URL/포트**: 연결 유형별 스트림 주소(포트 또는 URL일 수 있음).

- **Aspect Ratio**: 비디오 위젯에서 비디오 크기 조정을 위한 종횡비(크기 조정을 무시하려면 0.0으로 설정)

- **Disabled When Disarmed**: 기체 비시동시 비디오 전송을 비활성화합니다.

- **낮은 대기 시간 모드**: 짧은 대기 시간 모드를 사용하면 동영상 스트림 대기 시간이 줄어들지만 프레임 손실과 끊김 현상이 발생할 수 있습니다(특히 네트워크 연결이 불량한 경우). <!-- disables the internal jitter buffer -->

## 비디오 녹화

_동영상 녹화_ 섹션은 파일 형식과 동영상 저장을 위해 할당된 최대 파일 저장 공간을 지정하는 데 사용됩니다.
비디오는 [애플리케이션 로드/저장 경로](#load_save_path)의 하위 디렉토리("비디오")에 저장됩니다.

The settings are:

- **파일 자동 삭제**: 이 옵션을 선택하면 지정된 저장용량을 사용하면 파일이 자동으로 삭제됩니다.
- **최대 저장용량**: 동영상 파일이 자동 삭제되기 전의 최대 동영상 파일 저장용량입니다.
- **동영상 파일 형식**: 저장된 동영상 녹화를 위한 파일 형식: mkv, mov, mp4.

## 브랜드 이미지

이 설정은 실내/실외 색 구성표에 사용되는 _브랜드 이미&#xC9C0;_&#xB97C; 지정합니다.

도구 모음의 오른쪽 상단 모서리에 있는 연결된 자동 조종 장치의 아이콘 대신 브랜드 이미지가 표시됩니다.
회사 로고/브랜딩이 포함된 화면/동영상 캡처를 사용자가 쉽게 생성할 수 있습니다.

설정 항목들은 다음과 같습니다:

- **실내 이미지**: [실내 배색](#colour_scheme)에 사용되는 브랜드 이미지
- **실외 이미지**: [야외 배색](#colour_scheme)에 사용되는 브랜드 이미지
- **기본 브랜드 이미지 재설정**: 브랜드 이미지를 기본값으로 재설정합니다.
