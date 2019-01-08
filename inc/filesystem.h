/**
 * This is an API wrapper for the TI proprietary file system that is used with their non-volatile storage solution.
 * This API will be used to store IR button data that will be retrieved by the user or restored on power cycle.
 * @file filesystem.h
 * @date 1/3/2019
 * @author Marcus Mueller
 */

#ifndef INC_FILESYSTEM_H_
#define INC_FILESYSTEM_H_

#include <ti/drivers/net/wifi/simplelink.h>

#define BUTTON_TABLE_FILE "table_of_buttons"
#define BUTTON_TABLE_FILE_MAX_SIZE (_u32)8192
#define BUTTON_SINGLE_FILE_MAX_SIZE (_u32)1024
#define MAX_BUTTON_NAME_SIZE 32
#define MAX_AMOUNT_OF_BUTTONS 220

typedef struct
{
    char buttonName[MAX_BUTTON_NAME_SIZE];
    _u16 buttonIndex;
} ButtonTableEntry;

typedef enum
{
    flash_read,
    flash_write
} flashOperation;

void filesystem_init();
int fsGetFileSizeInBytes(const unsigned char* fileName);
int fsCreateFile(const unsigned char* fileName, _u32 maxFileSize);
int fsOpenFile(const unsigned char* fileName, flashOperation fileOp);
int fsReadFile(_i32 fileHandle, void* buff, _u32 offset, _u32 length);
int fsWriteFile(_i32 fileHandle, _u32 offset, _u32 length, const void* buffer);
int fsCloseFile(_i32 fileDescriptor);
int fsDeleteFile(const unsigned char* fileName);


#endif /* INC_FILESYSTEM_H_ */
