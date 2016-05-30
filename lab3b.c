#include <stdio.h>
#include <stdlib.h>
#include "map.h"
#include <string.h>


FILE* superCsv;
FILE* groupCsv;
FILE* bitmapCsv;
FILE* inodeCsv;
FILE* directoryCsv;
FILE* indirectCsv;

FILE* lab3bCheck;

struct inodeStructure {
	unsigned int inodeNumber;
	char fileType;
	unsigned int nodeLinkCount;
	unsigned int dirLinkCount;
	unsigned int blockPointers[15]; //Lists block pointers 0 through 14, with 12-14
						//being indirect, doubly indirect, triple indirect
};
typedef struct inodeStructure Inode;

struct indirectLinkStructure {
	unsigned int blockNumber;
	unsigned int* blockPointers; //Size of this array is always blockSize / 4
};
typedef struct indirectLinkStructure IndirectLink;

//List of inodes
Inode* listedInodes;
int listedInodesSize;

//Map of data block to listing inode relations
InodeMap* blockPointerToInodeMap;

//List of indirect block links
IndirectLink* indirectLinks;
int indirectLinksSize;

//List of free inode numbers
unsigned int* freeInodeList;
int freeInodeListSize;

//List of free data blocks
unsigned int* freeDatablockList;
int freeDatablockListSize;

//Lists data bitmap blocks
unsigned int* dataBitmapLocations;
int dataBitmapLocationsSize;

//Lists inode bitmap blocks
unsigned int* inodeBitmapLocations;
int inodeBitmapLocationsSize;


//Superblock info
int inodeCount;
int blockCount;
int blockSize;
int blocksPerGroup;
int inodesPerGroup;
int firstDataBlock; //Data blocks from zero to here are invalid


//Gets the next cell row of the csv file and stores the characters in buffer 
//(null terminated). Returns the length of buffer, -1 if there are no more lines
//(not including the null byte).
// ** BUFFER ASSUMED TO BE NULL **
int getCellRow(FILE* csv, char** buffer) {
	//char * line = NULL;
	size_t size = 0;
	ssize_t read;

	read = getline(buffer, &size, csv);

	if (read == -1)
		return read;

	//traverse entire line looking for quotations
	int i;
	int quoteCount = 0;
	for (i = 0; i < size; i++){
		if ((*buffer)[i] == '"'){
			quoteCount++;
		}
	}

	//if odd number of quotations, loop until next one is found
	char * line2;
	size_t size2;
	ssize_t read2;

	while ((quoteCount % 2) == 1){
		line2 = NULL;
		size2 = 0;

		read2 = getline(&line2, &size2, csv);
		if (read2 == -1)
			return read2;

		//append line2 to line
		size += (size2 + 1);
		*buffer = (char *) realloc(*buffer, size);
		strncat(*buffer, line2, size2);

		//updates quote count
		for (i = 0; i < size2; i++){
			if (line2[i] == '"'){
				quoteCount++;
			}
		}
	}

	return size;
}

//Given a certain column and line data, returns the colth column and stores to buffer
//(Null terminated). Returns the cell length of that buffer (not including the null byte)
//Returns -1 if invalid column number (too high)
int getCell(int col, char* line, char** buffer, int size) {
	//find starting point
	int i = 0;
	int curCol = 0;
	int inQuotes = 0;
	while (curCol < col){
		if (line[i] == '"'){
			inQuotes = (inQuotes + 1) % 2;
		} else if (inQuotes == 0 && line[i] == ',') {
			curCol++;
		} else if (inQuotes == 0 && (line[i] == '\0' || line[i] == '\n')){
			return -1;
		}
		i++;
	}
	//printf("i: %d\n", i);
	//find size of needed buffer, INCLUDING null byte
	int cellSize = 1;
	while (line[i + cellSize-1] != ',' && line[i + cellSize-1] != '\n' && line[i + cellSize-1] != '\0') {
		cellSize++;
		//	printf("cellSize: %d\n", cellSize);
	}
	//printf("Final size: %d\n", cellSize);

	//free buffer if not null and allocate
	if (*buffer == NULL){
		free(*buffer);
	}

	*buffer = (char *)malloc(cellSize * sizeof(char));


	//put letters into buffer
	int j = 0;
	//printf("i: %d j: %d cellSize: %d\n", i, j, cellSize);
	while (j < cellSize - 1){
		(*buffer)[j] = line[i + j];
		j++;
	}

	(*buffer)[j] = '\0';

	return (cellSize - 1);
}

//Returns the corresponding integer from cell assumed to be storing decimal info 
unsigned int getIntFromDecCell(unsigned char* buffer, size_t count) {
	unsigned int returnInt = 0;
	for (int i = 0; i < count; i++) {
		returnInt = (returnInt * 10) + (buffer[i] - '0');
	}
	return returnInt;
}

//Returns the corresponding integer from cell assumed to be storing lower case hex info
unsigned int getIntFromHexCell(unsigned char* buffer, size_t count) {
	unsigned int returnInt = 0;
	for (int i = 0; i < count; i++) {
		int number = buffer[i] - '0';
		if (number >= 10) {
			number = 10 + buffer[i] - 'a';
		}
		returnInt = (returnInt * 16) + number;
	}
	return returnInt;
}

//Returns a particular Inode structure given a number, or NULL if it doesn't exist.
Inode* getInode(unsigned int inodeNumber) {
	for (int i = 0; i < listedInodesSize; i++) {
		if (listedInodes[i].inodeNumber == inodeNumber)
			return &listedInodes[i];
	}
	return NULL;
}

//Initializes the super block information based on superCsv
void initSuperBlock() {
	char* lineBuffer = NULL;
	int lineLength = getCellRow(superCsv, &lineBuffer);
	char* cellBuffer = NULL;
	//Get inode count
	int cellLength = getCell(1, lineBuffer, &cellBuffer, lineLength); 
	inodeCount = getIntFromDecCell(cellBuffer, cellLength);
	//Get block count
	cellLength = getCell(2, lineBuffer, &cellBuffer, lineLength); 
	blockCount = getIntFromDecCell(cellBuffer, cellLength);
	//Get block size
	cellLength = getCell(3, lineBuffer, &cellBuffer, lineLength); 
	blockSize = getIntFromDecCell(cellBuffer, cellLength);
	//Get blocksPerGroup
	cellLength = getCell(5, lineBuffer, &cellBuffer, lineLength); 
	blocksPerGroup = getIntFromDecCell(cellBuffer, cellLength);
	//Get inodesPerGroup
	cellLength = getCell(6, lineBuffer, &cellBuffer, lineLength); 
	inodesPerGroup = getIntFromDecCell(cellBuffer, cellLength);
	//get firstDataBlock
	cellLength = getCell(8, lineBuffer, &cellBuffer, lineLength);
	firstDataBlock = getIntFromDecCell(cellBuffer, cellLength);

	free(lineBuffer);
	free(cellBuffer);
}

IndirectLink* getIndirectLink(unsigned int blockNumber) {
	for (int i = 0; i < indirectLinksSize; i++) {
		if (indirectLinks[i].blockNumber == blockNumber)
			return &(indirectLinks[i]);
	}
	return NULL;
}

void initIndirectStructure() {
	char* lineBuffer = NULL;
	char* cellBuffer = NULL;

	//Initialize buffer
	int maxSize = 5127; //No reason at all we'd pick this number. Nope.
	indirectLinks = malloc(maxSize * sizeof(IndirectLink));
	indirectLinksSize = 0;

	int lastIndex = 0;
	int lastContainingBlockNumber = 0;

	int lineLength = 0;
	int cellLength = 0;
	while (1) {
		lineLength = getCellRow(indirectCsv, &lineBuffer);
		if (lineLength == -1) break; //Reached end of indirects
		
		//Get the block number of the containing block
		cellLength = getCell(0, lineBuffer, &cellBuffer, lineLength);
		unsigned int thisBlock = getIntFromHexCell(cellBuffer, cellLength); 
		if (thisBlock != lastContainingBlockNumber) { //Different block number as the last 
													  //one in the list. Either block
													  //already exists or must be created
			//Assume block exists
			int i;
			for (i = 0; i < indirectLinksSize; i++) {
				if (thisBlock == indirectLinks[i].blockNumber) { //Found it
					break;
				}
			}
			if (i >= indirectLinksSize) { //Didn't find it
				if (indirectLinksSize >= maxSize) { //Must reallocate
					maxSize *= 2;
					indirectLinks = realloc(indirectLinks, maxSize * sizeof(IndirectLink));
				}
				//Must initialize
				indirectLinks[i].blockNumber = thisBlock;
				indirectLinks[i].blockPointers 
					= malloc((blockSize / 4) * sizeof(unsigned int));
				for (int j = 0; j < (blockSize / 4); j++) {
					indirectLinks[i].blockPointers[j] = 0;
				}
				indirectLinksSize++;
			}
			//Change previous pointers
			lastIndex = i;
			lastContainingBlockNumber = thisBlock;
		}

		//Get entry index
		cellLength = getCell(1, lineBuffer, &cellBuffer, lineLength);
		unsigned int blockPointerIndex = getIntFromDecCell(cellBuffer, cellLength);

		//Get block pointer value
		cellLength = getCell(2, lineBuffer, &cellBuffer, lineLength);
		unsigned int blockPointerValue = getIntFromHexCell(cellBuffer, cellLength);

		//Assign to indirectLinks[lastIndex];
		indirectLinks[lastIndex].blockPointers[blockPointerIndex] = blockPointerValue;
	}

	free(lineBuffer);
	free(cellBuffer);
}

//Returns whether the block pointer specified is valid for this file system. 0 is valid
//as a null pointer.
int isBlockPointerValid(unsigned int pointer) {
	return (pointer != 1 && pointer < blockCount);
}

//Adds all the indirect blocks for a certain block pointer into the map. Recursively 
//calls the rest at certain levels. 
void addIndirect(unsigned int blockPointer, unsigned int inodeNumber, int level) {
	if (level == 0) return; 
	IndirectLink* currentLink = getIndirectLink(blockPointer);
	for (int i = 0; i < (blockSize / 4); i++) {
		unsigned int currentBlockPointer = currentLink->blockPointers[i];
		if (currentBlockPointer != 0 && isBlockPointerValid(currentBlockPointer)) {
			InodeEntry* entry = malloc(sizeof(InodeEntry));
			entry->blockNumber = currentBlockPointer;
			entry->inodeNumber = inodeNumber;
			entry->indirectBlockNumber = blockPointer;
			entry->entryNumber = i;
			inodeMap_add(blockPointerToInodeMap, entry);
		}
		else if (currentBlockPointer != 0) {
			//Printing invalid block pointer #7
			fprintf(lab3bCheck, 
				"INVALID BLOCK < %u > IN INODE < %u > INDIRECT BLOCK < %u > ENTRY < %d >\n",
				currentBlockPointer, inodeNumber, blockPointer, i);
		}
		else break;
	}
	if (level > 0) {
		for (int i = 0; i < (blockSize / 4); i++) {
			unsigned int currentBlockPointer = currentLink->blockPointers[i];
			if (currentBlockPointer == 0) break;
			addIndirect(currentBlockPointer, inodeNumber, level - 1);
		}
	}
}

void initInodes() {
	//For each inode in inode.csv
		//Check all block pointers are valid pointers 
		//(pointing to locations that exist in fs)

		//Record block number referenecd to inode number in map

		//List inode in listedInodes
			//Link link count found in inode, 
	char* lineBuffer = NULL;
	char* cellBuffer = NULL;
	
	int lineLength;
	int cellLength;

	//Initialize the buffer
	int maxSize = 187; //No reason at all we'd pick this number.
	listedInodes = malloc(maxSize * sizeof(Inode));
	listedInodesSize = 0;

	//Initialize the map to the block pointer map
	blockPointerToInodeMap = malloc(sizeof(InodeMap));
	inodeMap_init(blockPointerToInodeMap, 1024);

	while (1) {
		lineLength = getCellRow(inodeCsv, &lineBuffer);
		if (lineLength == -1) break; //Reached end of inodes
		if (listedInodesSize >= maxSize) { //Must realloc
			maxSize *= 2;
			listedInodes = realloc(listedInodes, maxSize * sizeof(Inode));
		}
		//Each inode is unique, so store newest one at the current node.
		
		//Get inode number
		cellLength = getCell(0, lineBuffer, &cellBuffer, lineLength);
		listedInodes[listedInodesSize].inodeNumber 
			= getIntFromDecCell(cellBuffer, cellLength);

		//Get filetype char
		cellLength = getCell(1, lineBuffer, &cellBuffer, lineLength);
		listedInodes[listedInodesSize].fileType = cellBuffer[0];
		
		//Get link count (as specified by inode)
		cellLength = getCell(5, lineBuffer, &cellBuffer, lineLength);
		listedInodes[listedInodesSize].nodeLinkCount 
			= getIntFromDecCell(cellBuffer, cellLength);

		//Initialize dirLinkCount to zero (will be set by directory traversal)
		listedInodes[listedInodesSize].dirLinkCount = 0;
		
		//Block pointers start from cell 11. 
		for (int i = 0; i < 15; i++) {
			cellLength = getCell(11 + i, lineBuffer, &cellBuffer, lineLength);
			listedInodes[listedInodesSize].blockPointers[i]
				= getIntFromHexCell(cellBuffer, cellLength);

			//Check validity. If valid, put block reference into the map. If not,
			//immediately print the error
			unsigned int blockPointer = getIntFromHexCell(cellBuffer, cellLength);
			if (blockPointer != 0 && isBlockPointerValid(blockPointer)) {
				InodeEntry* entry = malloc(sizeof(InodeEntry));
				entry->blockNumber = blockPointer;
				entry->inodeNumber = listedInodes[listedInodesSize].inodeNumber;
				entry->indirectBlockNumber = -1;
				entry->entryNumber = i;
				inodeMap_add(blockPointerToInodeMap, entry);
			}
			else if (blockPointer != 0) {
				//Printing invalid block pointer #7
				fprintf(lab3bCheck, "INVALID BLOCK < %u > IN INODE < %u > ENTRY < %d >\n",
					blockPointer, listedInodes[listedInodesSize].inodeNumber, i);
			}
			else break;
		}
		//unsigned int blockPointers[15]; //Lists block pointers 0 through 14, with 12-14
							//being indirect, doubly indirect, triple indirect
		//TODO: do this for indirect
		for (int i = 1; i <= 3; i++) {
			unsigned int indirectBlockPointer 
				= listedInodes[listedInodesSize].blockPointers[11 + i];
			if (indirectBlockPointer == 0) break;
			addIndirect(indirectBlockPointer, 
				listedInodes[listedInodesSize].inodeNumber, i);
		}

		listedInodesSize++;
	}

	for (int i = 0; i < listedInodesSize; i++) {
		printf("%u, %C, %u, %u, ", 
			listedInodes[i].inodeNumber, listedInodes[i].fileType, 
			listedInodes[i].nodeLinkCount, listedInodes[i].dirLinkCount);
		for (int j = 0; j < 15; j++) {
			printf("%x, ", listedInodes[i].blockPointers[j]);
		}
		printf("\n");
	}

	free(lineBuffer);
	free(cellBuffer);
}

void initFreeList() {
	//For every row in the free list
		//Check that inodes not referenced exist in listedInodes
		//Store data block in list
	char* lineBuffer = NULL;
	char* cellBuffer = NULL;

	int lineLength;
	int cellLength;

	//Initialize bitmap location data structures
	//Data bitmap locations
	dataBitmapLocations = malloc(sizeof(unsigned int) * 10);
	dataBitmapLocationsSize = 0;
	int dataBitmapLocationsMaxSize = 10;

	//Inode bitmap locations
	inodeBitmapLocations = malloc(sizeof(unsigned int) * 10);
	inodeBitmapLocationsSize = 0;
	int inodeBitmapLocationsMaxSize = 10;

	//Initialize group info
	while (1) {
		lineLength = getCellRow(groupCsv, &lineBuffer);
		if (lineLength == -1) break;

		//Check if cells must be reallocated
		if (dataBitmapLocationsSize >= dataBitmapLocationsMaxSize) {
			//Reallocate data bitmap locations size
			dataBitmapLocationsMaxSize *= 2;
			dataBitmapLocations = realloc(dataBitmapLocations, 
				sizeof(unsigned int) * dataBitmapLocationsMaxSize);
		}
		if (inodeBitmapLocationsSize >= inodeBitmapLocationsMaxSize) {
			//Reallocate inode bitmap locations size
			inodeBitmapLocationsMaxSize *= 2;
			inodeBitmapLocations = realloc(inodeBitmapLocations, 
				sizeof(unsigned int) * inodeBitmapLocationsMaxSize);
		}

		//Cells 4,5, for inode bitmap and block bitmap
		//Get inode bitmap location
		cellLength = getCell(4, lineBuffer, &cellBuffer, lineLength);
		inodeBitmapLocations[inodeBitmapLocationsSize] 
			= getIntFromHexCell(cellBuffer, cellLength);
		inodeBitmapLocationsSize++;

		//Get data bitmap location
		cellLength = getCell(5, lineBuffer, &cellBuffer, lineLength);
		dataBitmapLocations[dataBitmapLocationsSize] 
			= getIntFromHexCell(cellBuffer, cellLength);
		dataBitmapLocationsSize++;
	}

	//Initialize free lists
	//Free inode list
	freeInodeList = malloc(sizeof(unsigned int) * inodeCount);
	freeInodeListSize = 0;
	int freeInodeListMaxSize = inodeCount;

	//Free data block list
	freeDatablockList = malloc(sizeof(unsigned int) * blockCount);
	freeDatablockListSize = 0;
	int freeDatablockListMaxSize = blockCount;

	//Traverse free list. While traversing free list (since free list is sorted),
	//record previous inode. Do checking on skipped inodes. 
	unsigned int previousInode = 0;
	while (1) {
		lineLength = getCellRow(bitmapCsv, &lineBuffer);
		if (lineLength == -1) break;

		//Check if free lists must be reallocated (they won't)
		if (freeInodeListSize >= freeInodeListMaxSize) {
			freeInodeListMaxSize *= 2;
			freeInodeList = realloc(freeInodeList, 
				sizeof(unsigned int) * freeInodeListMaxSize);
		}
		if (freeDatablockListSize >= freeDatablockListMaxSize) {
			freeDatablockListMaxSize *= 2;
			freeDatablockList = realloc(freeDatablockList, 
				sizeof(unsigned int) * freeDatablockListMaxSize);
		}

		//Check the data block of a given entry
		cellLength = getCell(0, lineBuffer, &cellBuffer, lineLength);
		unsigned int bitmapDataBlock
			= getIntFromHexCell(cellBuffer, cellLength);
		//Get the corresponding free element
		cellLength = getCell(1, lineBuffer, &cellBuffer, lineLength);
		unsigned int freeElement
			= getIntFromDecCell(cellBuffer, cellLength);

		//Check if it's inode or data
		int dataBlock = 0;
		for (int i = 0; i < dataBitmapLocationsSize; i++) {
			if (bitmapDataBlock == dataBitmapLocations[i]) {
				//Is a data block
				dataBlock = 1;
				//Add to data free list
				freeDatablockList[freeDatablockListSize] = freeElement;
				freeDatablockListSize++;
			}
		}
		if (dataBlock == 0) {
			//It's an inode. Add to inode free list.
			freeInodeList[freeInodeListSize] = freeElement;
			freeInodeListSize++;
			//Anything from previous inode to freeElement exclusive has not been touched
			//by the free list. Check for integrity.
			for (unsigned int i = previousInode + 1; i < freeElement; i++) {
				//Check that inodes not on free list are on the list of inodes. If not, 
				//output error. 
				int inodeFound = 0;
				for (int j = 0; j < listedInodesSize; j++) {
					if (listedInodes[j].inodeNumber == i) {
						inodeFound = 1;
						printf("Inode %4u found in inode list\n", i);
						//Check if the inode specified actually has zero reference counts
						//(since that's the definition of unused, apparently)
						if (listedInodes[j].dirLinkCount == 0) {
							inodeFound = 0;
						}
						break;
					}
				}
				if (!inodeFound) {
					//Didn't find the inode, so print
					fprintf(lab3bCheck, 
						"MISSING INODE < %u > SHOULD BE IN FREE LIST < %u >\n",
						i, inodeBitmapLocations[i / inodesPerGroup]);
				}
			}
			previousInode = freeElement;
		}
	}

	free(lineBuffer);
	free(cellBuffer);
}

void checkDuplicateBlocks() {
	//For every data block that exists
		//Check for existance on the free list
		//Check for duplicate entries
			//If such exist, check that it exists in free list. Print accordingly.
	for (unsigned int i = 0; i < blockCount; i++) {
		//Get entries for i
		InodeEntryList* entries = inodeMap_search(blockPointerToInodeMap, i);
		//If there's an entry, we're interested. 
		if (entries->size != 0) {
			int blockFoundInFreeList = 0;
			for (int j = 0; j < freeDatablockListSize; j++) {
				if (freeDatablockList[j] == i) {
					//Found it in the free list
					blockFoundInFreeList = 1;
					break;
				}
			}
			//If there's an entry and we found it on the free list, we have an 
			//unallocated block error. Print an error for all situations we found.
			if (blockFoundInFreeList) {
				fprintf(lab3bCheck, "UNALLOCATED BLOCK < %u > REFERENCED BY ", i);
			}
			//Otherwise, we're only interested if there's more than one entry. 
			else if (entries->size > 1) {
				fprintf(lab3bCheck, "MULTIPLY REFERENCED BLOCK < %u > BY ", i);
			}

			//Print all block references if needed.
			if (blockFoundInFreeList || entries->size > 1) {
				for (unsigned int j = 0; j < entries->size; j++) {
					InodeEntry entry = *(entries->entries[j]);
					if (entry.indirectBlockNumber != -1) {
						fprintf(lab3bCheck,
							"INODE < %u > INDIRECT BLOCK < %d > ENTRY < %u > ",
							entry.inodeNumber, 
							entry.indirectBlockNumber,
							entry.entryNumber
						);
					}
					else {
						fprintf(lab3bCheck,
							"INODE < %u > ENTRY < %u > ",
							entry.inodeNumber, 
							entry.entryNumber
						);	
					}
				}
				fprintf(lab3bCheck, "\n");
			}
		}
	}
}


void initializeDataStructures() {
	superCsv = fopen("super.csv", "r");
	groupCsv = fopen("group.csv", "r");
	bitmapCsv = fopen("bitmap.csv", "r");
	inodeCsv = fopen("inode.csv", "r");
	directoryCsv = fopen("directory.csv", "r");
	indirectCsv = fopen("indirect.csv", "r");

	lab3bCheck = fopen("lab3b_check.txt", "w+");

	//Initialize maps, lists, so on

	//Read super.csv
	initSuperBlock();

	//Initalize indirect link structure;
	initIndirectStructure();
	//Init inode structure
	//For each inode in inode.csv
		//Check all block pointers are valid pointers 
		//(pointing to locations that exist in fs)

		//Record block number referenecd to inode number in map

		//List inode in listedInodes
			//Link link count found in inode, 
	initInodes();


	//Starting from root, for every directory listing 
		//Find if corresponding inode exists in listedInodes
			//Else print to some data structure marking that fact
		//Increment link count of corresponding inodes
		//Check parent and self listing

	//Init free list
	//For every row in the free list
		//Check that inodes not referenced exist in listedInodes
		//Store data block in list
	initFreeList();
	
	//For every data block that exists
		//Check for duplicate entries
			//If such exist, check that it exists in free list. Print accordingly.
	checkDuplicateBlocks();
}

int main (int argc, const char* argv[]) {
	initializeDataStructures();

/*
	//checking if getCellRow and getCell work
	char * line = NULL;
	char * cell = NULL;	
	int lineLen = getCellRow(superCsv, &line);
	int cellLen = 0;
	int totalCols = 9;
	int curCol;

	while (lineLen != -1){
		printf("New row\n");
		for (curCol = 0; curCol < totalCols; curCol++){
			cellLen = getCell(curCol, line, &cell, lineLen);
			if (cellLen == -1){
				printf("Column counting error on column %d\n", curCol);
				break;
			}
			printf("%d\t%s\n", curCol, cell);
		}
		printf("\n");

		free(line);
		lineLen = getCellRow(superCsv, &line);
	}
*/
}