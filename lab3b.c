#include <stdio.h>
#include <stdlib.h>


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
}
typedef struct indirectLinkStructure IndirectLink;

//List of inodes
Inode* listedInodes;
int listedInodesSize;

//List of indirect block links
IndirectLink* indirectLinks;
int indirectLinksSize;

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
		if (*buffer[i] == '"'){
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
	//find size of needed buffer
	int cellSize = 0;
	while (line[i + cellSize] != ',' || line[i + cellSize] != '\n' || line[i + cellSize] != '\0'){
		cellSize++;
	}
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
	unsigned char lineBuffer[1024];
	int lineLength = getCellRow(superCsv, lineBuffer);
	unsigned char cellBuffer[32];
	//Get inode count
	int cellLength = getCell(1, lineBuffer, cellBuffer, lineLength); 
	cellLength = getIntFromDecCell(cellBuffer, cellLength);
	//Get block count
	cellLength = getCell(2, lineBuffer, cellBuffer, lineLength); 
	blockCount = getIntFromDecCell(cellBuffer, cellLength);
	//Get block size
	cellLength = getCell(3, lineBuffer, cellBuffer, lineLength); 
	blockSize = getIntFromDecCell(cellBuffer, cellLength);
	//Get blocksPerGroup
	cellLength = getCell(5, lineBuffer, cellBuffer, lineLength); 
	blocksPerGroup = getIntFromDecCell(cellBuffer, cellLength);
	//Get inodesPerGroup
	cellLength = getCell(6, lineBuffer, cellBuffer, lineLength); 
	inodesPerGroup = getIntFromDecCell(cellBuffer, cellLength);
	//get firstDataBlock
	cellLength = getCell(8, lineBuffer, cellBuffer, lineLength); 
	firstDataBlock = getIntFromDecCell(cellBuffer, cellLength);
}

void initIndirectStructure() {
	unsigned char lineBuffer[1024];
	unsigned char cellBuffer[32];

	//Initialize buffer
	int maxSize = 5127; //No reason at all we'd pick this number. Nope.
	indirectLinks = malloc(maxSize * sizeof(IndirectLink));
	indirectLinksSize = 0;

	int lastIndex = 0;
	int lastContainingBlockNumber = 0;

	int lineLength = 0;
	int cellLength = 0;
	while (1) {
		lineLength = getCellRow(indirectCsv, lineBuffer);
		if (lineLength == -1) break; //Reached end of indirects
		
		//Get the block number of the containing block
		cellLength = getCell(0, lineBuffer, cellBuffer, lineLength);
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
		cellLength = getCell(1, lineBuffer, cellBuffer, lineLength);
		unsigned int blockPointerIndex = getIntFromDecCell(cellBuffer, cellLength);

		//Get block pointer value
		cellLength = getCell(2, lineBuffer, cellBuffer, lineLength);
		unsigned int blockPointerValue = getIntFromHexCell(cellBuffer, cellLength);

		//Assign to indirectLinks[lastIndex];
		indirectLinks[lastIndex].blockPointers[blockPointerIndex] = blockPointerValue;
	}
}

void initInodes() {
	//For each inode in inode.csv
		//Check all block pointers are valid pointers 
		//(pointing to locations that exist in fs)

		//Record block number referenecd to inode number in map

		//List inode in listedInodes
			//Link link count found in inode, 
	unsigned char lineBuffer[1024];
	unsigned char cellBuffer[32];

	//Initialize the buffer
	int maxSize = 187; //No reason at all we'd pick this number.
	listedInodes = malloc(maxSize * sizeof(Inode));
	listedInodesSize = 0;

	int lineLength;
	int cellLength;
	while (1) {
		lineLength = getCellRow(indirectCsv, lineBuffer);
		if (lineLength == -1) break; //Reached end of inodes
		if (listedInodesSize >= maxSize) { //Must realloc
			maxSize *= 2;
			listedInodes = realloc(maxSize * sizeof(Inode));
		}
		//Each inode is unique, so store newest one at the current node.
		
		//Get inode number
		cellLength = getCell(0, lineBuffer, cellBuffer, lineLength);
		listedInodes[listedInodesSize].inodeNumber 
			= getIntFromDecCell(cellBuffer, cellLength);
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

	//Starting from root, for every directory listing 
		//Find if corresponding inode exists in listedInodes
			//Else print to some data structure marking that fact
		//Increment link count of corresponding inodes
		//Check parent and self listing

	//For every inode not in the free list
		//Check that it exists in listedInodes
}

int main (int argc, const char* argv[]) {
	initializeDataStructures();
}