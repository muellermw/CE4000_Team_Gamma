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
 *  This file represents the main logic control for the NCIR project
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
#include "Filesystem.h"
#include "Wifi.h"
#include "Button.h"
#include "IR_Emitter.h"
#include "IR_Receiver.h"
#include "Control_States.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif

// Constants
#define BUFF_SIZE 256
#define ARG_LENGTH 32

#define SOCKET_ERROR         "Error Creating Socket"
#define BINDING_ERROR        "Error Binding Socket"
#define RECEIVING_ERROR      "Error Receiving Message"
#define BUTTON_ADD_ERROR     "Error Adding Button"
#define BUTTON_DELETE_ERROR  "Error Deleting Button"
#define BUTTON_SEND_ERROR    "Error Sending Button"
#define BUTTON_REFRESH_ERROR "Error Refreshing Button List"
#define DEVICE_INFO_ERROR    "Error Sending Device Information"
#define SEND_ERROR           "Error Sending Message"

#define READY_REC            "ready_to_record"
#define BTN_NOT_AVAILABLE    "button_not_available"

int compareButtonNames(char* suppliedName, uint8_t buttonIndex);
char* createButtonRefreshBuffer();
void toLower(char* string);

#ifdef DEBUG_SESSION
void fileSystemTestCode();
void pairingLEDTestBlink();
#endif


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

    // Start the Network Processor for WiFi
    wifi_init();

    // Start the button subsystem that utilizes the file system
    button_init();

    // If we are in debug mode, this will print the current file system arrangement to the console
    fsPrintInfo();

    // Call driver initialization functions
    GPIO_init();

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
#ifdef DEBUG_SESSION
        UART_PRINT("\r\n%s\r\n", SOCKET_ERROR);

        // At this point, we can't go any further, so try to restart
        resetBoard();
#endif
    }

    // Binding Socket
    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(44444);
    Addr.sin_addr.s_addr = SL_INADDR_ANY;
    SlSocklen_t AddrSize = sizeof(SlSockAddrIn_t);
    Status = sl_Bind(Sd, ( SlSockAddr_t *)&Addr, AddrSize);
    if( Status )
    {
#ifdef DEBUG_SESSION
        UART_PRINT("\r\n%s\r\n", BINDING_ERROR);

        // At this point, we can't go any further, so try to restart
        resetBoard();
#endif
    }

    // Retrieved from http://e2e.ti.com/support/wireless-connectivity/wifi/f/968/t/368485
    // Enables non blocking networking receiving
    SlSockNonblocking_t enableOption;
    enableOption.NonBlockingEnabled = 1;
    sl_SetSockOpt(Sd,SL_SOL_SOCKET,SL_SO_NONBLOCKING, (_u8 *)&enableOption,sizeof(enableOption));

    // Within the network processor, UDP TX packets are buffered due to how the WiFi standard works.
    // Because of this, UDP packet reception times may vary significantly. This power setting policy is
    // utilized to maintain a much more consistent UDP reception time, and thus a more "snappy" application.
    sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_ALWAYS_ON_POLICY, NULL, 0);

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

        // Clear the send and receive buffers
        if (recBuf[0] != NULL)
        {
            memset(recBuf, NULL, sizeof(recBuf));
        }

        if (recBuf[0] != NULL)
        {
            memset(sendBuf, NULL, sizeof(sendBuf));
        }

        // Reinitialize address structure to avoid potential errors
        Addr.sin_family = SL_AF_INET;
        Addr.sin_port = sl_Htons(44444);
        Addr.sin_addr.s_addr = SL_INADDR_ANY;

        // Receive data from the network
        Status = sl_RecvFrom(Sd, recBuf, BUFF_SIZE, 0, ( SlSockAddr_t *)&Addr, &AddrSize);
        if(Status < 0 && Status != SL_EAGAIN)
        {
#ifdef DEBUG_SESSION
            UART_PRINT("\r\n%s\r\n", RECEIVING_ERROR);
#endif
        }

        // Data was received
        if (Status > 0)
        {
#ifdef DEBUG_SESSION
            UART_PRINT("\r\nReceived: %s\r\n", recBuf);
#endif

            // Parse commands from the receive buffer
            char *strState;
            char *arg1;
            char *arg2;
            const char delim[2] = ",";
            strState = strtok(recBuf, delim);
            arg1 = strtok(NULL, delim);
            arg2 = strtok(NULL, delim);
            toLower(strState);

            // SEND_BUTTON: Sends button with IR and reports back to app
            if(strncmp(strState, SEND_BUTTON_STR, strlen(SEND_BUTTON_STR)) == 0){

                // Check if the button name argument is not empty
                if (arg1[0] != NULL)
                {
                    int button_index = atoi(arg2);

                    // Compare the name of the button the client wants to delete with
                    // the name of the button that was stored at the button index. If
                    // it does not match, send a message telling the client that their
                    // button database is out-of-date and should be updated.
                    if (compareButtonNames(arg1, button_index) == 0)
                    {
                        bool error = true;

                        // Stop any IR detection while sending a button signal
                        IRstopEdgeDetectGPIO();
                        currState = send_button;

                        SignalInterval* irSequence = getButtonSignalInterval(button_index);
                        if (irSequence != NULL)
                        {
                            int carrFreq = getButtonCarrierFrequency(button_index);

                            if (carrFreq != FILE_IO_ERROR)
                            {
                                // Send out the signal
                                IRemitterSendButton(irSequence, carrFreq);

                                // Indicate success status
                                error = false;

                                sprintf(sendBuf, "\r\nbutton_sent,%s,%d\r\n", arg1, button_index);
                                Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                                if( strlen(sendBuf) != Status )
                                {
#ifdef DEBUG_SESSION
                                    UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                                }
                            }
                        }

                        // Send an error message if something went wrong
                        if (error)
                        {
                            sprintf(sendBuf, "\r\n%s\r\n", BUTTON_SEND_ERROR);
                            Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                            if( strlen(sendBuf) != Status )
                            {
#ifdef DEBUG_SESSION
                                UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                            }
                        }

                        // Start edge detection again, as we are going into an idle state
                        IRstartEdgeDetectGPIO();
                        currState = idle;
                    }
                    else
                    {
                        sprintf(sendBuf, "\r\n%s\r\n", BTN_NOT_AVAILABLE);
                        Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                        if( strlen(sendBuf) != Status )
                        {
#ifdef DEBUG_SESSION
                            UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                        }
                    }
                }
            }
            // APP_INIT: Provide the app with the device IP and Name
            else if(strncmp(strState, APP_INIT_STR, strlen(APP_INIT_STR)) == 0){

                // Get the obtained IP of the device
                uint16_t len = sizeof(SlNetCfgIpV4Args_t);
                uint16_t configOpt = 0;
                SlNetCfgIpV4Args_t ipV4 = {0};
                sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &configOpt, &len, (_u8 *)&ipV4);

                // Get the user-assigned name of the device
                len = DEVICE_NAME_LENGTH;
                char deviceName[len];
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
#ifdef DEBUG_SESSION
                    UART_PRINT("\r\n%s\r\n", DEVICE_INFO_ERROR);
#endif
                }
                currState = idle;
            }
            // BUTTON_REFRESH: Provide the app with a list of available buttons
            else if(strncmp(strState, BUTTON_REFRESH_STR, strlen(BUTTON_REFRESH_STR)) == 0){
                currState = button_refresh;

                char* refreshBuff = createButtonRefreshBuffer();

                if (refreshBuff != NULL)
                {
                    // Add the end NULL character for receiver convenience
                    uint16_t refreshBuffSize = strlen(refreshBuff) + 1;

                    Status = sl_SendTo(Sd, refreshBuff, refreshBuffSize, 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));

                    if(refreshBuffSize != Status)
                    {
#ifdef DEBUG_SESSION
                        UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                    }

                    free(refreshBuff);
                }
                else
                {
                    Status = sl_SendTo(Sd, BUTTON_REFRESH_ERROR, strlen(BUTTON_REFRESH_ERROR), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));

                    // Send the same message to the UART for debug purposes
                    if(strlen(BUTTON_REFRESH_ERROR) != Status)
                    {
#ifdef DEBUG_SESSION
                        UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                    }
                }
                currState = idle;
            }
            // ADD_BUTTON: Record the button and report back to the application
            else if(strncmp(strState, ADD_BUTTON_STR, strlen(ADD_BUTTON_STR)) == 0){

                // Check if the name argument is not empty
                if (arg1[0] != NULL)
                {
                    currState = add_button;
                    IRreceiverSetMode(program);

                    // Let the client know that the device is waiting for an IR signal to record
                    sprintf(sendBuf, "\r\n%s\r\n", READY_REC);
                    Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));

                    if( strlen(sendBuf) != Status )
                    {
#ifdef DEBUG_SESSION
                        UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                    }

                    // Wait for button recording to be completed
                    while(!IRbuttonReady());

                    uint16_t sequenceSize = 0;
                    SignalInterval* irSequence = getIRsequence(&sequenceSize);
                    uint16_t carrFreq = getIRcarrierFrequency();
                    int button_index = createButton((const unsigned char*)arg1, carrFreq, irSequence, sequenceSize);

                    if(button_index == FILE_IO_ERROR){
                        sprintf(sendBuf, "\r\n%s\r\n", BUTTON_ADD_ERROR);
                        sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                    }
                    else{
                        // Get the name that was saved to the button table of contents
                        // as the name could have been truncated if it was too long
                        char btnNameBuff[BUTTON_NAME_MAX_SIZE];
                        getButtonName(button_index, btnNameBuff);
                        sprintf(sendBuf, "\r\nbutton_saved,%s,%d\r\n", btnNameBuff, button_index);

                        Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                        if( strlen(sendBuf) != Status )
                        {
#ifdef DEBUG_SESSION
                            UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                        }
                    }
                    IRreceiverSetMode(passthru);
                    currState = idle;
                }
            }
            // DELETE_BUTTON: Deletes button from flash and reports back to app
            else if(strncmp(strState, DELETE_BUTTON_STR, strlen(DELETE_BUTTON_STR)) == 0){

                // Check if the button name argument is not empty
                if (arg1[0] != NULL)
                {
                    int button_index = atoi(arg2);

                    // Compare the name of the button the client wants to delete with
                    // the name of the button that was stored at the button index. If
                    // it does not match, send a message telling the client that their
                    // button database is out-of-date and should be updated.
                    if (compareButtonNames(arg1, button_index) == 0)
                    {
                        currState = delete_button;

                        int delCheck = deleteButton(button_index);
                        if(delCheck == FILE_IO_ERROR){
                            sprintf(sendBuf, "\r\n%s\r\n", BUTTON_DELETE_ERROR);
                            Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                            if( strlen(sendBuf) != Status )
                            {
#ifdef DEBUG_SESSION
                                UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                            }
                        }
                        else{
                            sprintf(sendBuf, "\r\ndeleted_button,%s,%d\r\n", arg1, button_index);
                            Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                            if( strlen(sendBuf) != Status )
                            {
#ifdef DEBUG_SESSION
                                UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                            }
                        }

                        currState = idle;
                    }
                    else
                    {
                        sprintf(sendBuf, "\r\n%s\r\n", BTN_NOT_AVAILABLE);
                        Status = sl_SendTo(Sd, sendBuf, strlen(sendBuf), 0, (SlSockAddr_t*)&Addr, sizeof(SlSockAddr_t));
                        if( strlen(sendBuf) != Status )
                        {
#ifdef DEBUG_SESSION
                            UART_PRINT("\r\n%s\r\n", SEND_ERROR);
#endif
                        }
                    }
                }
            }
            // CLEAR_ALL: Deletes all buttons on the device
            else if(strncmp(strState, CLEAR_BUTTONS_STR, strlen(CLEAR_BUTTONS_STR)) == 0){
                deleteAllButtons();
            }
        }
    }
}

/**
 * This function compares the name of the button supplied with what is
 * in the button table of contents to see if they are consistent
 * @return 0 if they are the same, -1 if different
 */
int compareButtonNames(char* suppliedName, uint8_t buttonIndex)
{
    int RetVal = -1;

    char btnNameBuff[BUTTON_NAME_MAX_SIZE];
    getButtonName(buttonIndex, btnNameBuff);

    uint8_t btnNameLength = strlen(suppliedName);

    if (btnNameLength <= BUTTON_NAME_MAX_SIZE)
    {
        // Add +1 to name length to compare null character as well
        if (strncmp(suppliedName, btnNameBuff, btnNameLength+1) == 0)
        {
            RetVal = 0;
        }
    }

    return RetVal;
}

/**
 * This function creates a button refresh buffer to send to a client
 * based on the data within the button table of contents file
 * @return the populated button refresh buffer
 * @note THE RETURNED BUFFER NEEDS TO BE FREED BY THE CALLER
 */
char* createButtonRefreshBuffer()
{
    char* refreshBuff = NULL;

    // This adds up the button name max size, size of the button index number,
    // and the extra characters added between the values
    uint8_t refreshEntryMaxSize = BUTTON_NAME_MAX_SIZE + sizeof(uint16_t) + strlen(",\r\n");
    uint16_t buttonTableSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

    if (buttonTableSize > 0)
    {
        ButtonTableEntry* buttonTable = retrieveButtonTableContents(BUTTON_TABLE_FILE, buttonTableSize);

        if (buttonTable != NULL)
        {
            uint8_t numEntries = findNumButtonEntries(buttonTable, buttonTableSize);

            if (numEntries > 0)
            {
                // Allocate the maximum theoretical buffer for the entries
                refreshBuff = malloc((refreshEntryMaxSize * numEntries) + 1);

                if (refreshBuff != NULL)
                {
                    uint8_t i = 0;
                    uint8_t entryIndex = 0;
                    char* offset = refreshBuff;
                    char entryStringBuff[refreshEntryMaxSize];

                    // Loop though the button entries to fill the buffer
                    while (i < numEntries)
                    {
                        // Make sure each button entry is valid, skip it if not
                        if (buttonTable[entryIndex].buttonName[0] != NULL)
                        {
                            sprintf(entryStringBuff, "%s,%d\r\n", (char *)(buttonTable[entryIndex].buttonName), buttonTable[entryIndex].buttonIndex);
                            uint8_t entryLength = strlen(entryStringBuff);

                            // Add +1 for the NULL character that strlen didn't account for
                            memcpy(offset, entryStringBuff, entryLength + 1);

                            // Increment the offset by the string length of the entry string
                            offset += entryLength;
                            i++;
                        }
                        // Always increment the entry index
                        entryIndex++;
                    }
                }
            }
            free(buttonTable);
        }
    }

    return refreshBuff;
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

