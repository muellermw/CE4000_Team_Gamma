/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,

 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== main_nortos.c ========
 */
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <NoRTOS.h>
// Driver Header files
#include <ti/drivers/GPIO.h>
// Driver for NVS
#include <ti/drivers/NVS.h>
// Board Header file
#include "Board.h"
#include "IR_Emitter.h"
#include "IR_Receiver.h"
#include "Signal_Interval.h"
#include "filesystem.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif

void gpioButtonFxnSW2(uint_least8_t index);
void gpioButtonFxnSW3(uint_least8_t index);

bool emitterReady = false;

/*
 *  ======== main ========
 */
int main(void)
{
    // Call driver init functions
    Board_initGeneral();

    // Start NoRTOS
    NoRTOS_start();

    // Enable the file system NVS driver
    filesystem_init();

    // Call driver init functions
    GPIO_init();

    // Configure the button pins
    GPIO_setConfig(Board_GPIO_BUTTON_SW2, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);
    GPIO_setConfig(Board_GPIO_BUTTON_SW3, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);

    // install Button callback
    GPIO_setCallback(Board_GPIO_BUTTON_SW2, gpioButtonFxnSW2);
    GPIO_setCallback(Board_GPIO_BUTTON_SW3, gpioButtonFxnSW3);

    // Enable interrupts
    GPIO_enableInt(Board_GPIO_BUTTON_SW2);
    GPIO_enableInt(Board_GPIO_BUTTON_SW3);

    // Initialize IR control
    if (fsCheckFileExists("Button0") == false)
    {
        IR_Init_Receiver();
    }
    else
    {
        emitterReady = true;
    }

    IR_Init_Emitter();


    // file system test code:

    //fsDeleteFile(BUTTON_TABLE_FILE);
    /*
    fsAddButtonTableEntry("testButton0");
    fsAddButtonTableEntry("testButton1");
    fsAddButtonTableEntry("testButton2");
    fsDeleteButtonTableEntry(1);
    fsAddButtonTableEntry("testButton1");

    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);
    ButtonTableEntry* testList = fsRetrieveButtonTableContents(BUTTON_TABLE_FILE, fileSize);
    int numValidEntries = fsFindNumButtonEntries(testList, fileSize);

    if (testList != NULL)
    {
        UART_PRINT("Valid entries: %d\r\n", numValidEntries);
        UART_PRINT("Name: %s\r\nIndex: %d\r\n", testList[0].buttonName, testList[0].buttonIndex);
        UART_PRINT("Name: %s\r\nIndex: %d\r\n", testList[1].buttonName, testList[1].buttonIndex);
        UART_PRINT("Name: %s\r\nIndex: %d\r\n", testList[2].buttonName, testList[2].buttonIndex);
    }

    free(testList);
    */

    while (1)
    {
        if (IRbuttonReady())
        {
            // Create the button file
            SignalInterval* irSignal = getIRsequence();
            fsSaveButton("TestButton0", getIRcarrierFrequency(), irSignal, MAX_SEQUENCE_INDEX*sizeof(SignalInterval));

            emitterReady = true;
        }
    }
}



/**
 *  ======== gpioButtonFxnSW2 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON_SW2
 */
void gpioButtonFxnSW2(uint_least8_t index)
{
    // Delete the first button file
    fsDeleteFile("Button0");
}

/**
 *  ======== gpioButtonFxnSW3 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON_SW3
 */
void gpioButtonFxnSW3(uint_least8_t index)
{
    // send a button as a test in this interrupt for now
    if (emitterReady && !IRbuttonSending())
    {
        IRemitterSendButton(getIRsequence(), getIRcarrierFrequency());
    }
}
