#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#include "tx_api.h"
#include "timer.h"
#include "string.h"

#define NUM_OF_CHARS_IN_KB (1024/2) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define MAX_FILES_SIZE (1010)
#define HALF_SIZE ((_flashSize_in_chars/2)*sizeof(char))
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_ON_DISK_SIZE (sizeof(FileHeaderOnDisk))
#define CHK_STATUS(n) if(n!=FS_SUCCESS)return n;
#define FLASH_CALL_BACK (0x01)
#define NO_HEADER (-1)
// wait for flash done call back- n is the returned flag
#define WAIT_FOR_FLASH_CB(n) {ULONG n;tx_event_flags_get(&fsFlag,FLASH_CALL_BACK,TX_OR_CLEAR,&n,TX_WAIT_FOREVER);};
#define READING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)/FILE_HEADRES_ON_DISK_SIZE) // number of FileHeaders that can be read from the flash in a single read command
//used for debug
extern data_length;
#pragma pack(1)
/**
 * FileHeader datatype used for every file
 */
typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	unsigned length:9;
	unsigned reserved:5;
	char name[8];
}FileHeaderOnDisk;

typedef struct{
	FileHeaderOnDisk onDisk;
	uint16_t adrress_of_header_on_flash;
	uint16_t data_start_pointer;
	uint16_t data_end_pointer;
}FileHeaderOnMemory;
/**
 * signature used for the beginning of every sector in memory
 */
typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	unsigned reserved:6;
}Signature;
#pragma pack()

TX_EVENT_FLAGS_GROUP fsFlag; // the fs flag

typedef enum {FIRST_HALF=0    ,SECOND_HALF=1} HALF;
FileHeaderOnDisk UNUSED_FILEHEADER_ON_DISK;
/**
 * global data types they are initialized at init and updated every write
 */
uint16_t _headerStartPos;
uint16_t _dataStartPos;
uint16_t _next_avilable_header_pos;
uint16_t _next_avilable_data_pos;
FileHeaderOnMemory _files[MAX_FILES_SIZE+10];
unsigned  _lastFile;
HALF _currentHalf;
unsigned _flashSize_in_chars;

/*
 * Wrapper write header to flash
 * to ensure no FileHeaderOnMemory write to disk
 */

result_t writeFileHeaderOnDiskToFlash(uint16_t address, FileHeaderOnDisk * file){
return flash_write(address, (uint16_t)FILE_HEADRES_ON_DISK_SIZE, (uint8_t*) file);
}
/*
 * Wrapper data header to flash
 * to ensure ....
 */
 result_t writeDataToFlash(uint16_t address,unsigned size,const char * data){
return flash_write(address, (uint16_t)size, (uint8_t*) data);

}

/**
 *    Must be called before any other operation on the file system.
 *  Arguments:
 *    settings - initialization information required to initialize the file system.
 **/

void fillArrayWith1ones(void * pointer,size_t numOfbytes){
	memset(pointer,0xff,numOfbytes);
}
/*
 * set header file on the flash to DELETED
 * headerPosOnDisk - pointer to the starting position on the flash
 */
FS_STATUS unactivateFileHeaderOnFlash(uint16_t headerPosOnDisk){
	FileHeaderOnDisk file;
	fillArrayWith1ones((void*)(&file),FILE_HEADRES_ON_DISK_SIZE);
	file.valid=DELETED;
	result_t stat=writeFileHeaderOnDiskToFlash(headerPosOnDisk, &file);
	if(stat!=OPERATION_SUCCESS) return FAILURE_ACCESSING_FLASH;
	return FS_SUCCESS;

}

FS_STATUS clearDuplicateFiles(){
	//TODO clear at least 2 duplicate files
	return FAILURE;
}


FS_STATUS addHeaderFileToMemory(FileHeaderOnDisk f){
	if(f.valid==UNUSED) { // the headerfile is unused and not empty - should be DELETED
		FS_STATUS stat=unactivateFileHeaderOnFlash(_next_avilable_header_pos);
		CHK_STATUS(stat);
	}
	else if(f.valid==USED){
		if(_lastFile>=MAX_FILES_SIZE+2) {
			FS_STATUS t=clearDuplicateFiles();
			if(t!=OPERATION_SUCCESS) return t;
		}
		_files[_lastFile].onDisk.valid=USED;
		memcpy(_files[_lastFile].onDisk.name,f.name,sizeof(f.name));
		_files[_lastFile].onDisk.length=f.length;
		_files[_lastFile].data_end_pointer=_next_avilable_data_pos;
		_files[_lastFile].data_start_pointer=(uint16_t)(_next_avilable_data_pos-f.length);
		_files[_lastFile].adrress_of_header_on_flash=_next_avilable_header_pos;
		_lastFile++;
	}
	else{
		// the headerfile is DELETED
	}
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;
	_next_avilable_data_pos-=f.length;
	return FS_SUCCESS;
}
/*
 * return 1 iff f is an empty(unused and unwritten ) FileHeaderOnDisk
 *
 */
int isUnusedHeader(FileHeaderOnDisk f){
	if (memcmp(&f,&UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE)==0) return 1;
	return 0;
}
/**
 * restoreFileSystem if system crashed with a valid file system restore it by reading all of the fs header
 * and indexing them also set global variables aboud data for eg num of files ect.
 * @param startAdress where the file sytem starts in the flash
 * @return success or failure reason
 */
result_t restoreFileSystem(HALF half){
	_currentHalf=half;
	_lastFile=0;
	if(half==FIRST_HALF){
		_headerStartPos=sizeof(Signature);
		_next_avilable_header_pos	=sizeof(Signature);
		_dataStartPos=(uint16_t)(HALF_SIZE-1);
		_next_avilable_data_pos=(uint16_t)(HALF_SIZE-1);

	}
	else{//half=SECOND_HALF
		_headerStartPos=(uint16_t)(HALF_SIZE+sizeof(Signature));
		_next_avilable_data_pos=(uint16_t)(HALF_SIZE+sizeof(Signature));
		_dataStartPos=(uint16_t)(_flashSize_in_chars*sizeof(char)-1);
		_next_avilable_data_pos=(uint16_t)(_flashSize_in_chars*sizeof(char)-1);

	}
	FileHeaderOnDisk f[READING_HEADRS_SIZE];
	result_t status=flash_read(_headerStartPos, FILE_HEADRES_ON_DISK_SIZE*READING_HEADRS_SIZE,(uint8_t[]) f);
	CHK_STATUS(status);
	int i=0;
	for(;!isUnusedHeader(f[i]);i++,_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE){
		if(i==READING_HEADRS_SIZE){ //need to read another headrsFiles
			status+=flash_read(_next_avilable_header_pos,FILE_HEADRES_ON_DISK_SIZE*READING_HEADRS_SIZE,
					(uint8_t[]) f);
			CHK_STATUS(status);
			i=0;
		}

		addHeaderFileToMemory(f[i]);

	}
	return status;
}

void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size){
}
/**
 *
 */
void fs_wakeup(){
	// set the event flag
	UINT status=tx_event_flags_set(&fsFlag,FLASH_CALL_BACK,TX_OR);
	if(status!=0){
		//TODO handle error
		data_length=(int)status;
	}
}

/**
 * Must be called before any other operation on the file system.
  Arguments:
    settings - initialization information required to initialize the file system.
 */
FS_STATUS fs_init(const FS_SETTINGS settings){
	if(settings.block_count!=16)return COMMAND_PARAMETERS_ERROR; //TODO
	//TODO to change when connecting to the UI and NETWORK

	fillArrayWith1ones((void*)&UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE); // setting a static file header that will be used at isEmptyFileHader(fh) method
	result_t status=flash_init(flash_data_recieve_cb,fs_wakeup);
	CHK_STATUS(status);
	status+=tx_event_flags_create(&fsFlag,"fsFlag");
	CHK_STATUS(status);
	_flashSize_in_chars=(settings.block_count)*NUM_OF_CHARS_IN_BLOCK;
	Signature TmpBuffer[2];
	status+= flash_read(0, sizeof(Signature), (uint8_t[]) TmpBuffer); //read first half Signature
	CHK_STATUS(status);
	Signature* firstHalf=TmpBuffer;
	if(firstHalf->valid==USED){
		status+=restoreFileSystem(FIRST_HALF);

	}
	else{
		status+= flash_read((uint16_t)HALF_SIZE, sizeof(Signature), ((uint8_t*) TmpBuffer));//read second half Signature
		CHK_STATUS(status);
		Signature* secondHalf=TmpBuffer;

		if(secondHalf->valid==USED){
			status+=restoreFileSystem(SECOND_HALF);
		}
		else{ //init first
			status+=flash_bulk_erase_start();
			CHK_STATUS(status);
			WAIT_FOR_FLASH_CB(actualFlag1);
			_currentHalf=FIRST_HALF;
			uint8_t toWrite[FILE_HEADRES_ON_DISK_SIZE+(sizeof(Signature))];
			fillArrayWith1ones(toWrite,FILE_HEADRES_ON_DISK_SIZE+(sizeof(Signature)));
			Signature* firstHalf=(Signature*)toWrite;
			firstHalf->valid=USED;
			_lastFile=0;
			_headerStartPos=sizeof(Signature);
			_next_avilable_header_pos=sizeof(Signature);
			_dataStartPos=(uint16_t)(HALF_SIZE-1);
			_next_avilable_data_pos=(uint16_t)(HALF_SIZE-1);
			void *t =(firstHalf+1);
			FileHeaderOnDisk * currentHeader=(FileHeaderOnDisk *)(t);
			currentHeader->valid=UNUSED;
			_lastFile=0;
			status+=writeDataToFlash(0, sizeof(Signature)+FILE_HEADRES_ON_DISK_SIZE,(char*)toWrite);
		}
	}
	CHK_STATUS(status);
	return FS_SUCCESS;

}
// goto Header and check file length (header[i].length-header[i+1].length)
FS_STATUS getLength(uint16_t headerNum ,uint16_t length){
	int stat=5;
	//    readHeader(headerNum,Header & data);
	//    length=data.legnth;
	return stat;
}
/*
 * remove file header from memory. and set it to DELETED on the flash
 * fileHeaderIndex - the index of the removed header file on the memory
 */
FS_STATUS removeFileHeader(int fileHeaderIndex){
	FS_STATUS status=unactivateFileHeaderOnFlash(_files[fileHeaderIndex].adrress_of_header_on_flash);
	CHK_STATUS(status);
	// remove from memory
	memmove((_files+fileHeaderIndex),(_files+fileHeaderIndex+1),
			(_lastFile-fileHeaderIndex)*sizeof(FileHeaderOnMemory));
	_lastFile--;
	return FS_SUCCESS;
}

// loop over headers and compare header.filename to filename return headerNum or fail
FS_STATUS FindFile(const char* filename, int *fileHeaderIndex){
	*fileHeaderIndex=NO_HEADER;
	for(int i=0;i<_lastFile;i++){
		if(_files[i].onDisk.valid==USED && strcmp(_files[i].onDisk.name,filename)==0){
			if(*fileHeaderIndex!=NO_HEADER) { //the same file appeared twice - the first one is irrelevant
				FS_STATUS status=removeFileHeader(*fileHeaderIndex);
				CHK_STATUS(status);
				i--;
			}
			*fileHeaderIndex=i;
		}
	}
	if(*fileHeaderIndex!=NO_HEADER) return FS_SUCCESS;
	return FILE_NOT_FOUND;
}
/*
 * Defragment the flash .
 * move it to the other Half
 */
FS_STATUS Defragment(){
	//TODO
	return FAILURE;
}
FS_STATUS writeNewDataToFlash(const char* filename, unsigned length,const char *data,int fileHeaderIndex){
	FileHeaderOnMemory * file;
	FS_STATUS stat=0;
	if(fileHeaderIndex!=NO_HEADER) file=&_files[fileHeaderIndex];
	else { // new filename
		if(_lastFile>=MAX_FILES_SIZE){
			stat=clearDuplicateFiles();
			CHK_STATUS(stat);
		}
		if(_lastFile>=MAX_FILES_SIZE) return MAXIMUM_FILE_SIZE_EXCEEDED;
		file=&_files[_lastFile++];
	}


	if(_next_avilable_header_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE) {
		// no place for the new/changed file
		if(fileHeaderIndex==NO_HEADER)_lastFile--;

		stat=Defragment();
		CHK_STATUS(stat);

		if(_next_avilable_header_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE)
			//still no place
			return MAXIMUM_FLASH_SIZE_EXCEEDED;

		return fs_write(filename, length, data); // try to write again
	}

	// write length and name to flash
	file->adrress_of_header_on_flash=_next_avilable_header_pos;
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;

	fillArrayWith1ones((void*)file,FILE_HEADRES_ON_DISK_SIZE);
	strcpy(file->onDisk.name,filename);
	file->onDisk.length=length;
	stat+=writeFileHeaderOnDiskToFlash(file->adrress_of_header_on_flash,&(file->onDisk));
	CHK_STATUS(stat);

	// write data to flash
	file->data_end_pointer=_next_avilable_data_pos;
	_next_avilable_data_pos-=length;
	file->data_start_pointer=(uint16_t)(_next_avilable_data_pos+1);
	stat+=writeDataToFlash(file->data_start_pointer,length,data);
		CHK_STATUS(stat);



	// set the header on the flash to USED
	FileHeaderOnDisk tmp;
	fillArrayWith1ones((void*)&tmp,FILE_HEADRES_ON_DISK_SIZE);
	tmp.valid=USED;
	stat+=writeFileHeaderOnDiskToFlash(file->adrress_of_header_on_flash,&(tmp));
	CHK_STATUS(stat);
	file->onDisk.valid=USED;
	return FS_SUCCESS;

}
/*

  Description:
    Write a file.

  Arguments:
    filename - the name of the file.
    length - the size of the 'data' input buffer.
    data - a buffer holding the file content.

 */
FS_STATUS fs_write(const char* filename, unsigned length, const char* data){
	if(length>=0.5*KB) return MAXIMUM_FLASH_SIZE_EXCEEDED;
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat!=FILE_NOT_FOUND){ // Existing file
		uint16_t headerOldPositon=_files[fileHeaderIndex].adrress_of_header_on_flash;
		stat=    writeNewDataToFlash(filename,length,data,fileHeaderIndex);
		CHK_STATUS(stat);
		return	stat=unactivateFileHeaderOnFlash(headerOldPositon);
	}
	else return   writeNewDataToFlash(filename,length,data,NO_HEADER);
}

FS_STATUS fs_filesize(const char* filename, unsigned* length){
	/*int headerLoc=0;
    int stat=FindFile(filename,headerLoc);
    if (stat!=FILE_NOT_FOUND){
        return getLength(headerLoc,length);
    }void tx_application_define(void *first_unused_memory) {
    status=intHARDWARE();


    //GUI_thread
    status+=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&guistack, STACK_SIZE,    16, 16, 4, TX_AUTO_START);

    //NetworkThread
    status+=tx_thread_create(&receiveThread, "NetworkReceiveThread", sendReceiveLoop, inputText,&receiveThreadStack, STACK_SIZE,    16, 16, 4, TX_AUTO_START);

    //create recive and send queue
    status=tx_queue_create(&receiveQueue, "receiveQueue", TX_1_ULONG, &receiveQueueStack, QUEUE_SIZE*sizeof(ULONG));
    status=tx_queue_create(&ToSendQueue, "ToSendQueue", TX_1_ULONG, &sendQueueStack, QUEUE_SIZE*sizeof(ULONG));
}

    else*/ return FILE_NOT_FOUND;
}
FS_STATUS fs_erase(const char* filename){
	/*int headerLoc=0;
    int stat=FindFile(filename,headerLoc);
    if (stat!=FILE_NOT_FOUND){
        return unactivateFile(headerLoc);
    }
    else */return FILE_NOT_FOUND;
}
FS_STATUS fs_count(unsigned* file_count){
	return FILE_NOT_FOUND;

}
