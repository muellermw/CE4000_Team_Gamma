/**
 * This is a small API file that focuses on providing button operations such as adding/removing
 * @file button.h
 * @date 1/14/2019
 * @author: Marcus Mueller
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "filesystem.h"

#define BUTTON_TABLE_FILE "table_of_buttons"
#define BUTTON_FILE_STRING "Button%d"
#define BUTTON_TABLE_FILE_MAX_SIZE (_u32)8192
#define BUTTON_SINGLE_FILE_MAX_SIZE (_u32)1024
#define BUTTON_NAME_MAX_SIZE 32
#define BUTTON_FILE_NAME_MAX_SIZE 10 // biggest value is "Button220"
#define MAX_AMOUNT_OF_BUTTONS 220

typedef struct
{
    char buttonName[BUTTON_NAME_MAX_SIZE];
    _u16 irCarrierFrequency;
    _u16 buttonIndex;
} ButtonTableEntry;

void button_init();
int createButton(const unsigned char* buttonName, _u16 buttonCarrierFrequency, SignalInterval* buttonSequence, _u16 sequenceSize);
int deleteButton(_u16 buttonIndex);
int addButtonTableEntry(const unsigned char* buttonName, _u16 buttonCarrierFrequency);
int deleteButtonTableEntry(_u16 buttonIndex);
int findNumButtonEntries(ButtonTableEntry* entryList, _u32 fileSize);
int getButtonCarrierFrequency(_u16 buttonIndex);
void initNewButtonEntry(ButtonTableEntry* newButton, _u16 buttonNameMaxSize);
ButtonTableEntry* retrieveButtonTableContents(const unsigned char* fileName, _u32 fileSize);
SignalInterval* getButtonSignalInterval(_u16 buttonIndex);
void deleteAllButtons();

void printButtonTable(); //MOVE TO WIFI WHEN READY

#endif /* INC_BUTTON_H_ */
