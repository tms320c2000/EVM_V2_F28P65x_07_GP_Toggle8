# TMS320F28X 개발보드 V2 — F28P65x 범용 Toggle 스위치 8채널 예제

[TMS320F28X 범용 개발보드 V2](https://tms320f28x.co.kr/goods/goods_view.php?goodsNo=200903127)의
회로블록 **(7) 범용 Toggle 스위치 8채널**을 F28P65x 모듈(GPIO18~25)로 읽는 예제입니다. 같은
동작(8개 스위치 상태 폴링)을 세 가지 방식으로 각각 구현해서 코드량·메모리 사용량을 비교할
수 있게 만들었습니다.

## 포함된 프로젝트

| 프로젝트 | 설명 |
|---|---|
| [7_GP_Toggle8_Bitfield](7_GP_Toggle8_Bitfield/) | 레지스터 비트필드 직접 제어 (DriverLib 미사용) |
| [7_GP_Toggle8_Driverlib](7_GP_Toggle8_Driverlib/) | TI DriverLib 사용 |
| [7_GP_Toggle8_Freertos](7_GP_Toggle8_Freertos/) | FreeRTOS 태스크로 구현 (정적 할당, 힙 미사용) |

각 폴더는 자기완결형(self-contained) CCS 프로젝트입니다 — 폴더 하나만 받아도 C2000Ware/
DriverLib 등 필요한 파일이 전부 로컬에 포함되어 있어 Import → Build가 됩니다. 토글(래칭)
스위치가 순간접점 스위치와 개념적으로 어떻게 다른지, 실무에서 노드 주소·동작 모드 설정에
어떻게 쓰이는지는 각 폴더의 README.md "동작 원리" 절에서 자세히 설명합니다. 실측 메모리
비교도 각 폴더의 README.md를 참고하세요.

## 배선

개발보드 A-Side 핀-헤더 101, 103, 105, 107, 109, 111, 113, 115번(GPIO18~25)을 (7) 범용
Toggle 스위치 8채널 블록에 연결합니다. 점퍼선 꼬임을 막기 위해 스위치 번호와 GPIO 대응이
핀 순서와 반대입니다(1번 스위치=GPIO25/115번 핀 ~ 8번 스위치=GPIO18/101번 핀).

![F28P65x 모듈 GPIO18~25 - 개발보드 V2 (7) 범용 TOGGLE 스위치 8채널 배선도](7_GP_Toggle8_Driverlib/f28xevm_v2_toggle8.png)

## 프로세서 모듈

- [TMS320F28P650DK9 모듈(산업용)](https://tms320f28x.co.kr/goods/goods_view.php?goodsNo=200903200)
- [TMS320F28P659DK8-Q1 모듈(차량 전장용)](https://tms320f28x.co.kr/goods/goods_view.php?goodsNo=200903201)

## 개발 환경

CCS 21.x(Theia 기반) / TI CGT 22.6.3.LTS / C2000Ware 26.00.00.00
