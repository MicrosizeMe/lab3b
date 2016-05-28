#include "map.h"
#include <stdlib.h> 

//Get the bucket for a block number.
unsigned int inodeMap_hash(InodeMap* map, unsigned int blockNumber) {
	return blockNumber % map->bucketSize; 
}

//Initializes a InodeEntryList
void inodeEntryList_init(InodeEntryList* list) {
	list->maxSize = 10;
	list->size = 0;
	list->entries = malloc(list->maxSize * sizeof(InodeEntry*));
}

//Increase the max size of a list
void inodeEntryList_reallocate(InodeEntryList* list) {
	list->maxSize *= 2;
	list->entries = realloc(list->entries, list->maxSize * sizeof(InodeEntry*));
}

//Add the specified InodeEntry to the list. 
void inodeEntryList_add(InodeEntryList* list, InodeEntry* entry) {
	if (list->size >= list->maxSize) {
		//Must reallocate
		inodeEntryList_reallocate(list);
	}

	//Add the element
	list->entries[list->size] = entry;
	list->size++;
}

//Initializes the map
void inodeMap_init(InodeMap* map, int bucketSize) {
	map->bucketSize = bucketSize;
	map->hashtable = malloc(bucketSize * sizeof(InodeEntryList));

	for (int i = 0; i < bucketSize; i++) {
		inodeEntryList_init(&(map->hashtable[i]));
	}
}

//Adds an entry to the map
void inodeMap_add(InodeMap* map, InodeEntry* entry) {
	unsigned int hashNumber = inodeMap_hash(map, entry->blockNumber);

	inodeEntryList_add(&(map->hashtable[hashNumber]), entry);
}

//Search for if entries exist in the map. If so, returns all of them. 
//If not, returns null.
InodeEntryList* inodeMap_search(InodeMap* map, unsigned int blockNumber) {
	unsigned int hashNumber = inodeMap_hash(map, blockNumber);

	InodeEntryList currentList = map->hashtable[hashNumber];
	
	InodeEntryList* returnList = malloc(sizeof(InodeEntryList));
	inodeEntryList_init(returnList);

	for (int i = 0; i < currentList.size; i++) {
		if (currentList.entries[i]->blockNumber == blockNumber) {
			inodeEntryList_add(returnList, currentList.entries[i]);
		}
	}
	return returnList;
}