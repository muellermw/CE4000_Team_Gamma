/*
 * Flash.h
 *
 *  Header file for the Flash.c interface
 *  Created on: Dec 16, 2018
 *      Author: haasbr
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_


//defines


//method headers

void init_NVS();
void writetoNVS(void* dataToWrite, uint32_t region, uint32_t sizeOfData);
void* readfromNVS(uint32_t region, uint32_t returnDataSize);



#endif /* INC_FLASH_H_ */
