# 토글 스위치 8채널 테스트 — DriverLib 버전 — TMS320F28P659DK8-Q1

개발보드 V2 [`07-3개월-일일예제-캘린더.md`](../../../../dev_board_v2/07-3개월-일일예제-캘린더.md)
1주차 화요일 항목. 회로블록 **(7) 범용 Toggle 스위치 8채널**을 순수 DriverLib(SysConfig
미사용)로 읽습니다.

`toggle8ch_test`라는 이름으로 있던 기존 예제를 [7_GP_Toggle8_Bitfield](../7_GP_Toggle8_Bitfield/README.md),
[7_GP_Toggle8_Freertos](../7_GP_Toggle8_Freertos/README.md)와 짝을 맞추기 위해
`7_GP_Toggle8_Driverlib`로 옮기고 이름만 바꿨습니다 — 동작 코드는 그대로입니다.

## 요구 하드웨어
- SyncWorks TMS320F28X 개발보드 V2
- TMS320F28P650DK9 또는 TMS320F28P659DK8-Q1 모듈
- 점퍼 케이블 8개

## 배선
개발보드 **A Side 핀-헤더 101, 103, 105, 107, 109, 111, 113, 115번**(홀수, 8핀,
GPIO18~GPIO25에 1:1 대응)을 (7) 범용 Toggle 스위치 8채널 블록의 입력 8핀에 연결하세요.
핀 매핑 근거:
[`SYNCWORKS_DEVBOARDV2_F28P65X.syscfg.json`](../../../../dev_board_v2/10-sysconfig-보드파일/SYNCWORKS_DEVBOARDV2_F28P65X.syscfg.json)

**v1.10부터 스위치 번호와 GPIO 대응이 핀 순서와 반대입니다** — 8핀을 순서대로 1~8번에
연결하면 점퍼선이 서로 엇갈려 꼬이기 때문에, 반대로 배정해서 1번 스위치 점퍼선이 가장
먼 핀(115번)에서 시작해 안쪽으로 나란히 들어오도록 했습니다:

| 스위치 | GPIO | A Side 핀 |
|---|---|---|
| 1번 | GPIO25 | 115번 |
| 2번 | GPIO24 | 113번 |
| 3번 | GPIO23 | 111번 |
| 4번 | GPIO22 | 109번 |
| 5번 | GPIO21 | 107번 |
| 6번 | GPIO20 | 105번 |
| 7번 | GPIO19 | 103번 |
| 8번 | GPIO18 | 101번 |

![F28P65x 모듈 GPIO18~25 - 개발보드 V2 (7) 범용 TOGGLE 스위치 8채널 배선도](f28xevm_v2_toggle8.png)

위 배선도의 TOGGLE 8채널 행 1~8번이 왼쪽부터 순서대로 핀 115/113/111/109/107/105/103/101번에
연결되는 것을 보면, 점퍼선이 서로 교차하지 않고 나란히 들어가는 걸 확인할 수 있습니다 —
이게 스위치 번호를 핀 순서와 반대로 배정한 이유입니다.

## 소프트웨어 버전
CCS 21.x / **SysConfig 미사용** / C2000Ware 26.00.00.00 driverlib(로컬 복사) / CGT 22.6.3.LTS.
`device.h`/`device.c`/`driverlib.h`/driverlib 헤더 전체/`driverlib.lib`를 전부 프로젝트
폴더 안에 복사해 두었으므로, C2000Ware 설치 경로와 무관하게 빌드됩니다.

## 동작 원리

### 1. 토글 스위치는 "이벤트"가 아니라 "상태"를 나타냅니다
[6_GP_Tactile4](../../6_GP_Tactile4_Driverlib/README.md)의 Tactile 스위치는 손을
떼면 원래대로 돌아오는 순간접점(모멘터리) 스위치라서 "눌림"이라는 **이벤트**를
감지하는 용도로 씁니다. 반면 토글 스위치는 한 번 젖히면 그 자리를 유지하는
**래칭(Latching)** 스위치입니다 — 그래서 토글 스위치가 표현하는 건 "지금 이
순간 무슨 일이 일어났는가"가 아니라 "지금 이 스위치가 어느 쪽에 있는가"라는
**정적인 설정값**입니다. 이 예제의 `toggleSwitchState`에 에지 검출이나
`pressCount` 같은 카운터가 없는 이유가 여기에 있습니다 — 매 순간의 "레벨 값"
자체가 그대로 의미 있는 정보이기 때문입니다.

### 2. 실무에서는 보통 "부팅 시 한 번만" 읽습니다
이 예제는 시연을 위해 20msec마다 계속 폴링하지만, 실제 제품에서 토글 스위치
8개는 대개 **전원을 켤 때(또는 리셋 직후) 딱 한 번만 읽어서** 그 값을 그대로
하드웨어 설정값으로 씁니다. 예를 들면:
- **통신 노드 주소/ID 설정**: SCI/CAN/I2C 예제에서 보드마다 다른 노드 ID(0~255)를
  하드웨어로 지정 — 펌웨어 재빌드 없이 여러 보드를 구분할 수 있습니다.
- **동작 모드 선택**: 모터 제어 예제에서 개방루프/폐루프 모드, PWM 프리셋 등을
  스위치 조합으로 고름.
- **기능 On/Off**: 디버그 로그 출력, 특정 안전 인터록 활성화 여부 등을 스위치
  하나로 켜고 끔.

이런 용도로 쓸 때는 이 예제의 `initSwitchGpio()` + 한 번의 `GPIO_readPin()` 호출
부분만 그대로 가져다 쓰면 됩니다 — 부팅 루틴 중 딱 한 번 `toggleSwitchState`를
읽어서 전역 설정 변수에 저장해두면, 메인 루프의 폴링/딜레이 부분은 필요 없습니다.

### 3. 왜 이 경우엔 디바운스가 문제되지 않는가
Tactile 스위치와 똑같이 기계식 접점이라 토글 스위치도 젖히는 순간 바운스(수
msec 동안 접점이 튕기는 현상, 자세한 설명은
[6_GP_Tactile4_Driverlib의 "동작 원리"](../../6_GP_Tactile4_Driverlib/README.md#동작-원리)
참고)가 있습니다. 하지만 "부팅 시 1회 읽기" 방식에서는 문제가 되지 않습니다 —
이미 스위치를 젖혀서 안정된 지 한참 지난 뒤(전원 인가 후 최소 수백 msec~수초
뒤)에 딱 한 번 읽기 때문에, 그 짧은 바운스 구간은 이미 끝난 뒤입니다. 이
예제처럼 "계속 폴링하며 매 순간 상태를 갱신"하는 방식이라면 스위치를 젖히는
그 순간 `toggleSwitchState`가 짧게 튈 수는 있지만, 값을 "카운트"하지 않고 매
순간 최신값만 보여주므로 실질적인 문제로 이어지지는 않습니다.

## Import → Build → Flash → Run
1. CCS에서 `CCS/7_GP_Toggle8_Driverlib.projectspec`를 Import — 압축을 미리 풀어서
   "Select search-directory"로 폴더를 지정하거나, zip 파일을 그대로 "Select archive
   file"로 지정해도 됩니다(둘 다 정상 동작 — 아래 참고).
2. Build (CPU1_RAM 또는 CPU1_FLASH)
3. Debug 연결 후 Flash/Run
4. `.ccxml`은 `TMS320F28P650DK9.ccxml`을 그대로 사용 — F28P659DK8-Q1 최초 연결 시 정상
   인식 확인 필요

> **고친 zip-import 버그**: 예전엔 `driverlib.lib`를 `.projectspec`에 `action="link"`로
> 지정해서, GitHub에서 zip을 받아 CCS "Select archive file"로 바로 import하면
> `driverlib.lib`가 **unresolved**로 뜨는 문제가 있었습니다 — CCS가 zip을 임시 폴더
> (`...\AppData\Local\Temp\ccs-import-XXXXXX\`)에 풀고 나서 그 임시 경로를 가리키는
> 링크를 만드는데, 임시 폴더가 정리되면 링크가 끊어지기 때문입니다(압축을 미리 풀어서
> 폴더로 import하면 그 폴더가 안 지워지니 문제가 없었습니다). `device/` 폴더처럼
> `action="copy"`로 바꿔서 완전히 해결했습니다.

## 정상 동작 확인
- 8개 토글 스위치를 켜고 끌 때, 스위치 옆의 LED Indicator(매뉴얼 기준 보드 내장)가
  즉시 반응하면 배선이 맞는 것입니다.
- CCS Expressions 창에 `toggleSwitchState`(비트0=1번 스위치=GPIO25 ~ 비트7=8번 스위치=GPIO18), `pollCount`를
  추가하고 Continuous Refresh를 켜서, 스위치를 하나씩 눌러보며 해당 비트가 바뀌는지
  확인하세요.

## 세 가지 버전 비교
| 버전 | 폴더 | 핵심 차이 |
|---|---|---|
| **DriverLib(이 폴더)** | 7_GP_Toggle8_Driverlib | TI 표준 HAL 함수 호출 |
| 비트필드 | [7_GP_Toggle8_Bitfield](../7_GP_Toggle8_Bitfield/) | 레지스터 구조체 직접 조작, DriverLib 미사용 |
| FreeRTOS | [7_GP_Toggle8_Freertos](../7_GP_Toggle8_Freertos/) | DriverLib + 태스크 스케줄링, 정적 할당 |

### 메모리 실측 비교 (2026-09-16, CCS 21.x, CPU1_RAM 빌드, `.map` 기준)

세 프로젝트 모두 `workspace_ccstheia`에서 실제로 Import → Build까지 성공한 뒤의 `.map`
파일 MODULE SUMMARY "Grand Total"(code/ro data/rw data) 기준 실측치입니다.

| 버전 | code | ro data | rw data | 합계 |
|---|---:|---:|---:|---:|
| **DriverLib(이 폴더)** | 3,023 B | 604 B | 1,030 B (스택 1,016B) | **4,657 B** |
| 비트필드 | 3,778 B | 482 B | 1,685 B (스택 256B) | **5,945 B** |
| FreeRTOS | 5,139 B | 821 B | 1,239 B (RTS 스택 512B + 태스크 정적 스택 512B + 전역변수 215B) | **7,199 B** |

비트필드 버전의 rw data가 유독 큰 건 클래식 `f28p65x_globalvariabledefs.c`가 주변장치
레지스터 구조체 전체를 항상 전역 심볼로 선언해서(실제 추가 RAM 소비 없이 기존 레지스터
주소를 덮어씌운 것) 링커가 "rw data"로 잡기 때문입니다 — DriverLib은 이런 이름 붙은
전역 구조체 없이 주소 매크로만 써서 이 항목이 없습니다.

## 관련 링크
- 상품 페이지: https://tms320f28x.co.kr/goods/goods_view.php?goodsNo=200903127
- 게시판 글: https://tms320f28x.co.kr/board/view.php?bdId=tms320f28xevmv2&sno=106
- 유튜브 영상: (게시 후 URL 추가 예정)
