/*
 * wifi.c
 *
 *  Created on: Jan 3, 2019
 *      Author: Marcus
 */

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

void wifi_init()
{
    GPIO_init();
    SPI_init();
}
