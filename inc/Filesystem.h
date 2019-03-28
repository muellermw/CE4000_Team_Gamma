/**
 * This is an API wrapper for the TI proprietary file system that is used with their non-volatile storage solution.
 * This API will be used to store IR button data that will be retrieved by the user or restored on power cycle.
 * @file Filesystem.h
 * @date 1/3/2019
 * @author Marcus Mueller
 */

#ifndef INC_FILESYSTEM_H_
#define INC_FILESYSTEM_H_

#include <ti/drivers/net/wifi/simplelink.h>
#include "Signal_Interval.h"

#define FILE_IO_ERROR -77 // arbitrary value (can reassign, but keep negative)

typedef enum
{
    flash_read,
    flash_write
} flashOperation;

void filesystem_init();
void fsPrintInfo();
int fsGetFileSizeInBytes(const unsigned char* fileName);
int fsCreateFile(const unsigned char* fileName, _u32 maxFileSize);
int fsOpenFile(const unsigned char* fileName, flashOperation fileOp);
int fsReadFile(_i32 fileHandle, void* buff, _u32 offset, _u32 length);
int fsWriteFile(_i32 fileHandle, _u32 offset, _u32 length, const void* buffer);
int fsCloseFile(_i32 fileDescriptor);
int fsDeleteFile(const unsigned char* fileName);
bool fsCheckFileExists(const unsigned char* fileName);

#endif /* INC_FILESYSTEM_H_ */
