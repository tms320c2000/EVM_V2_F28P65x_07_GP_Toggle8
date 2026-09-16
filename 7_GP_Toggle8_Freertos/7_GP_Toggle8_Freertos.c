// 파일이름	:	7_GP_Toggle8_Freertos.c
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
// 본 예제는 7_GP_Toggle8_Driverlib.c와 완전히 동일한 DriverLib GPIO 제어 코드를 쓰되,
// "메인 루프에서 직접 지연시간을 기다리는" 대신 FreeRTOS 태스크 하나가 스케줄러 위에서
// 도는 방식으로 구현했습니다. 힙(heap)을 전혀 쓰지 않는 정적 할당 구성입니다
// (configSUPPORT_DYNAMIC_ALLOCATION=0, xTaskCreateStatic 사용). 레지스터를 직접 조작하는
// Bit-Field 방식의 7_GP_Toggle8_Bitfield.c와 함께 세 가지 방식을 비교해 보실 수 있습니다.
//
//************************************************************************************************************************************************************************


// 헤더 파일들
#include "driverlib.h"		// TI 제공 Driver API Library 헤더파일 (driverlib)
#include "device.h"
#include "FreeRTOS.h"
#include "task.h"

// 전처리 구문 정의
#define	NUM_SWITCHES	8U		// 읽어들일 스위치 개수
#define	STACK_SIZE	256U	// 태스크 스택 크기, 워드 단위 (C28x는 1워드=16비트이므로 512바이트)

// 함수 원형 선언
void initSwitchGpio(void);
void ToggleSwitch_Task(void *pvParameters);


// 전역 변수 선언
volatile uint16_t	toggleSwitchState;	// 8개 스위치 상태를 비트0(1번 스위치=GPIO25)~비트7(8번 스위치=GPIO18)로 담은 값
volatile uint32_t	pollCount;			// 폴링 루프 실행 횟수(정상 동작 확인용)

// 이 배열의 인덱스 = 비트 번호(스위치 번호-1), 값 = GPIO 번호. 개발보드 A Side 핀-헤더
// 101~115번(홀수, GPIO18~25)과 일부러 반대 순서로 배정했습니다 - 점퍼선 꼬임 방지(파일
// 상단 배선 설명 참고).
static const uint16_t switchGpio[NUM_SWITCHES] =
{
    25U, 24U, 23U, 22U, 21U, 20U, 19U, 18U
};

// FreeRTOS 태스크용 정적 메모리 선언 - 스위치 태스크 1개 + Idle 태스크 1개, 전부 정적
// 할당(힙 미사용)입니다.
static StaticTask_t switchTaskBuffer;
static StackType_t  switchTaskStack[STACK_SIZE];
#pragma DATA_SECTION(switchTaskStack, ".freertosStaticStack")
#pragma DATA_ALIGN(switchTaskStack, portBYTE_ALIGNMENT)

static StaticTask_t idleTaskBuffer;
static StackType_t  idleTaskStack[STACK_SIZE];
#pragma DATA_SECTION(idleTaskStack, ".freertosStaticStack")
#pragma DATA_ALIGN(idleTaskStack, portBYTE_ALIGNMENT)


// 메인 함수
void main(void)
{

//	1. 전역 인터럽트 스위치 OFF, CPU 인터럽트 벡터 비-활성화 및 플래그(Flag) 비트 클리어
//	   (DriverLib에서는 별도 처리 없이, 3번의 Interrupt_initModule( ) 함수가 한 번에
//	   담당합니다 — 아래 3번 참고)


//	2. 시스템 초기화 - Device_init( ) 함수 호출
//	* 왓치독 타이머 비-활성화
//	* CPU 클럭 주파수 설정 (PLL)
//	* 주변회로 클럭 공급 설정
	Device_init();

//	* 범용 입출력 포트(GPIO) 핀 락 해제, 내부 풀업 활성화 - Device_initGPIO( ) 함수 호출
	Device_initGPIO();


//	3. 주변회로 인터럽트 확장회로 초기화 - Interrupt_initModule( ) 함수 호출
//	   (전역 인터럽트 비활성화 + PIE 인터럽트 인에이블/플래그 클리어까지 이 함수 하나로
//	   처리합니다 — 클래식 Bit-Field 버전의 1번+3번을 합친 것과 같습니다)
	Interrupt_initModule();


//	4. 주변회로 인터럽트 벡터 확장 및 복사 실행 - Interrupt_initVectorTable( ) 함수 호출
	Interrupt_initVectorTable();


//	5. 인터럽트 벡터와 인터럽트 서비스 루틴 재-연결, 인터럽트 벡터 활성화
//	   (본 예제는 인터럽트를 사용하지 않습니다)


//	6. 주변회로 초기화 - GPIO18~GPIO25를 입력으로 설정, initSwitchGpio( ) 함수 호출 (본 파일 하단)
	initSwitchGpio();


//	7. 전역 변수 및 S/W 모듈 초기화
	toggleSwitchState = 0U;
	pollCount = 0U;


//	8. 실시간 디버깅 활성화, 전역 인터럽트 스위치 ON
	ERTM;	// Debug Enable Mask 비트 설정 (실시간 디버깅이 가능하도록 ST1 레지스터의 /DBGM 비트를 0으로 클리어)
	EINT;	// 전역 인터럽트 스위치 ON (/INTM ON)


//	9. FreeRTOS 태스크 생성 및 스케줄러 시작
//	   (Idle Loop를 직접 돌리지 않고, ToggleSwitch_Task( )를 스케줄러에 맡깁니다 — 아래 10번 참고)
	xTaskCreateStatic(ToggleSwitch_Task,		// 태스크 함수
	                  "ToggleSwitch Task",		// 이름(디버깅용)
	                  STACK_SIZE,				// 스택 크기(워드)
	                  NULL,						// 파라미터 없음
	                  tskIDLE_PRIORITY + 1,
	                  switchTaskStack,
	                  &switchTaskBuffer);

	vTaskStartScheduler();		// 이 아래로는 절대 돌아오지 않음

	for(;;)
	{
	    // 여기 도달하면 스케줄러 시작 실패 (메모리 부족 등)
	}
}

//	10. 인터럽트 서비스 루틴 및 기타 함수들

//
// ToggleSwitch_Task - Idle(Background) Loop에 해당하는 실제 동작. DriverLib 버전의
// 메인 루프와 완전히 같은 로직이며, DEVICE_DELAY_US busy-wait 대신 vTaskDelay로 다른
// 태스크(Idle)에게 CPU를 양보한다는 점만 다릅니다.
//
void ToggleSwitch_Task(void *pvParameters)
{
    (void)pvParameters;

    for(;;)
    {
        uint16_t i;
        uint16_t state = 0U;

        for(i = 0U; i < NUM_SWITCHES; i++)
        {
            if(GPIO_readPin(switchGpio[i]) != 0U)
            {
                state |= (uint16_t)(1U << i);
            }
        }

        toggleSwitchState = state;
        pollCount++;

        vTaskDelay(20 / portTICK_PERIOD_MS);	// 20msec 지연 (폴링 주기)
    }
}

//
// initSwitchGpio - GPIO18~GPIO25를 전부 입력(내부 풀업, 동기 입력)으로 설정. DriverLib
// 버전(7_GP_Toggle8_Driverlib.c)과 설정 코드를 완전히 동일하게 맞춰서, 두 예제의 차이가
// "메인 루프를 어떻게 도는가"(딜레이 방식 + RTOS 유무)에만 있도록 했습니다.
//
void initSwitchGpio(void)
{
    GPIO_setPadConfig(18U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_18_GPIO18);
    GPIO_setDirectionMode(18U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(18U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(19U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_19_GPIO19);
    GPIO_setDirectionMode(19U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(19U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(20U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_20_GPIO20);
    GPIO_setDirectionMode(20U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(20U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(21U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_21_GPIO21);
    GPIO_setDirectionMode(21U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(21U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(22U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_22_GPIO22);
    GPIO_setDirectionMode(22U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(22U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(23U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_23_GPIO23);
    GPIO_setDirectionMode(23U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(23U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(24U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_24_GPIO24);
    GPIO_setDirectionMode(24U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(24U, GPIO_QUAL_SYNC);

    GPIO_setPadConfig(25U, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPinConfig(GPIO_25_GPIO25);
    GPIO_setDirectionMode(25U, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(25U, GPIO_QUAL_SYNC);
}

//
// vApplicationStackOverflowHook - FreeRTOS가 스택오버플로우를 감지하면 호출
//
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    for(;;) { }
}

//
// vApplicationGetIdleTaskMemory - configSUPPORT_STATIC_ALLOCATION=1 이면 Idle 태스크
// 메모리도 애플리케이션이 직접 제공해야 함(힙을 안 쓰므로)
//
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &idleTaskBuffer;
    *ppxIdleTaskStackBuffer = idleTaskStack;
    *pulIdleTaskStackSize = STACK_SIZE;
}

// 파일 끝.
