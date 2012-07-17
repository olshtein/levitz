#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#define NUM_OF_CHARS_IN_KB (1024/2) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define FIRST_HALF (0)
#define SECOND_HALF (_flashSize_in_chars/2)
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_SIZE (sizeof(FileHeader))

#define READING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*8)/FILE_HEADRES_SIZE) // number of FileHeaders that can be read from the flash in a single read command

#pragma pack(1)
typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	char name[8];
	uint16_t dataPointer;
}FileHeader;

typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	unsigned reserved:6;
}Signature;

#pragma pack()
unsigned _flashSize_in_chars;
unsigned headerStartPos;
unsigned dataStartPos;
unsigned next_avilable_header_pos
unsigned next_avilable_data_pos;
unsigned headerFiles_num;


/*

  Description:
	Initialize the file system.
	Must be called before any other operation on the file system.

  Arguments:
	settings - initialization information required to initialize the file system.

 */
FS_STATUS fs_init(const FS_SETTINGS settings){
	_flashSize_in_chars=(settings.block_count*2)*NUM_OF_CHARS_IN_BLOCK;
	FileHeader tmpBuffer[READING_HEADRS_SIZE];
	result_t status= flash_read(FIRST_HALF, READING_HEADRS_SIZE, (uint8_t[]) tmpBuffer);
	Signature firstHalf=((Signature*)&tmpBuffer)[0];
	FileHeader * currentHeader=(FileHeader *)(((Signature*)&firstHalf)+1);
	if(firstHalf->valid==UNUSED){
		firstHalf->valid=USED;
		headerStartPos=(uint16_t)&currentHeader;
		dataStartPos=SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
		currentHeader->dataPointer=dataStartPos;
		next_avilable_header_pos=(uint16_t)&currentHeader;
		next_avilable_data_pos=currentHeader->dataPointer;
		headerFiles_num=0;
	}
	else{
		if(firstHalf->valid==USED){
			headerFiles_num=0;
			while(currentHeader->valid==USED){

			}

		}
		else{//firstHalf->valid==DELETED

		}
	}
}

