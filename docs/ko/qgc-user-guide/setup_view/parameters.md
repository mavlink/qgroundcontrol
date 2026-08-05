# 매개변수

_매개변수_ 화면은 기체와 관련된 매개변수를 검색하고 수정합니다.

:::tip Having trouble?
If parameters fail to download, see [Parameter Download Failures](../troubleshooting/parameter_download.md).
:::

:::info
PX4 Pro와 ArduPilot은 매개변수들은 서로 다르지만, 모두 설정가능합니다.
:::

## 매개변수 검색

매개변수는 그룹화되어 있습니다. Select a group of parameters to view by clicking on the buttons to the left.

_검색_ 필드에 용어를 입력하여 매개변수를 _검색_합니다. 그러면, 입력된 하위 문자열이 포함된 모든 매개변수 이름 및 설명 목록이 표시됩니다(검색을 재설정하려면 **지우기**를 누르십시오).

## Changing a Parameter

To change the value of a parameter click on the parameter row in a group or search list. This will open a side dialog in which you can update the value (this dialog also provides additional detailed information about the parameter - including whether a reboot is required for the change to take effect).

:::info
**저장** 버튼을 클릭하면 매개변수는 기체에 업로드됩니다. 매개변수에 따라 변경된 내용을 적용하기 위해서 비행 컨트롤러 재부팅이 필요할 수 있습니다.
:::

## 도구

화면의 오른쪽 상단의 **도구** 메뉴에서 추가 옵션을 선택할 수 있습니다.

**새로 고침** <br />모든 매개변수를 기체로부터 재로딩합니다.

**기본값으로 재설정** <br />모든 매개변수를 펌웨어의 기본값으로 재설정합니다.

**파일에서 불러오기 / 파일에 저장** <br />기존 파일에서 매개변수를 불러오거나 현재 매개변수 설정을 파일에 저장합니다.

**RC를 Param으로 지우기** <br />이것은 RC 송신기 제어와 매개변수간의 연관성을 모두 삭제합니다. For more information see: [Radio Setup > Param Tuning Channels](../setup_view/radio.md#param-tuning-channels-px4).

**기체 재부팅** <br />기체을 재부팅합니다 (일부 매개변수를 변경후에 요구됩니다).
