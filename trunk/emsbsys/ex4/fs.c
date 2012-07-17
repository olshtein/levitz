#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#define NUM_OF_CHARS_IN_KB (1024/2) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define FIRST_HALF (0)
#define SECOND_HALF ((_flashSize_in_chars/2)*sizeof(char))
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_SIZE (sizeof(FileHeader))
#define CHK_STATUS(n) if(n!=SUCCESS)return n;

#define READING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*8)/FILE_HEADRES_SIZE) // number of FileHeaders that can be read from the flash in a single read command

#pragma pack(1)
typedef struct{
	uint16_t dataPointer;
	unsigned valid:2; // USED / DELETED / UNUSED
	char name[8];
}FileHeader;

typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	unsigned reserved:6;
}Signature;

#pragma pack()
unsigned _flashSize_in_chars;
uint16_t headerStartPos;
uint16_t dataStartPos;
uint16_t next_avilable_header_pos;
uint16_t next_avilable_data_pos;
unsigned headerFiles_num;


result_t restoreFileSystem(uint16_t startAdress){
	FileHeader files[READING_HEADRS_SIZE];
	result_t status=flash_read(startAdress, sizeof(FileHeader)*READING_HEADRS_SIZE,
			(uint8_t[]) files);
	CHK_STATUS(status);
	headerStartPos=startAdress;
	headerFiles_num=0;
	int i=0;
	for(;files[i].valid!=UNUSED;i++){
		if(i==READING_HEADRS_SIZE){ //need to read another headrsFiles
			startAdress+=sizeof(FileHeader)*READING_HEADRS_SIZE;
			status+=flash_read(startAdress, sizeof(FileHeader)*READING_HEADRS_SIZE,
					(uint8_t[]) files);
			CHK_STATUS(status);
			i=0;
		}
		if(files[i].valid==USED)headerFiles_num++;
	}
	next_avilable_data_pos=files[i].dataPointer;
	next_avilable_header_pos=(uint16_t)(&files[i]);
	return status;
}

void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size){
}
void flash_request_done_cb(){
}
/*

  Description:
	Initialize the file system.
	Must be called before any other operation on the file system.

  Arguments:
	settings - initialization information required to initialize the file system.

 */
FS_STATUS fs_init(const FS_SETTINGS settings){
	if(settings.block_count!=16)return COMMAND_PARAMETERS_ERROR;
	result_t status=flash_init(flash_data_recieve_cb,flash_request_done_cb);
	CHK_STATUS(status);
	_flashSize_in_chars=(settings.block_count)*NUM_OF_CHARS_IN_BLOCK;
	Signature TmpBuffer[2];

	status+= flash_read(FIRST_HALF, sizeof(Signature), (uint8_t[]) TmpBuffer);
	CHK_STATUS(status);
	Signature* firstHalf=TmpBuffer;
	if(firstHalf->valid==USED){
		dataStartPos=SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
		status+=restoreFileSystem((uint16_t)(firstHalf+1));

	}
	else{
		status+= flash_read(SECOND_HALF, sizeof(Signature), (uint8_t[]) TmpBuffer);
		CHK_STATUS(status);
		Signature* secondHalf=TmpBuffer;

		if(secondHalf->valid==USED){
			dataStartPos=SECOND_HALF+SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
			status+=restoreFileSystem((uint16_t)(secondHalf+1));
		}
		else{ //init first
			status+=flash_bulk_erase_start();
			uint8_t toWrite[2+(sizeof(FileHeader)/8)];
			Signature* firstHalf=(Signature*)toWrite;
			firstHalf->valid=USED;
			dataStartPos=SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
			FileHeader * currentHeader=(FileHeader *)(firstHalf+1);
			currentHeader->dataPointer=dataStartPos;
			currentHeader->valid=UNUSED;
			next_avilable_header_pos=sizeof(Signature);
			next_avilable_data_pos=currentHeader->dataPointer;
			headerStartPos=next_avilable_data_pos;
			headerFiles_num=0;
			status+=flash_write(FIRST_HALF, sizeof(Signature)+sizeof(uint16_t)+2,toWrite);
		}
	}
	CHK_STATUS(status);
	return SUCCESS;

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

