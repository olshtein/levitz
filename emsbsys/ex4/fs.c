#include "fs.h"
#include "flash.h"
#define NUM_OF_CJARS_IN_KB (1024/2) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define FIRST_HALF (0)
#define SECOND_HALF (_flashSize_in_chars/2)
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CJARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)

#pragma pack(1)
typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	char name[8];
	uint16_t dataPointer;
}FileHeader;

typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
}Signature;

#pragma pack()
unsigned _flashSize_in_chars;
unsigned headerStartPos;
unsigned dataStartPos;
unsigned next_avilable_header_pos;
unsigned next_avilable_data_pos;


/*

  Description:
	Initialize the file system.
	Must be called before any other operation on the file system.

  Arguments:
	settings - initialization information required to initialize the file system.

 */
FS_STATUS fs_init(const FS_SETTINGS settings){
	_flashSize_in_chars=(settings.block_count*2)*NUM_OF_CHARS_IN_BLOCK;

}



FS_STATUS fs_write(const char* filename, unsigned length, const char* data){
	int headerLoc=0;
	if ((headerLoc=FindFile(filename))!=0){
	int stat=	writeNewData(length,data);
	if (stat!=SUCCESS) 	return stat;
	stat=unactivateFile(headerLoc);
		return stat;
	}
return	writeNewData(length,data);
}
FS_STATUS fs_filesize(const char* filename, unsigned* length){
	int headerLoc=0;
	if ((headerLoc=FindFile(filename))!=0){
		return getLength(headerLoc,length);
	}
	else return FILE_NOT_FOUND;
}
FS_STATUS fs_erase(const char* filename){
	int headerLoc=0;
	if ((headerLoc=FindFile(filename))!=0){
		return unactivateFile(headerLoc);
	}
	else return FILE_NOT_FOUND;
}
FS_STATUS fs_count(unsigned* file_count){

}
/*
