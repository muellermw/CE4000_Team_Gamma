/**
 * This is a small API file that focuses on providing button operations such as adding/removing
 * @file button.c
 * @date 1/14/2019
 * @author: Marcus Mueller
 */

#include <stdio.h>
#include <ti/drivers/net/wifi/simplelink.h>
// Board Header file
#include "Board.h"
#include "filesystem.h"
#include "button.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif


static void initializeButtonTable();
static bool checkIdenticalButtonEntries(const unsigned char* newButtonName, ButtonTableEntry* buttonTableList, _u16 numButtonEntries);

/**
 *
 */
void button_init()
{
    // Get button table of contents set up
    initializeButtonTable();
}

/**
 * Save an IR button sequence to flash and its corresponding entry to the button table file
 * @param buttonName the name to save to the button table file
 * @param buttonCarrierFrequency the carrier frequency of the IR signal
 * @param buttonSequence the IR sequence to store in its own file
 * @param sequenceSize the size of the IR sequence in bytes
 * @return button index if OK, else FILE_IO_ERROR
 */
int createButton(const unsigned char* buttonName, _u16 buttonCarrierFrequency, SignalInterval* buttonSequence, _u16 sequenceSize)
{
    int RetVal = FILE_IO_ERROR;

    if (buttonName != NULL)
    {
        // Make sure the button name and IR sequence are not empty
        if ((buttonName[0] != NULL) && (buttonSequence != NULL))
        {
            int buttonIndex = addButtonTableEntry(buttonName, buttonCarrierFrequency);

            // Check if a valid index has been returned
            if (buttonIndex >= 0 && buttonIndex <= MAX_AMOUNT_OF_BUTTONS)
            {
                char sequenceFileName[BUTTON_FILE_NAME_MAX_SIZE];
                memset(sequenceFileName, NULL, BUTTON_FILE_NAME_MAX_SIZE);

                // Form the new file name string
                snprintf(sequenceFileName, BUTTON_FILE_NAME_MAX_SIZE, BUTTON_FILE_STRING, buttonIndex);

                int fd = fsCreateFile((const unsigned char*)sequenceFileName, sequenceSize);

                // Check if the file descriptor is valid
                if (fd != FILE_IO_ERROR)
                {
                    // Write the sequence into storage
                    fsWriteFile(fd, 0, sequenceSize, buttonSequence);
                    fsCloseFile(fd);
                    RetVal = buttonIndex;
                }
                // Something went seriously wrong, revert what was written
                else
                {
                    deleteButtonTableEntry(buttonIndex);
                }
            }
        }
    }

    return RetVal;
}

/**
 * Delete an IR sequence file and its corresponding button table entry from the system
 * @param buttonIndex the button index to delete
 * @return 0 if OK, else FILE_IO_ERROR
 */
int deleteButton(_u16 buttonIndex)
{
    int RetVal = FILE_IO_ERROR;

    if (buttonIndex <= MAX_AMOUNT_OF_BUTTONS)
    {
        // Generate the file to delete
        char sequenceFileName[BUTTON_FILE_NAME_MAX_SIZE];
        memset(sequenceFileName, NULL, BUTTON_FILE_NAME_MAX_SIZE);

        // Form the new file name string
        snprintf(sequenceFileName, BUTTON_FILE_NAME_MAX_SIZE, BUTTON_FILE_STRING, buttonIndex);

        fsDeleteFile((const unsigned char*)sequenceFileName);

        // Delete the table entry
        RetVal = deleteButtonTableEntry(buttonIndex);
    }

    return RetVal;
}

/**
 * Delete all buttons that were ever added to flash
 */
void deleteAllButtons()
{
    // Get the size of the file so we know how many entries we need to go through
    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

    int numButtons = fileSize/sizeof(ButtonTableEntry);

    // Go through each possible button entry and clear both the button entry and the file
    for (int i=numButtons; (i-1) >= 0; i--)
    {
        // Don't check for any errors, as we don't care if a button file doesn't exist at this point
        deleteButton(i-1);
    }
}

/**
 * This function clears the memory location of a button entry in the
 * button table entry at the index provided so it can be repopulated later
 * @param buttonIndex the index to clear the memory in the button table file
 * @return 0 if OK, else FILE_IO_ERROR
 */
int deleteButtonTableEntry(_u16 buttonIndex)
{
    // Default value
    int RetVal = FILE_IO_ERROR;

    // Get the size of the file so we know how many entries we need to go through
    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

    // Check that the size of the button table file was found
    if (fileSize != FILE_IO_ERROR)
    {
        _u16 numEntries = fileSize/sizeof(ButtonTableEntry);

        // Check if the button index is within the bounds of the file contents,
        // if it is the same or greater than the number of entries it is invalid
        if (numEntries > buttonIndex)
        {
            // Get the button table list so we can clear the given index
            ButtonTableEntry* buttonTableList = retrieveButtonTableContents(BUTTON_TABLE_FILE, fileSize);

            // Check that the list is valid
            if (buttonTableList != NULL && buttonTableList[buttonIndex].buttonName[0] != NULL)
            {
                int fd = fsOpenFile(BUTTON_TABLE_FILE, flash_write);
                if (fd != FILE_IO_ERROR)
                {
                    // Check to see if the button being deleted is the last button in the list,
                    // if so, don't even bother writing it to the file
                    if ((numEntries > 1) && (buttonIndex == numEntries-1))
                    {
                        fsWriteFile(fd, 0, (sizeof(ButtonTableEntry)*(numEntries-1)), buttonTableList);
                    }
                    else
                    {
                        // We are clear to erase the button index in memory
                        memset(&buttonTableList[buttonIndex], NULL, sizeof(ButtonTableEntry));
                        fsWriteFile(fd, 0, (sizeof(ButtonTableEntry)*numEntries), buttonTableList);
                    }
                    // Done writing, close the button table file
                    fsCloseFile(fd);

                    // Give an OK error condition
                    RetVal = 0;
                }

                free(buttonTableList);
            }
        }
    }

    return RetVal;
}

/**
 * This function determines how many valid (not blank) button entries there are in the button entry table
 * @param entryList list of button table entries
 * @param fileSize size of the button entry file
 * @return number of valid button entries if OK, else FILE_IO_ERROR
 */
int findNumButtonEntries(ButtonTableEntry* entryList, _u32 fileSize)
{
    int RetVal = FILE_IO_ERROR;
    _u16 numAllocatedEntries = fileSize/sizeof(ButtonTableEntry);
    _u16 numValidEntries = 0;

    if (entryList != NULL)
    {
        // Go through each allocated entry and find the total number of the ones that have data in them
        for (int i = 0; i < numAllocatedEntries; i++)
        {
            // Check to make sure the first character of the entry button name is not zeroed out
            if (entryList[i].buttonName[0] != NULL)
            {
                // This is a valid entry, increment the count
                numValidEntries++;
            }
        }
        RetVal = numValidEntries;
    }

    return RetVal;
}

/**
 * This function adds a button table entry by finding the next open index and writing it to the button table file
 * @param buttonName the string name of the new button
 * @param buttonCarrierFrequency the carrier frequency of the IR signal
 * @return the index that the new button was assigned, or FILE_IO_ERROR if error
 */
int addButtonTableEntry(const unsigned char* buttonName, _u16 buttonCarrierFrequency)
{
    // Default value for return value
    int RetVal = FILE_IO_ERROR;
    _u32 buttonIndex;
    ButtonTableEntry newButton;

    // Get the size of the file so we know how many entries we need to go through
    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

    if (fileSize != FILE_IO_ERROR)
    {
        // If the file contains at least one button table entry
        if (fileSize >= (_u16)sizeof(ButtonTableEntry))
        {
            // Get the button table list so we can check which index to assign the new button
            ButtonTableEntry* buttonTableList = retrieveButtonTableContents(BUTTON_TABLE_FILE, fileSize);

            // Check that the list is valid
            if (buttonTableList != NULL)
            {
                // Get the number of valid button entries in the entry file and check it against the maximum allowed buttons
                _u16 numValidEntries = findNumButtonEntries(buttonTableList, fileSize);

                // Make sure the maximum amount of buttons allowed on this system will not be exceeded
                if (numValidEntries < MAX_AMOUNT_OF_BUTTONS)
                {
                    _u16 numAllocatedEntries = fileSize/sizeof(ButtonTableEntry);

                    // Check if the new button name already exists as a button entry
                    bool duplicateButtonName = checkIdenticalButtonEntries(buttonName, buttonTableList, numAllocatedEntries);

                    if (duplicateButtonName == false)
                    {
                        // Go through each existing entry and find an empty spot to write the new button name and index.
                        // The index is decided automatically based on the position of the blank memory offset (starts at zero)
                        for (buttonIndex = 0; buttonIndex < numAllocatedEntries; buttonIndex++)
                        {
                            // Check if the first character of the entry button name is NULL or 0xFF (depends on how the chip clears memory)
                            if ((buttonTableList[buttonIndex].buttonName[0] == NULL) || (buttonTableList[buttonIndex].buttonName[0] == 0xFF))
                            {
                                // We have found a valid place to put the new button! Break out
                                break;
                            }
                        }

                        // Create the new button entry
                        initNewButtonEntry(&newButton, BUTTON_NAME_MAX_SIZE);
                        strncpy(newButton.buttonName, (char*)buttonName, BUTTON_NAME_MAX_SIZE-1);
                        newButton.irCarrierFrequency = buttonCarrierFrequency;
                        newButton.buttonIndex = buttonIndex;

                        // At this point we have the full list of button entries and their locations. Opening a file for write means
                        // we need to completely rewrite the entire file, but this is OK since we have all of the data
                        int fd = fsOpenFile(BUTTON_TABLE_FILE, flash_write);
                        if (fd != FILE_IO_ERROR)
                        {
                            // Create the new list in memory before writing it to storage
                            // Check if the list has a blank spot and can stay the same size
                            if (buttonIndex < numAllocatedEntries)
                            {
                                memcpy(&buttonTableList[buttonIndex], &newButton, sizeof(ButtonTableEntry));

                                // Write the updated list
                                fsWriteFile(fd, 0, (sizeof(ButtonTableEntry)*numAllocatedEntries), buttonTableList);
                            }
                            // We must make the list one button table entry larger
                            else
                            {
                                ButtonTableEntry* newTable = malloc(sizeof(ButtonTableEntry)*(numAllocatedEntries+1));

                                if (newTable != NULL)
                                {
                                    // Copy the entire existing button table list into the new list
                                    memcpy(newTable, buttonTableList, sizeof(ButtonTableEntry)*numAllocatedEntries);

                                    // Copy the new button entry to the end of the new list
                                    memcpy(&newTable[buttonIndex], &newButton, sizeof(ButtonTableEntry));

                                    // Write the new list
                                    fsWriteFile(fd, 0, (sizeof(ButtonTableEntry)*(numAllocatedEntries+1)), newTable);

                                    free(newTable);
                                }
                            }
                            // Done writing, close the button table file
                            fsCloseFile(fd);

                            RetVal = buttonIndex;
                        }
                    }
                }

                free(buttonTableList);
            }
        }
        // If we get here that means that the file size is likely 0 and has no entries
        else
        {
            // Make this entry the first (0th) entry
            buttonIndex = 0;

            // Create the new button entry
            initNewButtonEntry(&newButton, BUTTON_NAME_MAX_SIZE);
            strncpy(newButton.buttonName, (char*)buttonName, BUTTON_NAME_MAX_SIZE-1);
            newButton.irCarrierFrequency = buttonCarrierFrequency;
            newButton.buttonIndex = buttonIndex;

            int fd = fsOpenFile(BUTTON_TABLE_FILE, flash_write);
            if (fd != FILE_IO_ERROR)
            {
                // Write the new entry
                fsWriteFile(fd, 0, sizeof(ButtonTableEntry), (void*)&newButton);

                // Done writing, close the button table file
                fsCloseFile(fd);

                RetVal = buttonIndex;
            }
        }
    }

    return RetVal;
}

/**
 * This function extracts the button entries from the button table contents file and returns
 * them to the caller. This allows for button-index synchronization with the application.
 * @param fileName the name of the button table file
 * @param fileSize the size of the button table file in flash storage
 * @return a pointer to the list of button table entries, or NULL if error
 * @remark the button table entry pointer MUST BE FREED
 */
ButtonTableEntry* retrieveButtonTableContents(const unsigned char* fileName, _u32 fileSize)
{
    ButtonTableEntry* buttonTableList = NULL;

    // Check to see if the size is valid
    if (fileSize >= (_u16)sizeof(ButtonTableEntry))
    {
        buttonTableList = malloc(fileSize);

        // Check if the memory was successfully reserved
        if (buttonTableList != NULL)
        {
            // Open the file for reading
            int fd = fsOpenFile(fileName, flash_read);

            if (fd != FILE_IO_ERROR)
            {
                fsReadFile(fd, buttonTableList, 0, fileSize);
                fsCloseFile(fd);
            }
            else
            {
                free(buttonTableList);
                buttonTableList = NULL;
            }
        }
    }

    return buttonTableList;
}

/**
 * This function gets the IR signal sequence from the button at the given index
 * @param buttonIndex the index the get the signal sequence from
 * @return the signal interval pointer that contains the sequence, or NULL if error
 * @remark the signal interval pointer MUST BE FREED
 */
SignalInterval* getButtonSignalInterval(_u16 buttonIndex)
{
    SignalInterval* irSignal = NULL;

    if (buttonIndex <= MAX_AMOUNT_OF_BUTTONS)
    {
        // Generate the file to delete
        char sequenceFileName[BUTTON_FILE_NAME_MAX_SIZE];
        memset(sequenceFileName, NULL, BUTTON_FILE_NAME_MAX_SIZE);

        // Form the new file name string
        snprintf(sequenceFileName, BUTTON_FILE_NAME_MAX_SIZE, BUTTON_FILE_STRING, buttonIndex);

        int fileSize = fsGetFileSizeInBytes((const unsigned char*)sequenceFileName);

        if (fileSize != FILE_IO_ERROR)
        {
            irSignal = malloc(fileSize);

            if (irSignal != NULL)
            {
                // Open the file to read the sequence data
                int fd = fsOpenFile((const unsigned char*)sequenceFileName, flash_read);

                if (fd != FILE_IO_ERROR)
                {
                    fsReadFile(fd, irSignal, 0, fileSize);
                    fsCloseFile(fd);
                }
            }
        }
    }

    return irSignal;
}

/**
 * This function gets the IR carrier frequency of the button at the given index
 * @param buttonIndex the index to get the carrier frequency out of
 * @return the carrier frequency if OK, else FILE_IO_ERROR
 */
int getButtonCarrierFrequency(_u16 buttonIndex)
{
    int RetVal = FILE_IO_ERROR;

    if (buttonIndex <= MAX_AMOUNT_OF_BUTTONS)
    {
        int tableSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

        // Make sure the button table file contains valid data at this offset
        if (tableSize >= ((buttonIndex+1)*sizeof(ButtonTableEntry)))
        {
            // Open the button table file for reading
            int fd = fsOpenFile(BUTTON_TABLE_FILE, flash_read);

            if (fd != FILE_IO_ERROR)
            {
                _u16 offset = (buttonIndex*sizeof(ButtonTableEntry));
                ButtonTableEntry buttonEntry;
                initNewButtonEntry(&buttonEntry, BUTTON_NAME_MAX_SIZE);

                // Read in the button entry at that index
                fsReadFile(fd, &buttonEntry, offset, sizeof(ButtonTableEntry));
                fsCloseFile(fd);

                if (buttonEntry.irCarrierFrequency != 0)
                {
                    RetVal = buttonEntry.irCarrierFrequency;
                }
            }
        }
    }

    return RetVal;
}

/**
 * This function gets the name of the IR button at the given index
 * @param buttonIndex the index to get the button name out of
 * @param nameBuffer the buffer to fill with the button name
 */
void getButtonName(_u16 buttonIndex, char* nameBuffer)
{
    bool error = true;

    if ((buttonIndex <= MAX_AMOUNT_OF_BUTTONS) && (nameBuffer != NULL))
    {
        int tableSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);

        // Make sure the button table file contains valid data at this offset
        if (tableSize >= ((buttonIndex+1)*sizeof(ButtonTableEntry)))
        {
            // Open the button table file for reading
            int fd = fsOpenFile(BUTTON_TABLE_FILE, flash_read);

            if (fd != FILE_IO_ERROR)
            {
                _u16 offset = (buttonIndex*sizeof(ButtonTableEntry));
                ButtonTableEntry buttonEntry;
                initNewButtonEntry(&buttonEntry, BUTTON_NAME_MAX_SIZE);

                // Read in the button entry at that index
                fsReadFile(fd, &buttonEntry, offset, sizeof(ButtonTableEntry));
                fsCloseFile(fd);

                // Make sure the button name is valid
                if (buttonEntry.buttonName[0] != NULL)
                {
                    strncpy(nameBuffer, (char *)(buttonEntry.buttonName), BUTTON_NAME_MAX_SIZE);

                    // Set OK error condition
                    error = false;
                }
            }
        }
    }
    // Check if the method did not finish correctly
    if (error)
    {
        // Make sure the first char in the input buffer is set to NULL to indicate failure
        nameBuffer[0] = NULL;
    }
}

/**
 * This method makes sure that the button table of contents
 * exists, and creates it if it doesn't.
 */
static void initializeButtonTable()
{
    bool fileExists = fsCheckFileExists(BUTTON_TABLE_FILE);

    // Check if the file descriptor is valid, if it is not, the file does not exist, so create it
    if (fileExists == false)
    {
        int fd = fsCreateFile(BUTTON_TABLE_FILE, BUTTON_TABLE_FILE_MAX_SIZE);

        if (fd != FILE_IO_ERROR)
        {
            fsCloseFile(fd);
        }
    }
}

/**
 * Helper function to set the memory of a new button entry all to 0
 * @param newButton the button table entry to initialize
 * @param buttonNameMaxSize The maximum size of the button name string
 */
void initNewButtonEntry(ButtonTableEntry* newButton, _u16 buttonNameMaxSize)
{
    memset(newButton->buttonName, NULL, buttonNameMaxSize);
    newButton->irCarrierFrequency = 0;
    newButton->buttonIndex = 0;
}

void printButtonTable(){
    // Get the size of the file so we know how many entries we need to go through
    int fileSize = fsGetFileSizeInBytes(BUTTON_TABLE_FILE);
    int buttonEntries = fileSize/sizeof(ButtonTableEntry);
    ButtonTableEntry* btnTblList = retrieveButtonTableContents(BUTTON_TABLE_FILE, fileSize);

    for (int i = 0; i < buttonEntries; i++)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("\nName: %s\r\nIndex: %d\r\nFrequency: %d\r\n\n", btnTblList[i].buttonName, btnTblList[i].buttonIndex, btnTblList[i].irCarrierFrequency);
#endif
    }
}

/**
 * This function checks the button table list to make sure the new button name being added is unique from any existing button
 * @param newButtonName the new button name
 * @param buttonTableList the list of button entries to compare the new button name to
 * @param numButtonEntries the number of button entries the button table list contains
 * @return true if the function found a match (bad), false if the new button name is unique (good)
 */
static bool checkIdenticalButtonEntries(const unsigned char* newButtonName, ButtonTableEntry* buttonTableList, _u16 numButtonEntries)
{
    bool RetVal = false;

    for (int i = 0; i < numButtonEntries; i++)
    {
        // Compare the new button name and the current list entry
        if (strcmp((char*)newButtonName, (char*)&buttonTableList[i].buttonName) == 0)
        {
            // We found a match, no need to keep looking through the list
            RetVal = true;
#ifdef DEBUG_SESSION
            UART_PRINT("Add button: the button name '%s' already exists in the button table. Abandoning add button...\r\n", newButtonName);
#endif
            break;
        }
    }

    return RetVal;
}
