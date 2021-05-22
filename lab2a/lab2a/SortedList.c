// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *    The specified element will be inserted in to
 *    the specified list, which will be kept sorted
 *    in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    // Check that the list & the element to add are both valid. If not, return.
    if (list == NULL || element == NULL) {
        return;
    }
    
    // Create traveling pointer and then traverse the list.
    // Exit condition is curr != list because eventually, list will be inserted.
    // We want to exit after list is inserted, AKA when the pointer adds it then increments to it
    SortedListElement_t *curr = list->next;
    while (curr != list) {
        // Check if the element's key fits at the current location
        // <= 0 str1 is less than or the same as str 2
        if (strcmp(element->key, curr->key) <= 0)
            break;
        
        // Continue traversing the list
        curr = curr->next;
    }
    
    // Check if we should yield or not
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    
    // Location found, update the pointers as needed
    element->next = curr;
    element->prev = curr->prev;
    curr->prev->next = element;
    curr->prev = element;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *    The specified element will be removed from whatever
 *    list it is currently in.
 *
 *    Before doing the deletion, we check to make sure that
 *    next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t *element) {
    // Check if element is valid
    if (element == NULL)
        return 1;
    
    // Check if next->prev and prev->next point to this node
    if ((element->next->prev == element) && (element->prev->next == element)){
        // Check if we should yield or not
        if (opt_yield & DELETE_YIELD)
            sched_yield();
        
        // Delete the element
        element->prev->next = element->next;
        element->next->prev = element->prev;
        return 0;
    }
    
    // If we reached this point, there was an error
    return 1;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *    The specified list will be searched for an
 *    element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    // Check to make sure the list and key are valid
    if (list == NULL || key == NULL)
        return NULL;
    
    // Create traveling pointer and then traverse the list.
    SortedListElement_t *curr = list->next;
    while (curr != NULL) {
        // Check to see if the current key matches the key we're looking for
        if (strcmp(curr->key, key) == 0)
            return curr;
        
        // Check if we should yield or not
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
        
        // If we reached this point, key doesn't match and no yield. Increment pointer
        // and continue the search
        curr = curr->next;
    }
    
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *    While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *       -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list) {
    // Check if the list is valid
    if (list == NULL)
        return -1;
    
    // Create a counter variable and traveling pointer. Check the next/prev pointer
    int counter = 0;
    SortedListElement_t *curr = list->next;
    if (curr->prev != list)
        return -1;
    
    //Traverse through the list while checking previous/next pointers
    while (curr != list) {
        if (curr->next->prev != curr)
            return -1;
        
        // Check if we need to yield at this point
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
        
        // Increment and move pointer as needed
        counter++;
        curr = curr->next;
    }
    
    // Reached the end with no problems, counter has the length of the Linked List
    return counter;
}
