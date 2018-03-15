/**
  @file  main.c
  Revised:        $Date: 2015-05-22 07:14:53 -0700 (Fri, 22 May 2015) $
  Revision:       $Revision: 43907 $

  @brief main entry of the BLE stack sample application.

  <!--
  Copyright 2013 - 2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED ``AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
  -->
*/

#include <xdc/runtime/Error.h>
#include <ti/sysbios/family/arm/cc26xx/Power.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include "ICall.h"

#include "bcomdef.h"
#include "peripheral.h"
#include "hiddev.h"
#include "hidemukbd.h"
#include "util.h"
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/UART.h>

/* Header files required to enable instruction fetch cache */
#include <inc/hw_memmap.h>
#include <driverlib/vims.h>

#ifndef USE_DEFAULT_USER_CFG

#include "bleUserConfig.h"

// BLE user defined configuration
bleUserCfg_t user0Cfg = BLE_USER_CFG;

#endif // USE_DEFAULT_USER_CFG

#define TASKSTACKSIZE     768

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];
/**
 * Exception handler
 */
void exceptionHandler()
{
	volatile uint8_t i = 1;
	PINCC26XX_setOutputValue(Board_RLED, 1);
    while(i){}
}
Clock_Handle clkHandle;
volatile uint8_t uart_flag;
/*Clock 1s*/
Void clkFxn(UArg arg0)
{
    PINCC26XX_setOutputValue(Board_GLED, PINCC26XX_getOutputValue(Board_GLED) ^ 1);
    uart_flag = 1;
    Clock_start(clkHandle);
}

/*uart echo*/
Void echoFxn(UArg arg0, UArg arg1)
{
    char x[] = "A";
    UART_Handle uart;
    UART_Params uartParams;

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;
    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        PINCC26XX_setOutputValue(Board_RLED, 1);
    }

    //UART_write(uart, echoPrompt, sizeof(echoPrompt));

    /* Loop forever echoing */
    while (1) {
        if (uart_flag){
            uart_flag = 0;
            UART_write(uart, x, 1);
        }
    }
}
/*
 *  ======== main ========
 */
int main()
{
    Task_Params taskParams;
    Clock_Params clockParams;

    PIN_init(BoardGpioInitTable);
    Board_initUART();

    /* Construct BIOS objects */
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)echoFxn, &taskParams, NULL);

    Clock_Params_init(&clockParams);
    clockParams.period = 0;
    clockParams.startFlag = TRUE;
    clkHandle = Clock_create(clkFxn, 10000, &clockParams, NULL);

#ifndef POWER_SAVING
    /* Set constraints for Standby, powerdown and idle mode */
    Power_setConstraint  (Power_SB_DISALLOW);
    Power_setConstraint  (Power_IDLE_PD_DISALLOW);
#endif //POWER_SAVING

    /* Initialize ICall module */
    ICall_init();

    /* Start tasks of external images - Priority 5 */
    ICall_createRemoteTasks();

    /* Kick off profile - Priority 3 */
    GAPRole_createTask();
    
    /* Kick off HID service task - Priority 2 */
    HidDev_createTask();
    
    /* Kick off application - Priority 1 */
    HidEmuKbd_createTask();

    /* enable interrupts and start SYS/BIOS */
    BIOS_start();

    return 0;
}

/**
 * Error handled to be hooked into TI-RTOS
 */
Void smallErrorHook(Error_Block *eb)
{
    PINCC26XX_setOutputValue(Board_RLED, 1);
  for (;;);
}

/**
 * HAL assert handler required by OSAL memory module.
 */
void halAssertHandler(void)
{
    PINCC26XX_setOutputValue(Board_RLED, 1);
  for (;;);
}