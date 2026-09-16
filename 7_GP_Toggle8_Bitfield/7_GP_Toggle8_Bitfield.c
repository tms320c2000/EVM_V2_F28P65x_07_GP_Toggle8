// 파일이름	:	7_GP_Toggle8_Bitfield.c
// 대상장치	:	TMS320F28X EVM V2, TMS320F28P65x(F28P650DK9 산업용 / F28P659DK8-Q1 차량용) 모듈
// 파일버전	:	1.10
// 갱신이력	:	2026-09-15, 버전 1.00
//				2026-09-16, 버전 1.10 - 배선을 A Side 97~111번(GPIO16~23)에서 101~115번
//				(GPIO18~25)으로 옮기고, 점퍼선 꼬임 방지를 위해 스위치 번호도 반대로
//				배정 (1번=GPIO25 ~ 8번=GPIO18)
// 예제설명	:

//************************************************************************************************************************************************************************
//
// 본 예제는 TMS320F28P65x 모듈이 탑재된 TMS320F28X 개발보드(EVM) V2를 대상으로 하고 있으며,
// TMS320F28P65x 칩의 GPI/O 회로를 통해 개발보드의 범용 Toggle 스위치 8채널을 읽어냅니다.
//
// TMS320F28X 개발보드 V2의 '(7) 범용 Toggle 스위치 8채널' 회로는 스위치 ON = Active High이며,
// 스위치 옆의 LED Indicator로도 상태를 즉시 육안 확인할 수 있습니다.
//
// a.	예제 실행을 위해 아래와 같이 개발보드 Toggle 스위치 회로의 핀-헤더와
//		TMS320F28P65x 칩의 GPIO 포트를 연결하세요.
//
//			>> 1번 스위치 : GPIO 25번 (A Side 115번 핀)		>> 5번 스위치 : GPIO 21번 (A Side 107번 핀)
//			>> 2번 스위치 : GPIO 24번 (A Side 113번 핀)		>> 6번 스위치 : GPIO 20번 (A Side 105번 핀)
//			>> 3번 스위치 : GPIO 23번 (A Side 111번 핀)		>> 7번 스위치 : GPIO 19번 (A Side 103번 핀)
//			>> 4번 스위치 : GPIO 22번 (A Side 109번 핀)		>> 8번 스위치 : GPIO 18번 (A Side 101번 핀)
//
//		(핀 번호와 스위치 번호가 반대 순서인 이유: A Side 101,103,...,115번 핀 순서
//		그대로 8,7,...,1번을 배정하면 점퍼선 8개가 서로 엇갈려서 꼬입니다. 반대로
//		배정하면 1번 스위치 점퍼선이 가장 먼 115번 핀에서 시작해 안쪽으로 나란히
//		들어오므로 선이 꼬이지 않습니다.)
//
// 예제 폴더의 CCS Expressions 창에 아래 변수를 등록해두면 동작을 실시간으로 확인할 수 있습니다.
// --> toggleSwitchState(8개 스위치 상태를 비트0(1번 스위치=GPIO25)~비트7(8번 스위치=GPIO18)로 담은 값),
//     pollCount(폴링 루프 실행 횟수, 정상 동작 확인용)
//
// 본 예제는 28X 칩 MMR(Memory Mapped Register)을 직접 조작하는 TI의 Bit-Field Approach
// 만으로 제작되었습니다 (Driverlib 미사용, driverlib.lib 자체가 이 프로젝트에는 없습니다).
// 같은 동작을 DriverLib API로 구현한 7_GP_Toggle8_Driverlib.c, FreeRTOS 태스크로 구현한
// 7_GP_Toggle8_Freertos.c와 함께 세 가지 방식을 비교해 보실 수 있습니다.
//
//************************************************************************************************************************************************************************


// 헤더 파일들
#include "f28x_project.h"		// TI 제공 칩-지원 헤더 통합 Include 용 헤더파일 (bit-field)

// 전처리 구문 정의
#define	NUM_SWITCHES	8U		// 읽어들일 스위치 개수

// 함수 원형 선언


// 전역 변수 선언
volatile Uint16	toggleSwitchState;	// 8개 스위치 상태를 비트0(1번 스위치=GPIO25)~비트7(8번 스위치=GPIO18)로 담은 값
volatile Uint32	pollCount;			// 폴링 루프 실행 횟수(정상 동작 확인용)

// 이 배열의 인덱스 = 비트 번호(스위치 번호-1), 값 = GPIO 번호. 개발보드 A Side 핀-헤더
// 101~115번(홀수, GPIO18~25)과 일부러 반대 순서로 배정했습니다 - 점퍼선 꼬임 방지(파일
// 상단 배선 설명 참고). GPIO18~25는 모두 GpioDataRegs.GPADAT(GPIO0~31) 한 레지스터
// 안에 들어있어서, 루프마다 .all을 한 번만 읽고 비트를 뽑아 쓰면 충분합니다.
static const Uint16 switchGpio[NUM_SWITCHES] =
{
    25U, 24U, 23U, 22U, 21U, 20U, 19U, 18U
};


// 메인 함수
void main(void)
{

//	1. 전역 인터럽트 스위치 OFF, CPU 인터럽트 벡터 비-활성화 및 플래그(Flag) 비트 클리어
	DINT;			// 전역 인터럽트 스위치 OFF (/INTM OFF)
	IER = 0x0000;	// CPU 인터럽트 벡터 비-활성화
	IFR = 0x0000;	// CPU 인터럽트 플래그 클리어


//	2. 시스템 초기화 - InitSysCtrl( ) 함수 호출 (f28p65x_sysctrl.c)
//	* 왓치독 타이머 비-활성화
//	* CPU 클럭 주파수 설정 (PLL)
//	* 주변회로 클럭 공급 설정
	InitSysCtrl();

//	* 범용 입출력 포트(GPIO) 설정 - InitGpio( ) 함수 호출 후, 스위치 8개용 GPIO18~25를
//	  하나씩 입력(내부 풀업, 동기 입력)으로 개별 설정
	{
	    Uint16 i;
	    InitGpio();
	    for(i = 0U; i < NUM_SWITCHES; i++)
	    {
	        GPIO_SetupPinMux(switchGpio[i], GPIO_MUX_CPU1, 0);
	        GPIO_SetupPinOptions(switchGpio[i], GPIO_INPUT, GPIO_PULLUP);
	    }
	}


//	3. 주변회로 인터럽트 확장회로 초기화 - InitPieCtrl( ) 함수 호출 (f28p65x_piectrl.c)
	InitPieCtrl();


//	4. 주변회로 인터럽트 벡터 확장 및 복사 실행 - InitPieVectTable( ) 함수 호출 (f28p65x_pievect.c)
	InitPieVectTable();


//	5. 인터럽트 벡터와 인터럽트 서비스 루틴 재-연결, 인터럽트 벡터 활성화
//	   (본 예제는 인터럽트를 사용하지 않습니다)


//	6. 주변회로 초기화
//	   (스위치용 GPIO 설정은 2번에서 이미 처리했으므로 생략)


//	7. 전역 변수 및 S/W 모듈 초기화
	toggleSwitchState = 0U;
	pollCount = 0UL;


//	8. 실시간 디버깅 활성화, 전역 인터럽트 스위치 ON
	ERTM;	// Debug Enable Mask 비트 설정 (실시간 디버깅이 가능하도록 ST1 레지스터의 /DBGM 비트를 0으로 클리어)
	EINT;	// 전역 인터럽트 스위치 ON (/INTM ON)


//	9. Idle(Background) Loop
	for(;;)
	{
		Uint16 i;
		Uint16 state = 0U;
		Uint32 gpadat = GpioDataRegs.GPADAT.all;	// GPIO0~31을 한 번에 읽음

		for(i = 0U; i < NUM_SWITCHES; i++)
		{
			if((gpadat & (1UL << switchGpio[i])) != 0UL)
			{
				state |= (Uint16)(1U << i);
			}
		}

		toggleSwitchState = state;
		pollCount++;

		DELAY_US(20000);	// 20msec 지연 (폴링 주기)
	}
}

//	10. 인터럽트 서비스 루틴 및 기타 함수들
//	    (본 예제는 인터럽트 서비스 루틴 및 별도 함수가 없습니다)


// 파일 끝.
