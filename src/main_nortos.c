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

void gpioButtonFxn0(uint_least8_t index);

/*
 *  ======== main ========
 */
int main(void)
{
    // Call driver init functions
    Board_initGeneral();

    // Start NoRTOS
    NoRTOS_start();

    NVS_Handle rHandle;
    NVS_Attrs regionAttrs;
    uint_fast16_t status;
    char buf[32];
    // Initialize the NVS driver
    NVS_init();
    //
    // Open the NVS region specified by the 0 element in the NVS_config[]
    // array defined in Board.c.
    // Use default NVS_Params to open this memory region, hence NULL
    //
    rHandle = NVS_open(Board_NVSINTERNAL, NULL);

    // fetch the generic NVS region attributes
    NVS_getAttrs(rHandle, &regionAttrs);
    // erase the first sector of the NVS region
    status = NVS_erase(rHandle, 0, regionAttrs.sectorSize);
    if (status != NVS_STATUS_SUCCESS) {
        // Error handling code
    }
    // write "Hello" to the base address of region 0, verify after write
    status = NVS_write(rHandle, 0, "Hello", strlen("Hello")+1, NVS_WRITE_POST_VERIFY);
    if (status != NVS_STATUS_SUCCESS) {
        // Error handling code
    }
    // copy "Hello" from region0 into local 'buf'
    status = NVS_read(rHandle, 0, buf, strlen("Hello")+1);
    if (status != NVS_STATUS_SUCCESS) {
        // Error handling code
    }

    // close the region
    NVS_close(rHandle);

    // Call driver init functions
    GPIO_init();

    // Configure the LED and button pins
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);
    GPIO_setConfig(Board_GPIO_BUTTON1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);

    // Turn on user LED
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    // install Button callback
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);
    GPIO_setCallback(Board_GPIO_BUTTON1, gpioButtonFxn0);

    // Enable interrupts
    GPIO_enableInt(Board_GPIO_BUTTON0);
    GPIO_enableInt(Board_GPIO_BUTTON1);

    // Initialize IR control
    IR_Init_Receiver();
    IR_Init_Emitter();

    while (1) {}
}

/**
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON0.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    // Clear the GPIO interrupt and toggle an LED
    GPIO_toggle(Board_GPIO_LED0);

    // send a button as a test in this interrupt for now
    if (IRbuttonReady() && !IRbuttonSending())
    {
        IRemitterSendButton(getIRsequence(), getIRcarrierFrequency());
    }
}
