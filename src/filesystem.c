/*
 *   *** FOR STATIC DEBUGGING CODE ***
 *   Copyright (C) 2016 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
 *
 */

/**
 * This is an API wrapper for the TI proprietary file system that is used with their non-volatile storage solution.
 * This API will be used to store IR button data that will be retrieved by the user or restored on power cycle.
 * @file filesystem.c
 * @date 1/3/2019
 * @author Marcus Mueller
 */

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include "Board.h"
#include "wifi.h"
#include "filesystem.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif


#ifdef DEBUG_SESSION

// used to get file list information
#define MAX_FILE_ENTRIES 4

typedef struct
{
    SlFileAttributes_t attribute;
    char fileName[SL_FS_MAX_FILE_NAME_LENGTH];
} slGetfileList_t;


/**
 * This is a static debug function that prints flash storage file system info out to the UART
 * @author - written by TI developers
 */
static _i32 st_ShowStorageInfo()
{
    _i32        RetVal = 0;
    _i32        size;
    _i32        used;
    _i32        avail;
    SlFsControlGetStorageInfoResponse_t storageInfo;

    UART_PRINT("\n\rGet Storage Info:\n\r");

    RetVal = sl_FsCtl(( SlFsCtl_e)SL_FS_CTL_GET_STORAGE_INFO,
                   0,
                   NULL ,
                   NULL ,
                   0,
                   (_u8 *)&storageInfo,
                   sizeof(SlFsControlGetStorageInfoResponse_t),
                   NULL );

    if(RetVal < 0)
    {
        UART_PRINT("sl_FsCtl error: %d\n\r");
    }

    size = (storageInfo.DeviceUsage.DeviceBlocksCapacity *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;
    UART_PRINT("Total space: %dK\n\r\n\r", size);

    UART_PRINT("Filestsyem      Size \tUsed \tAvail\t\n\r");

    size = ((storageInfo.DeviceUsage.NumOfAvailableBlocksForUserFiles +
            storageInfo.DeviceUsage.NumOfAllocatedBlocks) *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;

    used = (storageInfo.DeviceUsage.NumOfAllocatedBlocks *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;

    avail = (storageInfo.DeviceUsage.NumOfAvailableBlocksForUserFiles *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;

    UART_PRINT("%-15s %dK \t%dK \t%dK \t\n\r", "User", size, used, avail);

    size = (storageInfo.DeviceUsage.NumOfReservedBlocksForSystemfiles *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;

    UART_PRINT("%-15s %dK \n\r", "System", size);

    size = (storageInfo.DeviceUsage.NumOfReservedBlocks *
            storageInfo.DeviceUsage.DeviceBlockSize) / 1024;

    UART_PRINT("%-15s %dK \n\r", "Reserved", size);

    UART_PRINT("\n\r");
    UART_PRINT("\n\r");

    UART_PRINT("%-32s: %d \n\r", "Max number of files",
               storageInfo.FilesUsage.MaxFsFiles);

    UART_PRINT("%-32s: %d \n\r", "Max number of system files",
               storageInfo.FilesUsage.MaxFsFilesReservedForSysFiles);

    UART_PRINT("%-32s: %d \n\r", "Number of user files",
               storageInfo.FilesUsage.ActualNumOfUserFiles);

    UART_PRINT("%-32s: %d \n\r", "Number of system files",
               storageInfo.FilesUsage.ActualNumOfSysFiles);

    UART_PRINT("%-32s: %d \n\r", "Number of alert",
               storageInfo.FilesUsage.NumOfAlerts);

    UART_PRINT("%-32s: %d \n\r", "Number Alert threshold",
               storageInfo.FilesUsage.NumOfAlertsThreshold);

    UART_PRINT("%-32s: %d \n\r", "FAT write counter",
               storageInfo.FilesUsage.FATWriteCounter);

    UART_PRINT("%-32s: ", "Bundle state");

    if(storageInfo.FilesUsage.Bundlestate == SL_FS_BUNDLE_STATE_STOPPED)
    {
        UART_PRINT("%s \n\r", "Stopped");
    }
    else if(storageInfo.FilesUsage.Bundlestate == SL_FS_BUNDLE_STATE_STARTED)
    {
        UART_PRINT("%s \n\r", "Started");
    }
    else if(storageInfo.FilesUsage.Bundlestate ==
            SL_FS_BUNDLE_STATE_PENDING_COMMIT)
    {
        UART_PRINT("%s \n\r", "Commit pending");
    }

    UART_PRINT("\n\r");

    return RetVal;
}

/**
 * This is a static debug function that prints the file system list out to the UART
 * @param  numOfFiles number of files to print out
 * @param  pPrintDescription boolean value deciding whether or not to print the file property flags info
 * @return 0 for OK, else error
 * @author - written by TI developers
 */
static _i32 st_listFiles(_i32 numOfFiles, int bPrintDescription)
{
    int retVal = SL_ERROR_BSD_ENOMEM;
    _i32            index = -1;
    _i32 fileCount = 0;
    slGetfileList_t *buffer = malloc(MAX_FILE_ENTRIES * sizeof(slGetfileList_t));

    UART_PRINT("\n\rRead files list:\n\r");
    if(buffer)
    {
        while( numOfFiles > 0 )
        {
            _i32 i;
            _i32 numOfEntries = (numOfFiles < MAX_FILE_ENTRIES) ? numOfFiles : MAX_FILE_ENTRIES;

            // Get FS list
            retVal = sl_FsGetFileList(&index,
                                      numOfEntries,
                                      sizeof(slGetfileList_t),
                                      (_u8*)buffer,
                                      SL_FS_GET_FILE_ATTRIBUTES);
            if(retVal < 0)
            {
                UART_PRINT("sl_FsGetFileList error:  %d\n\r", retVal);
                break;
            }
            if(retVal == 0)
            {
                break;
            }

            // Print single column format
            for (i = 0; i < retVal; i++)
            {
                UART_PRINT("[%3d] ", ++fileCount);
                UART_PRINT("%-40s\t", buffer[i].fileName);
                UART_PRINT("%8d\t", buffer[i].attribute.FileMaxSize);
                UART_PRINT("0x%03x\t", buffer[i].attribute.Properties);
                UART_PRINT("\n\r");
            }
            numOfFiles -= retVal;
        }
        UART_PRINT("\n\r");

        if(bPrintDescription)
        {
            UART_PRINT(" File properties flags description:\n\r");
            UART_PRINT(" 0x001 - Open file commit\n\r");
            UART_PRINT(" 0x002 - Open bundle commit\n\r");
            UART_PRINT(" 0x004 - Pending file commit\n\r");
            UART_PRINT(" 0x008 - Pending bundle commit\n\r");
            UART_PRINT(" 0x010 - Secure file\n\r");
            UART_PRINT(" 0x020 - No file safe\n\r");
            UART_PRINT(" 0x040 - System file\n\r");
            UART_PRINT(" 0x080 - System with user access\n\r");
            UART_PRINT(" 0x100 - No valid copy\n\r");
            UART_PRINT(" 0x200 - Public write\n\r");
            UART_PRINT(" 0x400 - Public read\n\r");
            UART_PRINT("\n\r");
        }
        free (buffer);
    }

    return retVal;
}

#endif /* DEBUG_SESSION */

/**
 * This function retrieves the size of a specified file in bytes
 * @param  fileName the name of the file to get the size of
 * @return the size of the file if OK, else FILE_IO_ERROR
 */
int fsGetFileSizeInBytes(const unsigned char* fileName)
{
    SlFsFileInfo_t fsFileInfo;
    int RetVal = sl_FsGetInfo(fileName, 0, &fsFileInfo);
    if (RetVal != 0)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("sl_FsGetInfo error: %d\n\r", RetVal);
#endif
        RetVal = FILE_IO_ERROR;
    }
    else
    {
        RetVal = fsFileInfo.Len;
#ifdef DEBUG_SESSION
        UART_PRINT("File size: %d\n\r", RetVal);
#endif
    }

    return RetVal;
}

/**
 * This function attempts to create a file within the TI flash filesystem
 * @param  fileName the name of the file
 * @param  maxFileSize the maximum length of the file in bytes (defaults to 4096, but cannot be 0)
 * @return the file descriptor if OK, else FILE_IO_ERROR
 */
int fsCreateFile(const unsigned char* fileName, _u32 maxFileSize)
{
    _i32 fd = sl_FsOpen(fileName, (SL_FS_CREATE | SL_FS_OVERWRITE | SL_FS_CREATE_MAX_SIZE(maxFileSize)), 0);

    if(fd < 0)
    {
        // could not create file
#ifdef DEBUG_SESSION
        UART_PRINT("sl_FsOpen (Creation) error: %d\n\r", fd);
#endif
        fd = FILE_IO_ERROR;
    }

    return fd;
}

/**
 * This function opens a file for reading OR writing (cannot do both at the same time)
 * @param  fileName the name of the file
 * @param  fileOperation for reading or writing
 * @return the file descriptor if OK, else FILE_IO_ERROR
 */
int fsOpenFile(const unsigned char* fileName, flashOperation fileOp)
{
    _u32 flags = 0;

    if (fileOp == flash_read)
    {
        flags |= SL_FS_READ;
    }
    else
    {
        flags |= SL_FS_WRITE;
    }

    _i32 fd = sl_FsOpen(fileName, flags, 0);

    if(fd < 0)
    {
        // could not open file
#ifdef DEBUG_SESSION
        UART_PRINT("sl_FsOpen error: %d\n\r", fd);
#endif
        fd = FILE_IO_ERROR;
    }

    return fd;
}

/**
 * This function writes to a file
 * @param  fileHandle the file descriptor to write to
 * @param  offset the location in the file to start writing to
 * @param  length the amount of data to write to the file
 * @param  buffer pointer to the data to write
 * @return the new offset if OK, else FILE_IO_ERROR
 */
int fsWriteFile(_i32 fileHandle, _u32 offset, _u32 length, const void* buffer)
{
    int RetVal;
    // keep track of initial offset
    int offsetStart = offset;

    while(offset < (length + offsetStart))
    {
        // write data to open file
        RetVal = sl_FsWrite(fileHandle,
                         offset,
                         (_u8 *)buffer,
                         length);
        if (RetVal <= 0)
        {
#ifdef DEBUG_SESSION
            UART_PRINT("sl_FsWrite error:  %d\n\r" ,RetVal);
#endif
            return FILE_IO_ERROR;
        }
        else
        {
            offset += RetVal;
            length -= RetVal;
        }

    }
#ifdef DEBUG_SESSION
    UART_PRINT("Wrote %d bytes...\n\r", (offset - offsetStart));
#endif
    return offset;
}

/**
 * This function reads from a file
 * @param  fileHandle the file descriptor to read from
 * @param  buff the buffer to fill with the read in data
 * @param  offset the offset in the file the start reading from
 * @param  length the amount of data to read from the file
 * @return the offset in the file that was read to if OK, else FILE_IO_ERROR
 */
int fsReadFile(_i32 fileHandle, void* buff, _u32 offset, _u32 length)
{
    // keep track of initial offset
    int offsetStart = offset;
    int RetVal = 0;

    while(offset < (length + offsetStart))
    {
        RetVal = sl_FsRead(fileHandle, offset, (_u8*)buff, length);

        if(RetVal == SL_ERROR_FS_OFFSET_OUT_OF_RANGE)
        {   // EOF
            break;
        }
        if(RetVal < 0)
        {   // Error
#ifdef DEBUG_SESSION
            UART_PRINT("sl_FsRead error: %d\n\r", RetVal);
#endif
            return FILE_IO_ERROR;
        }

        offset += RetVal;
        length -= RetVal;
    }

#ifdef DEBUG_SESSION
    UART_PRINT("Read \"%s\" (%d bytes)...\n\r", buff, (offset - offsetStart));
#endif
    return offset;
}

/**
 * This function attempts to close a file
 * @param  fileDescriptor file descriptor to close
 * @return 0 if OK, else FILE_IO_ERROR
 */
int fsCloseFile(_i32 fileDescriptor)
{
    int RetVal = sl_FsClose(fileDescriptor, 0, 0, 0);

    if(fileDescriptor < 0)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("sl_FsClose error: %d\n\r", RetVal);
#endif
        RetVal = FILE_IO_ERROR;
    }

    return RetVal;
}

/**
 * This function attempts to delete a file
 * @param  fileName file name to delete
 * @return 0 if OK, else FILE_IO_ERROR
 */
int fsDeleteFile(const unsigned char* fileName)
{
    int RetVal = sl_FsDel(fileName, 0);

    if(RetVal < 0)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("sl_FsDel error: %d\n\r", RetVal);
#endif
        RetVal = FILE_IO_ERROR;
    }

    return RetVal;
}

/**
 * This function finds out if the given file exists in the file system
 * @param fileName the file to check
 * @return true if the file exists, false if not
 */
bool fsCheckFileExists(const unsigned char* fileName)
{
    bool RetVal = false;
    int fd = fsOpenFile(fileName, flash_read);

    // Check if the file was successfully opened
    if (fd != FILE_IO_ERROR)
    {
        RetVal = true;
        fsCloseFile(fd);
    }

    return RetVal;
}

/**
 * This method initializes the file system subsystems in order to utilize the file system API
 */
void filesystem_init()
{
    // Terminal Initialization
    InitTerm();

    // Wifi Enabling (needed for the file system)
    wifi_init();

    // Powerup the CC3220SF Network Processor
    sl_Start(0, 0, 0);

    // These are static debug UART calls
#ifdef DEBUG_SESSION
    st_ShowStorageInfo();
    st_listFiles(250, 0);
#endif
}


/************************************************
 * These functions are required by the SDK and
 * will be filled out during later development.
 ***********************************************/

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
#ifdef DEBUG_SESSION
    UART_PRINT("pWlanEvent->Id=%d\n\r", pWlanEvent->Id);
#endif
}

void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
#ifdef DEBUG_SESSION
    UART_PRINT("slFatalErrorEvent->Id=%d\n\r", slFatalErrorEvent->Id);
#endif
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
#ifdef DEBUG_SESSION
     UART_PRINT("pDevEvent->Id=%d\n\r", pDevEvent->Id);
#endif
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    /* Unused in this application for now */
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    /* Unused in this application for now */
}

void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent,
                                      SlNetAppHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application for now */
}

void SimpleLinkNetAppRequestMemFreeEventHandler (uint8_t *buffer)
{
    /* Unused in this application for now */
}

void SimpleLinkNetAppRequestEventHandler (SlNetAppRequest_t *pNetAppRequest,
                                          SlNetAppResponse_t *pNetAppResponse)
{
    /* Unused in this application for now */
}

void SimpleLinkSocketTriggerEventHandler(SlSockTriggerEvent_t *pSockTriggerEvent)
{
    /* Unused in this application for now */
}
