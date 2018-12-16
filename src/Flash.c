/*
 * Flash.c
 *
 *  This file is an interface for initializing memory as well
 *  as reading and writing from it.
 *  Created on: Dec 16, 2018
 *      Author: haasbr
 */
// Driver for NVS
#include <ti/drivers/NVS.h>
#include "Board.h"
#include "Flash.h"
#include <stdlib.h>

NVS_Handle rHandle;
NVS_Attrs regionAttrs;

void init_NVS(){
    NVS_init();
    return;
}

/*
 * This method writes data to a desired region in memory
 * @params: We need to know what region of the memory to read from
 * @returns: a void pointer to the data read at the memory
 */
void writetoNVS(void* dataToWrite, uint32_t region, uint32_t sizeOfData){
    //
    // Open the NVS region specified by the 0 element in the NVS_config[]
    // array defined in Board.c.
    // Use default NVS_Params to open this memory region, hence NULL
    //
    rHandle = NVS_open(Board_NVSINTERNAL, NULL);//open the region

    // fetch the generic NVS region attributes
    NVS_getAttrs(rHandle, &regionAttrs);
    // erase the desired sector (region) of the NVS region
    NVS_erase(rHandle, region, regionAttrs.sectorSize);

    // write contents of dataToWrite to the base address of requested region
    NVS_write(rHandle, region, dataToWrite, sizeOfData+1, NVS_WRITE_POST_VERIFY);

    // close the region
       NVS_close(rHandle);
}
/*
 * This method reads data from memory and returns it
 * @params: We need to know what region of the memory to read from
 * @returns: a void pointer to the data read at the memory
 */
void* readfromNVS(uint32_t region, uint32_t returnDataSize){
    // Open the NVS region specified by the 0 element in the NVS_config[]
    // array defined in Board.c.
    // Use default NVS_Params to open this memory region, hence NULL
    //
    rHandle = NVS_open(Board_NVSINTERNAL, NULL);
    void* returnData;
    // copy contents from region into returnData
    NVS_read(rHandle, region, returnData, returnDataSize+1);


   // close the region
   NVS_close(rHandle);

   return returnData;

}
