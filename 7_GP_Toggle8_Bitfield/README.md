# 토글 스위치 8채널 테스트 — 비트필드(레지스터 직접 제어) 버전 — TMS320F28P659DK8-Q1

[7_GP_Toggle8_Driverlib](../7_GP_Toggle8_Driverlib/README.md)(DriverLib 버전)와 동일한 동작을
**DriverLib을 전혀 쓰지 않고** TI의 클래식 레지스터 비트필드 구조체(`GpioDataRegs`)만으로
구현했습니다. `driverlib.lib` 자체가 이 프로젝트에는 없습니다.

## 이 버전의 핵심
- 시스템 초기화: `InitSysCtrl()` (DriverLib의 `Device_init()`에 대응, 클래식 헬퍼)
- GPIO 입력 설정: `GPIO_SetupPinMux(gpio, GPIO_MUX_CPU1, 0)` + `GPIO_SetupPinOptions(gpio,
  GPIO_INPUT, GPIO_PULLUP)` — DriverLib 버전의 `GPIO_setPinConfig()`/`GPIO_setDirectionMode()`/
  `GPIO_setPadConfig()` 조합과 대응.
- **스위치 상태 읽기**: `GpioDataRegs.GPADAT.all`(GPIO0~31을 담당하는 32비트 레지스터)을
  루프마다 한 번만 읽고 비트 시프트로 8개 채널을 한꺼번에 뽑아냅니다 — DriverLib 버전의
  `GPIO_readPin()`을 채널마다 8번 호출하는 것과 실질적으로 같은 결과지만, 레지스터를 직접
  다루면 이렇게 한 번에 처리할 수도 있다는 걸 보여주는 지점입니다.

## 요구 하드웨어 / 배선
[7_GP_Toggle8_Driverlib](../7_GP_Toggle8_Driverlib/README.md)와 완전히 동일. **v1.10부터
스위치 번호와 GPIO 대응이 핀 순서와 반대입니다** — 점퍼선 꼬임 방지 목적:

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

![F28P65x 모듈 GPIO18~25 - 개발보드 V2 (7) 범용 TOGGLE 스위치 8채널 배선도](../7_GP_Toggle8_Driverlib/f28xevm_v2_toggle8.png)

## 소프트웨어 버전
CCS 21.x / **SysConfig·C2000Ware 라이브러리 링크 없음** (products="C2000WARE"만 사용) /
CGT 22.6.3.LTS. `device/common_include`, `device/headers_include`에 필요한 클래식 헤더를
전부 복사해 자기완결형으로 만들었습니다.

## 동작 원리

### 1. 토글 스위치는 "이벤트"가 아니라 "상태"를 나타냅니다
[6_GP_Tactile4](../../6_GP_Tactile4_Bitfield/README.md)의 Tactile 스위치는 손을
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

이런 용도로 쓸 때는 이 예제의 `initSwitchGpio()` + 한 번의 레지스터 읽기
부분만 그대로 가져다 쓰면 됩니다 — 부팅 루틴 중 딱 한 번 `GpioDataRegs.GPADAT.all`을
읽어서 전역 설정 변수에 저장해두면, 메인 루프의 폴링/딜레이 부분은 필요 없습니다.

### 3. 왜 이 경우엔 디바운스가 문제되지 않는가
Tactile 스위치와 똑같이 기계식 접점이라 토글 스위치도 젖히는 순간 바운스(수
msec 동안 접점이 튕기는 현상, 자세한 설명은
[6_GP_Tactile4_Bitfield의 "동작 원리"](../../6_GP_Tactile4_Bitfield/README.md#동작-원리)
참고)가 있습니다. 하지만 "부팅 시 1회 읽기" 방식에서는 문제가 되지 않습니다 —
이미 스위치를 젖혀서 안정된 지 한참 지난 뒤(전원 인가 후 최소 수백 msec~수초
뒤)에 딱 한 번 읽기 때문에, 그 짧은 바운스 구간은 이미 끝난 뒤입니다. 이
예제처럼 "계속 폴링하며 매 순간 상태를 갱신"하는 방식이라면 스위치를 젖히는
그 순간 `toggleSwitchState`가 짧게 튈 수는 있지만, 값을 "카운트"하지 않고 매
순간 최신값만 보여주므로 실질적인 문제로 이어지지는 않습니다.

## Import → Build → Flash → Run
1. CCS에서 `CCS/7_GP_Toggle8_Bitfield.projectspec`를 Import
2. Build (CPU1_RAM 또는 CPU1_FLASH)
3. Debug 연결 후 Flash/Run — `.ccxml`은 `TMS320F28P650DK9.ccxml` 사용 (DK8-Q1 최초 연결 시
   정상 인식 확인 필요)

## 정상 동작 확인
DriverLib 버전과 동일하게, 8개 토글 스위치를 켜고 끌 때 스위치 옆 LED Indicator가 즉시
반응하면 배선이 맞는 것입니다. `toggleSwitchState`(비트0=1번 스위치=GPIO25 ~ 비트7=8번 스위치=GPIO18),
`pollCount`도 CCS Expressions에서 동일하게 확인 가능합니다.

## 세 가지 버전 비교
| 버전 | 폴더 | 핵심 차이 |
|---|---|---|
| DriverLib | [7_GP_Toggle8_Driverlib](../7_GP_Toggle8_Driverlib/) | TI 표준 HAL 함수 호출 |
| **비트필드(이 폴더)** | 7_GP_Toggle8_Bitfield | 레지스터 구조체 직접 조작, DriverLib 미사용 |
| FreeRTOS | [7_GP_Toggle8_Freertos](../7_GP_Toggle8_Freertos/) | DriverLib + 태스크 스케줄링, 정적 할당 |

### 메모리 실측 비교 (2026-09-16, CCS 21.x, CPU1_RAM 빌드, `.map` 기준)

세 프로젝트 모두 `workspace_ccstheia`에서 실제로 Import → Build까지 성공한 뒤의 `.map`
파일 MODULE SUMMARY "Grand Total"(code/ro data/rw data) 기준 실측치입니다.

| 버전 | code | ro data | rw data | 합계 |
|---|---:|---:|---:|---:|
| DriverLib | 3,023 B | 604 B | 1,030 B (스택 1,016B) | **4,657 B** |
| **비트필드(이 폴더)** | 3,778 B | 482 B | 1,685 B (스택 256B) | **5,945 B** |
| FreeRTOS | 5,139 B | 821 B | 1,239 B (RTS 스택 512B + 태스크 정적 스택 512B + 전역변수 215B) | **7,199 B** |

비트필드 버전의 rw data(1,685B)가 유독 큰 건 태스크 코드 때문이 아니라, 클래식
`f28p65x_globalvariabledefs.c`가 `GpioCtrlRegsFile`/`CpuSysRegsFile` 등 주변장치
레지스터 구조체 전체를 예제가 실제로 쓰든 안 쓰든 항상 전역 심볼로 선언해서 링커가
"rw data"로 잡기 때문입니다(실제로 추가 RAM을 소비하는 게 아니라 이미 있는 레지스터
주소를 심볼로 덮어씌운 것 — 물리적으로는 공짜입니다). DriverLib/FreeRTOS 버전은 이런
이름 붙은 전역 구조체 없이 주소 매크로만 쓰기 때문에 이 항목이 없습니다.

## 관련 링크
- 상품 페이지: https://tms320f28x.co.kr/goods/goods_view.php?goodsNo=200903127
- 게시판 글: (게시 후 URL 추가 예정)
- 유튜브 영상: (게시 후 URL 추가 예정)
