
FILE* superCsv;
FILE* groupCsv;
FILE* bitmapCsv;
FILE* inodeCsv;
FILE* directoryCsv;
FILE* indirectCsv;

FILE* lab3bCheck;

int main (int argc, const char* argv[]) {
	superCsv = fopen("super.csv", "r");
	groupCsv = fopen("group.csv", "r");
	bitmapCsv = fopen("bitmap.csv", "r");
	inodeCsv = fopen("inode.csv", "r");
	directoryCsv = fopen("directory.csv", "r");
	indirectCsv = fopen("indirect.csv", "r");

	lab3bCheck = fopen("lab3b_check.txt", "w+");
}