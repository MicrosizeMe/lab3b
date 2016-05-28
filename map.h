#ifndef MAP_H
#define MAP_H

struct InodeEntryStructure {
	unsigned int blockNumber;
	unsigned int inodeNumber;
	int indirectBlockNumber; //Negative one if irrelevant
	unsigned int entryNumber;
};
typedef struct InodeEntryStructure InodeEntry;

struct InodeEntryListStructure {
	unsigned int maxSize;
	unsigned int size;
	InodeEntry** entries;
};
typedef struct InodeEntryListStructure InodeEntryList;

struct InodeMapStructure{
	unsigned int bucketSize;
	InodeEntryList* hashtable;
};
typedef struct InodeMapStructure InodeMap;

//Initializes the map
void inodeMap_init(InodeMap* map, int bucketSize);

//Add an entry to the map
void inodeMap_add(InodeMap* map, InodeEntry* entry);

//Search for if entries exist in the map. If so, returns all of them. 
//If not, returns null.
InodeEntryList* inodeMap_search(InodeMap* map, unsigned int blockNumber);

#endif