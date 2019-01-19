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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "button.h"
#include "Control_States.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif

void toLower(char* string);
void gpioButtonFxnSW2(uint_least8_t index);
void gpioButtonFxnSW3(uint_least8_t index);
#ifdef DEBUG_SESSION
void fileSystemTestCode();
#endif

bool emitterReady = false;
bool mainSendButton = false;
bool mainDeleteButton = false;

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

    // Start the button subsystem that utilizes the file system
    button_init();

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

    // Enable IR receiver and emitter
    IR_Init_Receiver();
    IR_Init_Emitter();

    ControlState currState = idle;
    while (1)
    {
        char buff[64] = {0};
        UART_GET(buff, 64);
        UART_PRINT("\r\nReceived: %s\r\n", buff);

        char strState[20] = {0};
        char arg[32] = {0};
        sscanf(buff, "%s %s", strState, arg);

        toLower(strState);

        if(strncmp(strState, APP_INIT_STR, sizeof(APP_INIT_STR)) == 0){
            currState = app_init;
            printButtonTable();
            currState = idle;
        }
        else if(strncmp(strState, ADD_BUTTON_STR, sizeof(ADD_BUTTON_STR)) == 0){
            currState = add_button;
            IRreceiverSetMode(program);
            UART_PRINT("\r\nReady to Record\r\n");
            while(!IRbuttonReady());
            uint16_t sequenceSize = 0;
            SignalInterval* irSequence = getIRsequence(&sequenceSize);
            uint16_t carrFreq = getIRcarrierFrequency();
            int button_index = createButton((const unsigned char*)arg, carrFreq, irSequence, sequenceSize);
            if(button_index == FILE_IO_ERROR){
                UART_PRINT("\r\nFILE_IO_ERROR\r\n");
            }
            else{
                UART_PRINT("\r\nButton Info: %s, %d\r\n", arg, button_index);
            }
            IRreceiverSetMode(passthru);
            currState = idle;
        }
        else if(strncmp(strState, DELETE_BUTTON_STR, sizeof(DELETE_BUTTON_STR)) == 0){
            currState = delete_button;
            int button_index = atoi(arg);
            int delCheck = deleteButton(button_index);
            if(delCheck == FILE_IO_ERROR){
                UART_PRINT("\r\nFailed to Delete Button at Index: %s\r\n", arg);
            }
            else{
                UART_PRINT("\r\nDeleted Button at Index: %s\r\n", arg);
            }
            currState = idle;
        }
        else if(strncmp(strState, SEND_BUTTON_STR, sizeof(SEND_BUTTON_STR)) == 0){
            currState = send_button;
            IRstopEdgeDetectGPIO();
            int button_index = atoi(arg);
            SignalInterval* irSequence = getButtonSignalInterval(button_index);
            if (irSequence != NULL)
            {
                int carrFreq = getButtonCarrierFrequency(button_index);

                if (carrFreq != FILE_IO_ERROR)
                {
                    IRemitterSendButton(irSequence, carrFreq);
                    UART_PRINT("\r\nSent Button at Index: %s\r\n", arg);
                }
                else{
                    UART_PRINT("\r\nFailed to Send Button at Index: %s\r\n", arg);
                }
            }
            currState = idle;
            IRstartEdgeDetectGPIO();
        }
    }
}

void toLower(char* string){
    for(int i = 0; string[i]; i++){
        string[i] = tolower(string[i]);
    }
}

/**
 *  ======== gpioButtonFxnSW2 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON_SW2
 */
void gpioButtonFxnSW2(uint_least8_t index)
{
    // Delete the first button file
    mainDeleteButton = true;
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
        mainSendButton = true;
    }
}

#ifdef DEBUG_SESSION
void fileSystemTestCode()
{
    // file system test code:

    //fsDeleteFile(BUTTON_TABLE_FILE);
    addButtonTableEntry("testButton0", 38000);
    addButtonTableEntry("testButton1", 56000);
    addButtonTableEntry("testButton2", 34000);
    deleteButtonTableEntry(1);
    addButtonTableEntry("testButton1", 60000);

    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);
    ButtonTableEntry* testList = retrieveButtonTableContents(BUTTON_TABLE_FILE, fileSize);
    int numValidEntries = findNumButtonEntries(testList, fileSize);

    if (testList != NULL)
    {
        UART_PRINT("Valid entries: %d\r\n", numValidEntries);
        UART_PRINT("Name: %s\r\nIndex: %d\r\nFrequency: %d\r\n", testList[0].buttonName, testList[0].buttonIndex, testList[0].irCarrierFrequency);
        UART_PRINT("Name: %s\r\nIndex: %d\r\nFrequency: %d\r\n", testList[1].buttonName, testList[1].buttonIndex, testList[1].irCarrierFrequency);
        UART_PRINT("Name: %s\r\nIndex: %d\r\nFrequency: %d\r\n", testList[2].buttonName, testList[2].buttonIndex, testList[2].irCarrierFrequency);
    }

    free(testList);
}
#endif

