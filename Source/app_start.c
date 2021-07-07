/****************************************************************************
 *
 * MODULE:              Lumi Router
 *
 * COMPONENT:           app_start.c
 *
 * DESCRIPTION:         Router Initialisation
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2017. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/* Generated */
#include "pdum_gen.h"

/* Application */
#include "app_main.h"
#include "app_router_node.h"
#include "uart.h"

/* SDK JN-SW-4170 */
#include "AppApi.h"
#include "AppHardwareApi.h"
#include "PDM.h"
#include "bdb_api.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pwrm.h"
#include "zps_nwk_pub.h"

#include <stdio.h>
#include <stdlib.h>

#include "AppHardwareApi.h"

#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)

#define BOARD_BTN_BIT               (0)
#define BOARD_BTN_PIN               (1UL << BOARD_BTN_BIT)

ZTIMER_tsTimer timers[1];
uint8 blinkTimerHandle;
uint8 buttonScanTimerHandle;

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_APP
#define TRACE_APP TRUE
#else
#define TRACE_APP FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void APP_vInitialise(void);
PRIVATE void vfExtendedStatusCallBack(ZPS_teExtendedStatus eExtendedStatus);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern void *_stack_low_water_mark;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

typedef enum
{
	BUTTON_SHORT_PRESS,
	BUTTON_LONG_PRESS
} ButtonPressType;

ButtonPressType queue[3];
tszQueue queueHandle;

PUBLIC void buttonScanFunc(void *pvParam)
{
	static int duration = 0;

	uint32 input = u32AHI_DioReadInput();
	bool btnState = (input & BOARD_BTN_PIN) == 0;

	if(btnState)
	{
		duration++;
	}
	else
	{
		// detect long press
		if(duration > 200)
		{
			DBG_vPrintf(TRUE, "Button released. Long press detected\n");
            DBG_vPrintf(TRACE_APP, "Leave -> Reset Data Structures\n");
            //APP_vFactoryResetRecords();
            //vAHI_SwReset();
            DBG_vPrintf(TRUE, "Erase PDM.......\n");
			PDM_vDeleteAllDataRecords();
			DBG_vPrintf(TRUE, "Reset...........\n");
			APP_WriteMessageToSerial("FACTORY RESET\n");
			//ZTIMER_eStart(u8TimerRestart, ZTIMER_TIME_MSEC(100));
			vAHI_SwReset();
			ButtonPressType value = BUTTON_LONG_PRESS;
			ZQ_bQueueSend(&queueHandle, (uint8*)&value);
		}

		// detect short press
		else if(duration > 5)
		{
			DBG_vPrintf(TRUE, "Button released. Short press detected\n");
			ButtonPressType value = BUTTON_SHORT_PRESS;
			ZQ_bQueueSend(&queueHandle, &value);
		}

		duration = 0;
	}

	ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));
}

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 * ---
 * Called in SDK JN-SW-4170
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{
    /* Wait until FALSE i.e. on XTAL - otherwise UART data will be at wrong speed */
    while (bAHI_GetClkSource() == TRUE)
        ;

    /* Move CPU to 32 MHz; vAHI_OptimiseWaitStates automatically called */
    bAHI_SetClockRate(3);

#ifdef UART_DEBUGGING
    /* Initialise the debug diagnostics module to use UART1 at 115K Baud */
    DBG_vUartInit(DBG_E_UART_1, DBG_E_UART_BAUD_RATE_115200);
#endif

    /* Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size. */
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);

    /* Catch resets due to watchdog timer expiry. Comment out to harden code. */
    if (bAHI_WatchdogResetEvent()) {
        DBG_vPrintf(TRACE_APP, "APP: Watchdog timer has reset device!\n");
        DBG_vDumpStack();
    }

#ifdef ENABLING_HIGH_POWER_MODE
    /* After testing on Xiaomi DGNWG05LM and Aqara ZHWG11LM devices, it was
     * decided to use the deprecated vAppApiSetHighPowerMode method for use on
     * JN5168 instead of the new vAHI_ModuleConfigure method for use on JN5169.
     * I checked the following options:
     * - vAHI_ModuleConfigure(E_MODULE_DEFAULT) does not work on Aqara
     * - vAHI_ModuleConfigure(E_MODULE_JN5169_001_M03_ETSI) does not work on Aqara
     * - vAHI_ModuleConfigure(E_MODULE_JN5169_001_M06_FCC) low signal on Xiaomi
     * - vAppApiSetHighPowerMode (APP_API_MODULE_HPM05, TRUE) works well both on Xiaomi and Aqara */
    vAppApiSetHighPowerMode(APP_API_MODULE_HPM05, TRUE);
#endif

    /* idle task commences here */
    DBG_vPrintf(TRACE_APP, "*** ROUTER RESET ***\n");

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vSetUpHardware()\n");
    APP_vSetUpHardware();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitResources()\n");
    APP_vInitResources();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitialise()\n");
    APP_vInitialise();

    DBG_vPrintf(TRACE_APP, "APP: Entering BDB_vStart()\n");
    BDB_vStart();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vMainLoop()\n");
    APP_vMainLoop();
}

/****************************************************************************
 *
 * NAME: vAppRegisterPWRMCallbacks
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called to allow the application to register sleep and wake callbacks.
 * ---
 * Called in SDK JN-SW-4170
 *
 ****************************************************************************/
PUBLIC void vAppRegisterPWRMCallbacks(void)
{
    /* nothing to register as device does not sleep */
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialise
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 ****************************************************************************/
PRIVATE void APP_vInitialise(void)
{
    /* Initialise Power Manager even on non-sleeping nodes as it allows the
     * device to doze when in the idle task */
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

    /* Initialise the Persistent Data Manager */
    PDM_eInitialise(63);

    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();

    UART_vInit();
    UART_vRtsStartFlow();

    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);

  	// Initialize hardware
  	vAHI_DioSetDirection(BOARD_BTN_PIN, BOARD_LED_PIN);
  	vAHI_DioSetPullup(BOARD_BTN_PIN, 0);
  	vAHI_DioInterruptEdge(0, BOARD_BTN_PIN);
  	vAHI_DioInterruptEnable(BOARD_BTN_PIN, 0);

  	// Init and start timers
  	ZTIMER_eInit(timers, sizeof(timers) / sizeof(ZTIMER_tsTimer));

  	ZTIMER_eOpen(&buttonScanTimerHandle, buttonScanFunc, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
  	//ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));

  	ZQ_vQueueCreate(&queueHandle, 3, sizeof(ButtonPressType), (uint8*)queue);
  
    /* Initialise application */
    APP_vInitialiseRouter();
}

/****************************************************************************
 *
 * NAME: vfExtendedStatusCallBack
 *
 * DESCRIPTION:
 * Callback from stack on extended error situations.
 *
 ****************************************************************************/
PRIVATE void vfExtendedStatusCallBack(ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRACE_APP, "ERROR: Extended status 0x%02x\n", eExtendedStatus);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
