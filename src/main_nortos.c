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
#include <stddef.h>

#include <NoRTOS.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
/* Example/Board Header files */
#include "Board.h"


void IRedgeDetector(uint_least8_t index);
void gpioButtonFxn0(uint_least8_t index);
void gpioButtonFxn1(uint_least8_t index);

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call driver init functions */
    Board_initGeneral();

    /* Start NoRTOS */
    NoRTOS_start();

    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);
    GPIO_setConfig(Board_GPIO_BUTTON1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);
    GPIO_setConfig(Board_EDGE_DETECT_P58, GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES);

    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
    GPIO_write(Board_IR_OUTPUT_P50, Board_GPIO_LED_OFF);

    /* install Button callback */
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);
    GPIO_setCallback(Board_GPIO_BUTTON1, gpioButtonFxn1);
    GPIO_setCallback(Board_EDGE_DETECT_P58, IRedgeDetector);

    /* Enable interrupts */
    GPIO_enableInt(Board_GPIO_BUTTON0);
    GPIO_enableInt(Board_GPIO_BUTTON1);
    GPIO_enableInt(Board_EDGE_DETECT_P58);

    while (1) {}
}


/*
 *  ======== IRedgeDetector ========
 *  Callback function for the GPIO edge interrupt on GPIOCC32XX_GPIO_03.
 */
void IRedgeDetector(uint_least8_t index)
{
    /* Clear the GPIO interrupt and set the IR out to match the interrupt edge */
    if (GPIO_read(Board_EDGE_DETECT_P58))
    {
        GPIO_write(Board_IR_OUTPUT_P50, Board_GPIO_LED_OFF);
    }
    else
    {
        GPIO_write(Board_IR_OUTPUT_P50, Board_GPIO_LED_ON);
    }
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON0.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Clear the GPIO interrupt and toggle an LED */
    GPIO_toggle(Board_GPIO_LED0);
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON1.
 *  This may not be used for all boards.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Clear the GPIO interrupt and toggle an LED */
    GPIO_toggle(Board_GPIO_LED0);
}
