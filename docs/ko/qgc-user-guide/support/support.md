# 지원

이 사용자 가이드는 _QGroundControl_에 대한 주요 지원 제공자를 위한 것입니다.
올바르지 않거나 누락된 정보를 발견하면 [문제](https://github.com/mavlink/qgc-user-guide/issues)를 신고하여 주십시오.

_QGroundControl_ 사용 방법에 대한 _질문_은 관련 플라이트 스택에 대한 토론 포럼에서 제기하여야 합니다.

- [PX4 Pro Flight Stack](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-usage)(discuss.px4.io).
- [ArduPilot Flight Stack](http://discuss.ardupilot.org/c/ground-control-software/qgroundcontrol) (discuss.ardupilot.org).

이 포럼은 또한 _QGroundControl_에 대한 버그, 문제 및 원하는 기능에 대한 토론을 시작하기에 가장 적합합니다. 거기에서 추가 해결을 위하여 GitHub 문제에 정보를 입력할 수 있습니다.

### 개발자 채팅 {#developer_chat}

_QGroundControl_ 개발자(및 많은 일반/깊은 관련 사용자)는 [#QGroundControl Slack 채널](https://px4.slack.com/)에서 찾을 수 있습니다.

## GitHub 이슈

문제는 _QGroundControl_에 대한 버그와 이후 버전에 대한 기능 요청을 추적하는 데 사용됩니다. 현재 문제 목록은 [GitHub 여기](https://github.com/mavlink/qgroundcontrol/issues)에서 찾을 수 있습니다.

:::info
버그 또는 기능 요청에 대한 GitHub 문제를 생성하기 **전에** 지원 포럼을 사용하여 개발자에게 문의하십시오.
:::

### 버그 신고

문제를 생성하라는 지시를 받은 경우 "버그 보고서" 템플릿을 사용하고 템플릿에 지정된 모든 정보를 제공합니다.

##### Windows 빌드에서 충돌 보고

QGroundControl이 충돌하면 충돌 덤프 파일이 Users LocalAppData 디렉터리에 저장됩니다. 해당 디렉토리로 이동하려면 시작/실행 명령을 사용하십시오. WinKey+R 단축키를 이용하여 이 창을 열수 있습니다. `%localappdata%` 열기에 대해 확인을 클릭합니다. 크래시 덤프는 `QGCCrashDumps` 해당 디렉토리의 폴더에 저장됩니다. 새 **.dmp** 파일을 찾을 수 있습니다. 문제 보고시 GitHub 문제에 해당 파일에 대한 링크를 추가하십시오.

##### Windows 빌드에서 보고 중단

Windows에서 _QGroundControl 프로그램이 응답하지 않습니다_라는 메시지가 표시되면 다음 단계를 사용하여 중단을 보고합니다.

1. _작업 관리자_ 오픈(작업 표시줄을 마우스 오른쪽 버튼으로 클릭하고 **작업 관리자** 선택) 합니다.
2. 프로세스 탭과 로컬 **qgroundcontrol.exe**로 전환합니다.
3. **groundcontrol.exe**를 마우스 오른쪽 버튼으로 클릭하고 **덤프 파일 만들기**를 선택합니다.
4. 공용 위치에 덤프 파일을 배치합니다.
5. GitHub 문제에서 **.dmp** 파일 및 위의 세부 정보에 대한 링크를 추가합니다.

### 기능 요청

지원 포럼에서 토론한 후 기능 요청을 생성하라는 지시를 받은 경우에는 필수 세부 정보에 대한 유용한 정보가 있는 "기능 요청" 템플릿을 사용하십시오.

## 문제 해결

문제 해결 정보는 [여기](../troubleshooting/index.md)를 참고하십시오.

### 콘솔 로깅

_콘솔 로그_는 _QGroundControl_ 문제 진단에 유용합니다. 자세한 내용은 [콘솔 로깅](../settings_view/console_logging.md)를 참고하십시오.

## 이 문서 개선 작업을 도와주십시오.

_QGroundControl_ 자체와 마찬가지로 사용자 가이드는 사용자가 만들고 GitBook을 지원하는 오픈 소스입니다. 수정 및/또는 업데이트에 대한 가이드에 대한 [풀 리퀘스트](https://github.com/mavlink/qgc-user-guide/pulls)를 환영합니다.
