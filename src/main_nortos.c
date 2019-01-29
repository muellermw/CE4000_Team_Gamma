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
#include "wifi.h"
#include "button.h"
#include "Control_States.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif

// Defines
#define BUFF_SIZE 256
#define ARG_LENGTH 32
#define SOCKET_ERROR "Error Creating Socket"
#define BINDING_ERROR "Error Binding Socket"
#define RECEIVING_ERROR "Error Receiving Message"
#define DEVICE_INFO_ERROR "Error Sending Device Information"
#define READY_REC "ready_to_record"
#define SEND_ERROR "Error Sending Message"

void toLower(char* string);
void gpioButtonFxnSW2(uint_least8_t index);
void gpioButtonFxnSW3(uint_least8_t index);

#ifdef DEBUG_SESSION
void fileSystemTestCode();
void pairingLEDTestBlink();
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

#ifdef DEBUG_SESSION
    // Terminal Initialization
    InitTerm();
#endif

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

    /***********************************************************************
     * Init UDP for non-blocking sockets
     ***********************************************************************/
    // Initialize socket and buffers
    _i16 Sd;
    _i16 Status;
    SlSockAddrIn_t Addr;
    char recBuf[BUFF_SIZE] = {0};
    char sendBuf[BUFF_SIZE] = {0};
    Sd = sl_Socket(SL_AF_INET, SL_SOCK_DGRAM, 0);
    if( 0 > Sd )
    {
        UART_PRINT("\r\n%s\r\n", SOCKET_ERROR);
    }

    // Binding Socket
    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(44444);
    Addr.sin_addr.s_addr = SL_INADDR_ANY;
    Status = sl_Bind(Sd, ( SlSockAddr_t *)&Addr, sizeof(SlSockAddrIn_t));
    if( Status )
    {
        UART_PRINT("\r\n%s\r\n", BINDING_ERROR);
    }

    // Retrieved from http://e2e.ti.com/support/wireless-connectivity/wifi/f/968/t/368485
    // Enables non blocking networking receiving
    SlSockNonblocking_t enableOption;
    enableOption.NonBlockingEnabled = 1;
    sl_SetSockOpt(Sd,SL_SOL_SOCKET,SL_SO_NONBLOCKING, (_u8 *)&enableOption,sizeof(enableOption));

    ControlState currState = idle;
    while (1)
    {
        //The SimpleLink host driver architecture mandate calling 'sl_task' in
        //a NO-RTOS application's main loop.
        //The purpose of this call, is to handle asynchronous events and get
        //flow control information sent from the NWP.
        //Every event is classified and later handled by the host driver
        //event handlers.
        sl_Task(NULL);

        // Receive data from the network
        SlSocklen_t AddrSize = sizeof(SlSockAddrIn_t);
        Status = sl_RecvFrom(Sd, recBuf, BUFF_SIZE, 0, ( SlSockAddr_t *)&Addr, &AddrSize);
        if(Status < 0 && Status != SL_EAGAIN)
        {
            UART_PRINT("\r\n%s\r\n", RECEIVING_ERROR);
        }

        // Data was received
        if (Status > 0)
        {
            UART_PRINT("\r\nReceived: %s\r\n", recBuf);

            // Parse commands from the receive buffer
            char *strState;
            char *arg1;
            char *arg2;
            const char delim[2] = ",";
            strState = strtok(recBuf, delim);
            arg1 = strtok(NULL, delim);
            arg2 = strtok(NULL, delim);
            toLower(strState);

            // APP_INIT: Provide the app with the device IP and Name
            if(strncmp(strState, APP_INIT_STR, strlen(APP_INIT_STR)) == 0){
                // Get the obtained IP of the device
                uint16_t len = sizeof(SlNetCfgIpV4Args_t);
                uint16_t configOpt = 0;
                SlNetCfgIpV4Args_t ipV4 = {0};
                sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &configOpt, &len, (_u8 *)&ipV4);

                // Get the user-assigned name of the device
                len = DEVICE_NAME_LENGTH;
                char* deviceName[len];
                memset(deviceName, 0, len);
                configOpt = SL_WLAN_P2P_OPT_DEV_NAME;
                sl_WlanGet(SL_WLAN_CFG_P2P_PARAM_ID, &configOpt , &len, (_u8*)deviceName);

                currState = app_init;
                sprintf(sendBuf, "\r\n%d.%d.%d.%d,%s\r\n",
                                 (uint8_t)SL_IPV4_BYTE(ipV4.Ip, 3),
                                 (uint8_t)SL_IPV4_BYTE(ipV4.Ip, 2),
                                 (uint8_t)SL_IPV4_BYTE(ipV4.Ip, 1),
                                 (uint8_t)SL_IPV4_BYTE(ipV4.Ip, 0),
                                 (char *)deviceName);

                Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                if( strlen(sendBuf) != Status )
                {
                    UART_PRINT("\r\n%s\r\n", DEVICE_INFO_ERROR);
                }
                currState = idle;
            }
            // BUTTON_REFRESH: Provide the app with a list of available buttons
            // TODO Reformat the list structure and send over the network
            else if(strncmp(strState, BUTTON_REFRESH_STR, strlen(BUTTON_REFRESH_STR)) == 0){
                currState = button_refresh;
                printButtonTable();
                currState = idle;
            }
            // ADD_BUTTON: Record the button and report back to the app
            // TODO Check for null argument
            else if(strncmp(strState, ADD_BUTTON_STR, strlen(ADD_BUTTON_STR)) == 0){
                currState = add_button;
                IRreceiverSetMode(program);

                //UART_PRINT("\r\nReady to Record\r\n");
                sprintf(sendBuf, "\r\n%s\r\n", READY_REC);
                Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                if( strlen(sendBuf) != Status )
                {
                    UART_PRINT("\r\n%s\r\n", SEND_ERROR);
                }

                while(!IRbuttonReady());
                uint16_t sequenceSize = 0;
                SignalInterval* irSequence = getIRsequence(&sequenceSize);
                uint16_t carrFreq = getIRcarrierFrequency();
                int button_index = createButton((const unsigned char*)arg1, carrFreq, irSequence, sequenceSize);
                if(button_index == FILE_IO_ERROR){
                    UART_PRINT("\r\nFILE_IO_ERROR\r\n");
                }
                else{
                    //UART_PRINT("\r\nbutton_saved,%s,%d\r\n", arg1, button_index);
                    sprintf(sendBuf, "\r\nbutton_saved,%s,%d\r\n", arg1, button_index);
                    Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                    if( strlen(sendBuf) != Status )
                    {
                        UART_PRINT("\r\n%s\r\n", SEND_ERROR);
                    }
                }
                IRreceiverSetMode(passthru);
                currState = idle;
            }
            // DELETE_BUTTON: Deletes button from flash and reports back to app
            // TODO Check button name and send errors and check for null arguments
            else if(strncmp(strState, DELETE_BUTTON_STR, strlen(DELETE_BUTTON_STR)) == 0){
                currState = delete_button;
                int button_index = atoi(arg1);
                int delCheck = deleteButton(button_index);
                if(delCheck == FILE_IO_ERROR){
                    UART_PRINT("\r\ndeleted_button,%s\r\n", arg1);
                }
                else{
                    //UART_PRINT("\r\ndeleted_button,%s\r\n", arg1);
                    sprintf(sendBuf, "\r\ndeleted_button,%s\r\n", arg1);
                    Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                    if( strlen(sendBuf) != Status )
                    {
                        UART_PRINT("\r\n%s\r\n", SEND_ERROR);
                    }
                }
                currState = idle;
            }
            // DELETE_BUTTON: Sends button with IR and reports back to app
            // TODO Check button name and send errors and check for null arguments
            else if(strncmp(strState, SEND_BUTTON_STR, strlen(SEND_BUTTON_STR)) == 0){
                currState = send_button;
                IRstopEdgeDetectGPIO();
                int button_index = atoi(arg1);
                SignalInterval* irSequence = getButtonSignalInterval(button_index);
                if (irSequence != NULL)
                {
                    int carrFreq = getButtonCarrierFrequency(button_index);

                    if (carrFreq != FILE_IO_ERROR)
                    {
                        IRemitterSendButton(irSequence, carrFreq);
                        //UART_PRINT("\r\nbutton_sent,%s\r\n", button_index);
                        sprintf(sendBuf, "\r\nbutton_sent,%s\r\n", arg1);
                        Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                        if( strlen(sendBuf) != Status )
                        {
                            UART_PRINT("\r\n%s\r\n", SEND_ERROR);
                        }
                    }
                    else{
                        UART_PRINT("\r\nfailed_send_button,%d\r\n", button_index);
                    }
                }
                currState = idle;
                IRstartEdgeDetectGPIO();
            }
            else if(strncmp(strState, CLEAR_BUTTONS_STR, strlen(CLEAR_BUTTONS_STR)) == 0){
                deleteAllButtons();
            }
        }
    }
}

/**
 * This method converts all upper-case letters in a string to lower-case
 * @param string the char array to convert to lower-case
 */
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

void pairingLEDTestBlink()
{
    while (1)
    {
        for (int i=0; i<=4000000; i++)
        {
            if (i==0)
            {
                GPIO_write(Board_PAIRING_OUTPUT_PIN, 1);
                GPIO_write(Board_IR_OUTPUT_PIN, 1);
            }
            else if (i==2000000)
            {
                GPIO_write(Board_PAIRING_OUTPUT_PIN, 0);
                GPIO_write(Board_IR_OUTPUT_PIN, 0);
            }
        }
    }
}
#endif

