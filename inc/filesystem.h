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

void filesystem_init();
int fsGetFileSizeInBytes(const unsigned char* fileName);
int fsCreateFile(const unsigned char* fileName, _u32 maxFileSize);
int fsOpenFile(const unsigned char* fileName, _u8 read, _u8 write);
int fsReadFile(_i32 fileHandle, char* buff, _u32 offset, _u32 length);
int fsWriteFile(_i32 fileHandle, _u32 offset, _u32 length, _const char *buffer);
int fsDeleteFile(const unsigned char* fileName);

#endif /* INC_FILESYSTEM_H_ */
