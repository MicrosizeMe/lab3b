#include <stdio.h>

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
typedef inodeStructure Inode;

Inode* listedInodes;
int listedInodesSize;

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
int getCellRow(FILE* csv, char* buffer) {
	char * line = NULL;
	size_t size = 0;
}

//Given a certain column and line data, returns the colth column and stores to buffer
//(Null terminated). Returns the cell length of that buffer (not including the null byte)
int getCell(int col, char* line, char* buffer) {

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
	char* line = NULL;
	size_t len = 0;
	ssize_t read; 

	while ((read = getline(&line, &len, superCsv)) != -1) {
		//should only really be the first line

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

	//Create indirection data structure
	//For each inode in inode.csv
		//Check all block pointers are valid pointers 
		//(pointing to locations that exist in fs)

		//Record block number referenecd to inode number in map

		//List inode in listedInodes
			//Link link count found in inode, 

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