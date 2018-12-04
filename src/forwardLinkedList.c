/**
 * This file contains the implementation of the linked list functions
 * using the structures defined in the header file.
 * @file linkedlist.c
 * @author Max Kallenberger,Marcus Mueller
 * @date September 17, 2017, December 2018
 */

#include "forwardLinkedList.h"

/**
 * This function initializes the elements in the linkedList structure to default values.
 * @param list This is a pointer to the list to initialize.
 */
void fll_init(struct linkedList* list){
	// Checks if the list parameter is NULL to avoid a null pointer dereference
	if(list != NULL){
		// Initializes the list values
		list->head = NULL;
		list->tail = NULL;
		list->size = 0;
	}
}

/**
 * This function adds an element to the linked list at the end of the list.
 * @param list This is a pointer to the list to add to.
 * @param object This is a pointer to the object to be added to the list.
 * @param size This is the size of the object being added in bytes.
 * @return This returns true if the add was successful, false if it failed.
 */
bool fll_add(struct linkedList* list, const void* object, uint32_t size){
	bool completed = false;

	// Checks all parameters for valid values to avoid null pointer dereferences
	if((list != NULL) && (object != NULL) && (size != 0)){
		// Allocate space in memory for the new node and data
		struct listNode* node = (struct listNode*)malloc(sizeof(struct listNode));
		node->data = (void*)malloc(size);

		// Copy the data from the object to the node
		memcpy(node->data, object, size);

		// Set the node structure elements
		node->nextNode = NULL;

		// Check if the list tail is null to avoid a null pointer dereference
		if(list->tail != NULL){
			// Point the old tails next node to the new node
			list->tail->nextNode = node;
		}

		// The new tail is the new node
		list->tail = node;

		// If the list was empty, the new node is the head
		if(list->size == 0){
			list->head = node;
		}

		// Increase the list size to accurately represent the number of nodes contained in the list
		list->size = (list->size) + 1;


		completed = true;
	}

	return completed;
}

/**
 * This function adds an element to the linked list at the desired index.
 * @param list This is a pointer to the list to add to.
 * @param object This is a pointer to the object to be added to the list.
 * @param size This is the size of the object being added in bytes.
 * @param index This is the index to add the object at.
 * @return This returns true if the add was successful, false if it failed.
 */
bool fll_addIndex(struct linkedList* list, const void* object, uint32_t size, uint32_t index){
	bool completed = false;

	// Check the parameters for valid values to avoid null pointer dereferencing and index out of bounds errors
	if((list != NULL) && (object != NULL) && (size != 0) && (index >= 0) && (index < list->size)){
		// Allocate space in memory for the new node and data
		struct listNode* node = (struct listNode*)malloc(sizeof(struct listNode));
		node->data = malloc(size);

		// Copy the data from the object to the node
		memcpy(node->data, object, size);

		// If the new node is inserted at the first index it is the head
		if(index == 0) {
			// First set the list head to the new node's next node
			node->nextNode = list->head;
			// Then set the list head as this node
			list->head = node;
		// Otherwise we need to iterate through the list in order to add at the correct index
		} else {
			// Iterate to one node before the index where the new node is to be added
			struct linkedListIterator* iter = fll_getIterator(list);
			for(int i = 0; i < index-1; i++) {
				fll_next(iter);
			}
			// Grab the node that is after the node being inserted
			struct listNode* after  = iter->current->nextNode;
			// Set the new node to point to the old node at this index next
			iter->current->nextNode = node;
			// Set the new node's next node to be the node that was after the iterator node
			node->nextNode = after;

			// Free the iterator to avoid a memory leak
			free(iter);
		}
		// Increase the list size to accurately represent the number of nodes contained in the list
		list->size = list->size + 1;
		completed = true;
	}
	// If the index to add at is the last index, run the simple add function to avoid redundant code
	else if((list != NULL) && (index == list->size)){
		completed = fll_add(list, object, size);
	}

	return completed;
}

/**
 * This function removes an object from the list at a given index.
 * @param list This is a pointer to the list to remove from.
 * @param index This is the index to remove the object from.
 * @return This returns true if the remove was successful, false if it failed.
 */
bool fll_remove(struct linkedList* list, uint32_t index){
	bool completed = false;

	// Check if the parameters are valid to avoid null pointer dereferencing and index out of bounds errors
	if((list != NULL) && (index >= 0) && (index < list->size)){
		struct listNode* remove = NULL;
		// Check if we are removing the head
		if (index == 0) {
			remove = list->head;
			// Set the head to the next node in the list
			list->head = remove->nextNode;
		// Otherwise proceed with iteration
		} else {
			// Iterate to one node before the node that needs to be removed
			struct linkedListIterator* iter = fll_getIterator(list);
			for(int i = 0; i < index-1; i++){
				fll_next(iter);
			}

			remove = iter->current->nextNode;

			// Remove the requested node from the linked list
			iter->current->nextNode = iter->current->nextNode->nextNode;

			// Check if the tail was removed
			if(index == (list->size - 1)){
				// Set the iterator node to the tail
				list->tail = iter->current;
			}

			// Free the iterator to avoid memory leaks
			free(iter);
		}
		// Free the removed node
		free(remove->data);
		free(remove);
		// Decrease the list size to accurately represent the number of nodes contained in the list
		list->size = list->size - 1;

		// If the list is empty, reinitialize it to restore default values
		if(list->size == 0){
			fll_init(list);
		}

		completed = true;
	}

	return completed;
}

/**
 * This function gets the object from the desired list index.
 * @param list This is a pointer to the list to get the object from.
 * @param index This is the index to get the object from.
 */
void* fll_get(struct linkedList* list, uint32_t index){
	void* result = NULL;

	// Check if the parameters are valid values to avoid null pointer dereferences and index out of bounds errors
	if((list != NULL) && (index >= 0) && (index < list->size)){

		// Iterate to the node to retrieve data from
		struct linkedListIterator* iter = fll_getIterator(list);
		for(int i = 0; i < index; i++){
			fll_next(iter);
		}

		// Set the output to point at the data in the node
		result = iter->current->data;

		// Free the iterator to avoid memory leaks
		free(iter);
	}

	return result;
}

/**
 * This function frees the memory allocated for the nodes and data of objects in the list.
 * The elements of the linkedList structure are reinitialized to their default values.
 * @param list This is a pointer to the list to be cleared.
 */
void fll_clear(struct linkedList* list){
	// Check if list is NULL to avoid null pointer dereferencing
	if(list != NULL){

		struct linkedListIterator* iter = fll_getIterator(list);

		// Iterate through the list to make sure all nodes are cleared
		while(fll_hasNext(iter)){
			struct listNode* temp = iter->current;

			// Move to the next node to clear
			fll_next(iter);

			// Free the current node and the data in it to avoid memory leaks
			free(temp->data);
			free(temp);
		}

		// Free the iterator to avoid memory leaks
		free(iter);

		// Initialize the list to return to default values
		fll_init(list);
	}
}

/**
 * This function returns the size element of the linked list structure.
 * @param list This is a pointer to the list to return the size of.
 * @return This returns the size of the list (the number of objects contained in the list).
 */
uint32_t fll_size(struct linkedList* list){
	uint32_t num = 0;

	// Check if list is NUll to avoid null pointer dereferencing
	if(list != NULL){
		// Set the output to the size of the list
		num = list->size;
	}

	return num;
}

/**
 * This function creates an iterator for a given linked list. The iterator starts at the head of the list.
 * @param list This is a pointer to the list used to create the iterator.
 * @return This returns a pointer to the linkedListIterator structure if the iterator generated correctly,
 * 	       else it returns NULL.
 */
struct linkedListIterator* fll_getIterator(struct linkedList* list){
	struct linkedListIterator* iterator = NULL;

	// Checks if the list is NUll to avoid null pointer dereferencing
	if(list != NULL){
		// Allocate space in memory for the iterator
		iterator = (struct linkedListIterator*) malloc(sizeof(struct linkedListIterator));

		// Start the iterator at the head of the list
		iterator->current = list->head;
	}
	return iterator;
}

/**
 * This function determines if an iterator has another element or more data to retrieve.
 * @param iter This is a pointer to the iterator check.
 * @return This returns true if there is another element or more data has not been retrieved,
 * 		   else it returns false.
 */
bool fll_hasNext(struct linkedListIterator* iter){
	bool hasNext = false;

	// Checks if the iterator is NUll to avoid null pointer dereferencing
	if(iter != NULL){
		// Checks if the current node of the iterator is NUll to tell if there is still data
		// to access with the next method
		if(iter->current != NULL){
				hasNext = true;
		}
	}
	return hasNext;
}

/**
 * This function gets the data stored in the current node of the iterator and iterates to
 * the next node in the list.
 * @param iter This is a pointer to the iterator to access the data of and iterate.
 */
void* fll_next(struct linkedListIterator* iter){
	void* data = NULL;

	// Checks if the iterator is NUll to avoid null pointer dereferencing
	if(iter != NULL){
		// Checks if the current node of the iterator is NUll to avoid null pointer dereferencing
		if(iter->current != NULL){
			// Set the output to point at the data in the current node
			data = iter->current->data;
			// Iterate to the next node
			iter->current = iter->current->nextNode;
		}
	}

	return data;
}
