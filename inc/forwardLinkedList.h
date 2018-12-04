/**
 * This file contains the interface for the linked list file. THis includes
 * defining all of the necessary structures and functions.
 * @file linkedlist.h
 * @author Max Kallenberger, Marcus Mueller
 * @date September 17, 2017, December 2018
 */

#ifndef FORWARDLINKEDLIST_H
#define FORWARDLINKEDLIST_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>

/**
 * This structure holds the data and links needed for an element in a linked list.
 */
struct listNode {
  void* data; // A pointer to the data contained within the node
  struct listNode* nextNode; // A pointer to the next node in the linked list
};

/**
 * This structure is a linked list data structure that can be manipulated through function calls.
 */
struct linkedList
{
  struct listNode* head; // A pointer to the first node in the linked list
  struct listNode* tail; // A pointer to the last node in the linked list
  uint32_t size; // The number of nodes in the linked list
};

/**
 * This structure provides the elements needed to iterate through the linked list data structure.
 */
struct linkedListIterator
{
  struct listNode* current; // A pointer to the node the iterator is currently at
};

/**
 * This function initializes the elements in the linkedList structure to default values.
 * @param list This is a pointer to the list to initialize.
 */
void fll_init(struct linkedList* list);

/**
 * This function adds an element to the linked list at the end of the list.
 * @param list This is a pointer to the list to add to.
 * @param object This is a pointer to the object to be added to the list.
 * @param size This is the size of the object being added in bytes.
 * @return This returns true if the add was successful, false if it failed.
 */
bool fll_add(struct linkedList* list, const void* object, uint32_t size);

/**
 * This function adds an element to the linked list at the desired index.
 * @param list This is a pointer to the list to add to.
 * @param object This is a pointer to the object to be added to the list.
 * @param size This is the size of the object being added in bytes.
 * @param index This is the index to add the object at.
 * @return This returns true if the add was successful, false if it failed.
 */
bool fll_addIndex(struct linkedList* list, const void* object, uint32_t size, uint32_t index);

/**
 * This function removes an object from the list at a given index.
 * @param list This is a pointer to the list to remove from.
 * @param index This is the index to remove the object from.
 * @return This returns true if the remove was successful, false if it failed.
 */
bool fll_remove(struct linkedList* list, uint32_t index);

/**
 * This function gets the object from the desired list index.
 * @param list This is a pointer to the list to get the object from.
 * @param index This is the index to get the object from.
 */
void* fll_get(struct linkedList* list, uint32_t index);

/**
 * This function frees the memory allocated for the nodes and data of objects in the list.
 * The elements of the linkedList structure are reinitialized to their default values.
 * @param list This is a pointer to the list to be cleared.
 */
void fll_clear(struct linkedList* list);

/**
 * This function returns the size element of the linked list structure.
 * @param list This is a pointer to the list to return the size of.
 * @return This returns the size of the list (the number of objects contained in the list).
 */
uint32_t fll_size(struct linkedList* list);

/**
 * This function creates an iterator for a given linked list. The iterator starts at the head of the list.
 * @param list This is a pointer to the list used to create the iterator.
 * @return This returns a pointer to the linkedListIterator structure if the iterator generated correctly,
 * 	       else it returns NULL.
 */
struct linkedListIterator* fll_getIterator(struct linkedList* list);

/**
 * This function determines if an iterator has another element or more data to retrieve.
 * @param iter This is a pointer to the iterator check.
 * @return This returns true if there is another element or more data has not been retrieved,
 * 		   else it returns false.
 */
bool fll_hasNext(struct linkedListIterator* iter);

/**
 * This function gets the data stored in the current node of the iterator and iterates to
 * the next node in the list.
 * @param iter This is a pointer to the iterator to access the data of and iterate.
 */
void* fll_next(struct linkedListIterator* iter);

#endif /*FORWARDLINKEDLIST_H*/
