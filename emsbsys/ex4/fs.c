#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#include "tx_api.h"
#include "timer.h"

#define BLOCK_SIZE (4*KB)
#define NUM_OF_CHARS_IN_KB (1024) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define MAX_FILES_SIZE (1000)
#define HALF_SIZE ((_half_flashSize))
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_ON_DISK_SIZE (sizeof(FileHeaderOnDisk))
#define CHK_RESUALT_T_STATUS(n) if(n!=OPERATION_SUCCESS){ return convert_FlashError_to_FSError(n);};
#define CHK_FS_STATUS(n) if(n!=FS_SUCCESS) {return n;};
#define TRUE (1)
#define FALSE (0)
#define MAXIMUM_FILE_SIZE_LIMIT (0.5*KB)
#define ENABLE_AND_RETURN_IF_NOT_READY {if(_is_ready!=TRUE){_enable();return FS_NOT_READY;}};
#define VALIDATE_HALF(n) changeHalfStatus(n,USED)
#define UNVALIDATE_HALF(n) changeHalfStatus(n,DELETED)

#define FLASH_CALL_BACK (0x01)
#define NO_HEADER (-1)
// wait for flash done call back- n is the returned flag
#define WAIT_FOR_FLASH_CB(n) {ULONG n;tx_event_flags_get(&fsFlag,FLASH_CALL_BACK,TX_OR_CLEAR,&n,TX_WAIT_FOREVER);};
#define READING_WRITING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)/FILE_HEADRES_ON_DISK_SIZE) // number of FileHeaders that can be read/write from the flash in a single read/write command
#define READING_WRITING_DATA_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)) // number of chars that can be read/write from the flash in a single read/write command

//#include <assert.h>
//#define DEBUG_MODE 1 //TODO set it to 0

#define CHK_TMP_FILE_EMPTY {assert(isusedOrCrashedOrDeletedHeader(TMP_FileForWrtitingToFlash)==FS_SUCCESS);}
typedef enum {FIRST_HALF=0    ,SECOND_HALF=1} HALF;

//used for debug
extern data_length;
//-------------structs-------------//
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
	//	uint16_t data_end_pointer;
}FileHeaderOnMemory;
/**
* signature used for the beginning of every sector in memory
*/
typedef struct{
	unsigned valid:2; // USED / DELETED / UNUSED
	unsigned reserved:6;
}Signature;
#pragma pack()
//-------------structs-------------//

TX_EVENT_FLAGS_GROUP fsFlag; // the fs flag

FileHeaderOnDisk FINAL_UNUSED_FILEHEADER_ON_DISK; // example of unused fileHeader on the flash
/**
* global data types they are initialized at init and updated every write
*/
//uint16_t _headerStartPos;
//uint16_t _dataStartPos;
uint16_t _next_avilable_header_pos; // the empty place on the flash start here
uint16_t _end_of_avilable_data_pos; // from this place the flash is used (data is written)
unsigned _block_count;

FileHeaderOnMemory _files[MAX_FILES_SIZE+10]; //the files headers
unsigned  _lastFile;
HALF _currentHalf;
unsigned _half_flashSize;
volatile int _is_ready=FALSE;


FS_STATUS convert_FlashError_to_FSError(result_t er){
	switch (er){
	case OPERATION_SUCCESS: return FS_SUCCESS;
	case NOT_READY: return FAILURE_ACCESSING_FLASH;
	case NULL_POINTER: return COMMAND_PARAMETERS_ERROR;
	case INVALID_ARGUMENTS: return COMMAND_PARAMETERS_ERROR;
	default: return FAILURE;
	//TODO  return 	FS_NOT_READY
	}
}
FileHeaderOnDisk TMP_headerFileForWrtitingToFlash; //used for writing file headers to flash


/**
*    clean the array. (fill it with 1).
**/

void fillArrayWith1ones(void * pointer,size_t size){
	memset(pointer,0xFF,size);
}

/**
* Wrapper start write to flash - none blocking
* sleep until the writing done
* to ensure ....
*/
result_t writeDataToFlash(uint16_t address,uint16_t size, const uint8_t * data){
	if(!flash_is_ready()) return NOT_READY;
	result_t res= flash_write_start(address, size,  data);
	WAIT_FOR_FLASH_CB(wait_fro_writng_to_flash_done);
	return res;
}
/**
//TODO change to none blocking method
* Wrapper start read to flash -  blocking
* to ensure ....
*/
result_t readDataFromFlash(uint16_t start_address,uint16_t len,  uint8_t * data){
	if(!flash_is_ready()) return NOT_READY;
	//TODO change to none blocking method
	result_t res= flash_read(start_address,len,data);
	//	WAIT_FOR_FLASH_CB(wait_fro_writng_to_flash_done);
	return res;
}


/**
* Wrapper write header to flash v- none blocking
* sleep until the writing done
* to ensure no FileHeaderOnMemory write to disk
* WARN: empty the TMP_FileForWrtitingToFlash (fill it with 1's)
*
*/
result_t writeTMP_FileToFlash(uint16_t address){
	result_t res= writeDataToFlash(address, (uint16_t)FILE_HEADRES_ON_DISK_SIZE, (uint8_t*) &TMP_headerFileForWrtitingToFlash);
	fillArrayWith1ones(&TMP_headerFileForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	return res;
}

/**
* set header file on the flash to DELETED
* headerPosOnDisk - pointer to the starting position on the flash
*/
FS_STATUS unactivateFileHeaderOnFlash(uint16_t headerPosOnDisk){
	//	if(DEBUG_MODE) {CHK_TMP_FILE_EMPTY;}


	TMP_headerFileForWrtitingToFlash.valid=DELETED;
	if(writeTMP_FileToFlash(headerPosOnDisk)!=OPERATION_SUCCESS) {
		//		fillArrayWith1ones(&TMP_FileForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
		return FAILURE_ACCESSING_FLASH;
	}
	return FS_SUCCESS;

}

/**
* remove file header from memory. and set it to DELETED on the flash
* fileHeaderIndex - the index of the removed header file on the memory
*/
volatile int ready;
FS_STATUS removeFileHeader(int fileHeaderIndex){
//	_disable();
	ready=_is_ready;
	_is_ready=FALSE;
//	_enable();

	//remove from flash
	FS_STATUS status=unactivateFileHeaderOnFlash(_files[fileHeaderIndex].adrress_of_header_on_flash);
	if(status==FS_SUCCESS){
		// remove from memory
		memmove((_files+fileHeaderIndex),(_files+fileHeaderIndex+1),
				(_lastFile-fileHeaderIndex)*sizeof(FileHeaderOnMemory));
		_lastFile--;
	}
	_is_ready=ready;
	return status;
}

/**
* clear 1-2 duplicate files
*/
FS_STATUS clearDuplicateFiles(){
	FS_STATUS stat=FS_SUCCESS;
	int numOfDuplicateFilesDeleted=0;
	for(int i=0;i<_lastFile &&numOfDuplicateFilesDeleted<2 ;i++){
		for(int j=i+1;j<_lastFile&&numOfDuplicateFilesDeleted<2;j++){
			if(strcmp(_files[i].onDisk.name,_files[j].onDisk.name)==0){
				//remove file i
				stat+= removeFileHeader(i);
				CHK_FS_STATUS(stat);
				numOfDuplicateFilesDeleted++;
				i=i-1;
				break;
			}
		}
	}
	return stat;
}


/**
* add a FileHeaderOnDisk to the memory.
* assume the FileHeaderOnDisk isn't empty isusedOrCrashedOrDeletedHeader( f)!=0
* if the FileHeaderOnDisk has a failure deleted it.
*
*/

FS_STATUS addHeaderFileToMemory(FileHeaderOnDisk f){
	FS_STATUS stat;
	if(f.valid==UNUSED) {
		// failure - the headerfile is unused and not empty - should be DELETED
		stat=unactivateFileHeaderOnFlash(_next_avilable_header_pos);
		CHK_FS_STATUS(stat);
	}
	else{
		if(f.valid==USED){
			if(_lastFile>=MAX_FILES_SIZE+2) {
				stat=clearDuplicateFiles();
				CHK_FS_STATUS(stat);
			}
			_files[_lastFile].onDisk.valid=USED;
			memcpy(_files[_lastFile].onDisk.name,f.name,sizeof(f.name));
			_files[_lastFile].onDisk.length=f.length;
			//		_files[_lastFile].data_end_pointer=_next_avilable_data_pos;
			_files[_lastFile].data_start_pointer=(_end_of_avilable_data_pos-f.length);
			_files[_lastFile].adrress_of_header_on_flash=_next_avilable_header_pos;
			_lastFile++;
		}
		else{// the headerfile is DELETED
			stat=FS_SUCCESS;
		}
	}
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;
	_end_of_avilable_data_pos-=f.length;
	return stat;
}
/**
* return 0 iff f is an empty(unused and unwritten ) FileHeaderOnDisk
* meaning the FileHeaderOnDisk has only one's
*/
int isNotEmptyFileheadr(FileHeaderOnDisk f){
	return memcmp(&f,&FINAL_UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE);
}
/**
* set the pointers of the half
*/
void setHalfPointers(HALF half,uint16_t* next_avilable_header_pos,uint16_t *end_of_avilable_data_pos ){
	*next_avilable_header_pos	=sizeof(Signature);
	*end_of_avilable_data_pos=(HALF_SIZE);

	if(half==SECOND_HALF){
		*next_avilable_header_pos+=HALF_SIZE;
		*end_of_avilable_data_pos+=HALF_SIZE;

	}
}
void setHalf(HALF half){
	setHalfPointers(half,&_next_avilable_header_pos,&_end_of_avilable_data_pos);
}
/**
* restoreFileSystem if system crashed with a valid file system restore it by reading all of the fs header
* and indexing them also set global variables aboud data for num of files etc.
* @param startAdress where the file sytem starts in the flash
* @return success or failure reason
*/

FileHeaderOnDisk TMP_readedWritedFilesHeaders[READING_WRITING_HEADRS_SIZE]; //used for reading/writing from/to the flash
FS_STATUS restoreFileSystem(HALF half){
	_currentHalf=half;
	_lastFile=0;
	setHalf(_currentHalf);
	memset(TMP_readedWritedFilesHeaders,0,sizeof(TMP_readedWritedFilesHeaders));
	result_t status=flash_read(_next_avilable_header_pos, FILE_HEADRES_ON_DISK_SIZE*READING_WRITING_HEADRS_SIZE,(uint8_t[]) TMP_readedWritedFilesHeaders);
	CHK_RESUALT_T_STATUS(status);
	for(int i=0;isNotEmptyFileheadr(TMP_readedWritedFilesHeaders[i]);i++){
		if(i==READING_WRITING_HEADRS_SIZE){ //need to read another headrsFiles
			memset(TMP_readedWritedFilesHeaders,0,sizeof(TMP_readedWritedFilesHeaders));
			status+=flash_read(_next_avilable_header_pos,FILE_HEADRES_ON_DISK_SIZE*READING_WRITING_HEADRS_SIZE,(uint8_t[]) TMP_readedWritedFilesHeaders);
			CHK_RESUALT_T_STATUS(status);
			i=0;
		}

		addHeaderFileToMemory(TMP_readedWritedFilesHeaders[i]);

	}
	return FS_SUCCESS;
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
void flash_read_done_cb(uint8_t const *buffer, uint32_t size){
	// TODO
	//TODO HANDLE ERROR
	fs_wakeup();
}

/**
*  change the half status to stat (USED/DELETED)
*/
Signature TMP_Signature;
FS_STATUS changeHalfStatus(HALF half,unsigned stat){
	TMP_Signature.valid=stat;
	result_t status= flash_write((uint16_t)(half*HALF_SIZE), (uint16_t)sizeof(Signature),(uint8_t*)&TMP_Signature);
	CHK_RESUALT_T_STATUS(status);
	return status;

}


/**
* Must be called before any other operation on the file system.
Arguments:
settings - initialization information required to initialize the file system.
*/
FS_STATUS fs_init(const FS_SETTINGS settings){
	//	if(settings.block_count!=16)return COMMAND_PARAMETERS_ERROR; //TODO
	//TODO to change when connecting to the UI and NETWORK
	_block_count=settings.block_count*2; // *2 for using the Log-based file system

	fillArrayWith1ones((void*)(&TMP_headerFileForWrtitingToFlash),FILE_HEADRES_ON_DISK_SIZE);
	fillArrayWith1ones((void*)&FINAL_UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE); // setting a static file header that will be used at isEmptyFileHader(fh) method
	result_t status=flash_init(flash_read_done_cb,fs_wakeup);
	CHK_RESUALT_T_STATUS(status);
	status+=tx_event_flags_create(&fsFlag,"fsFlag");
	CHK_RESUALT_T_STATUS(status);
	_half_flashSize=settings.block_count*NUM_OF_CHARS_IN_BLOCK;

	//read the first half Signature
	status+= flash_read(0, sizeof(Signature),  (uint8_t*)&TMP_Signature); //read first half Signature
	CHK_RESUALT_T_STATUS(status);
	if(TMP_Signature.valid==USED){
		status+=restoreFileSystem(FIRST_HALF);

	}
	else{
		// read the second half Signature
		status+= flash_read((uint16_t)HALF_SIZE, sizeof(Signature), ((uint8_t*) &TMP_Signature));//read second half Signature
		CHK_RESUALT_T_STATUS(status);
		if(TMP_Signature.valid==USED){
			status+=restoreFileSystem(SECOND_HALF);
		}
		else{
			//empty flash
			status+=flash_bulk_erase_start();
			CHK_RESUALT_T_STATUS(status);
			WAIT_FOR_FLASH_CB(erase_flash);
			_currentHalf=FIRST_HALF;
			status+=VALIDATE_HALF(FIRST_HALF);
			CHK_RESUALT_T_STATUS(status);
			setHalf(_currentHalf);
			_lastFile=0;
			_files[0].onDisk.valid=UNUSED;
			_files[0].onDisk.length=0;
		}
	}
	_is_ready=TRUE;
	return status;

}


/**
*  loop over headers and compare header.filename to filename
*  if find more then 1 file - clean them
*  set the fileHeaderIndex to the file header index
* return FILE_NOT_FOUND if not found
*/
FS_STATUS FindFile(const char* filename, int *fileHeaderIndex){
	*fileHeaderIndex=NO_HEADER;
	// the bigest index is the relavent file
	for(int i=_lastFile;i>=0;i--){
		if(_files[i].onDisk.valid==USED && strcmp(_files[i].onDisk.name,filename)==0){
			if(*fileHeaderIndex==NO_HEADER) *fileHeaderIndex=i;
			else {
				//the same file appeared twice - the lower index is irrelevant, delete it
				FS_STATUS status=removeFileHeader(i);
				CHK_FS_STATUS(status);
				i--;
			}
		}
	}
	if(*fileHeaderIndex!=NO_HEADER) return FS_SUCCESS;
	else return FILE_NOT_FOUND;
}

/**
* erase half of the flash
*/
FS_STATUS eraseHalf(HALF half){
	if(!flash_is_ready()) return FS_NOT_READY;
	result_t stat;
	unsigned startAtblock=0;
	if(half==SECOND_HALF){
		startAtblock=_block_count/2;
	}
	for(int i=0;i<_block_count/2;i++){
		stat=flash_block_erase_start((uint16_t)((startAtblock+i)*BLOCK_SIZE));
		WAIT_FOR_FLASH_CB(wait_for_flash_erase_block);
		CHK_RESUALT_T_STATUS(stat);
	}
	return stat;
}

/**
*
* change the data of an existing file or write a new file to the file system.
* write it to the flash
* if fileHeaderIndex==NO_HEADER this is a new file.
* delete the previous header file on the disk;
*/
FS_STATUS writeFileDataToFlash(const char* filename, uint16_t length,const uint8_t *data,int fileHeaderIndex){
	FileHeaderOnMemory * file=NULL;
	FS_STATUS stat=FS_SUCCESS;
	uint16_t previousAddressHeaderOnFlash=0;
	if(fileHeaderIndex!=NO_HEADER) {
		// existing file
		file=&_files[fileHeaderIndex];
		previousAddressHeaderOnFlash=file->adrress_of_header_on_flash;
	}
	else {
		// new file

		//chk for files size limit
		if(_lastFile>MAX_FILES_SIZE){
			stat+=clearDuplicateFiles();
			CHK_FS_STATUS(stat);
			if(_lastFile>MAX_FILES_SIZE){
				//still no place
				return MAXIMUM_FILE_SIZE_EXCEEDED;
			}
		}

		// create new FileHeaderOnMemory
		else {
			file=&_files[_lastFile];
			strcpy(file->onDisk.name,filename);
		}
	}

	//chk for flash size limit
	if(_end_of_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE) {
		// no place for the new/changed file
		stat+=changehalf();
		CHK_FS_STATUS(stat);
		if(_end_of_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE){
			//still no place
			return MAXIMUM_FLASH_SIZE_EXCEEDED;
		}
		return writeFileDataToFlash(filename, length, data,fileHeaderIndex); // try to write again
	}



	// write data to flash
	_end_of_avilable_data_pos-=length;
	stat+=writeDataToFlash(_end_of_avilable_data_pos,length,data);
	CHK_FS_STATUS(stat);
	file->data_start_pointer=_end_of_avilable_data_pos;

	// write length and name to flash
	//	fillArrayWith1ones((void*)&TMP_FileForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	strcpy(TMP_headerFileForWrtitingToFlash.name,filename);
	TMP_headerFileForWrtitingToFlash.length=length;
	result_t stat1=writeTMP_FileToFlash(_next_avilable_header_pos);
	CHK_RESUALT_T_STATUS(stat1);
	file->onDisk.length=length;
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;


	// set the header on the flash to USED
	//	fillArrayWith1ones((void*)&TMP_FileForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	TMP_headerFileForWrtitingToFlash.valid=USED;
	stat+=writeTMP_FileToFlash((uint16_t)(_next_avilable_header_pos-FILE_HEADRES_ON_DISK_SIZE));
	CHK_RESUALT_T_STATUS(stat1);
	file->adrress_of_header_on_flash=(uint16_t)(_next_avilable_header_pos-FILE_HEADRES_ON_DISK_SIZE);
	file->onDisk.valid=USED;
	if(fileHeaderIndex==NO_HEADER){
		_lastFile++;
		_files[_lastFile].onDisk.valid=UNUSED;
	}
	else { // delete the previous header file on the disk
		stat=unactivateFileHeaderOnFlash(previousAddressHeaderOnFlash);
	}
	return stat;
}

/**

Description:
Write a file.

Arguments:
filename - the name of the file.
length - the size of the 'data' input buffer.
data - a buffer holding the file content.

*/
FS_STATUS fs_write(const char* filename, unsigned length, const char* data){
//	_disable();
//	ENABLE_AND_RETURN_IF_NOT_READY;

	_is_ready=FALSE;
//	_enable();

	if(length>MAXIMUM_FILE_SIZE_LIMIT) {
		_is_ready=TRUE;
		return MAXIMUM_FILE_SIZE_EXCEEDED;
	}

	int fileHeaderIndex=NO_HEADER;
	FS_STATUS stat=FindFile(filename,&fileHeaderIndex);
	if(stat==FS_SUCCESS || stat==FILE_NOT_FOUND) {
		stat=writeFileDataToFlash(filename,(uint16_t)length,(const uint8_t*)data,fileHeaderIndex);
	}

	_is_ready=TRUE;
	return stat;
}
/**

Description:
Erase a file.
Arguments:
filename - the name of the file.

*/
FS_STATUS fs_erase(const char* filename){
//	_disable();
//	ENABLE_AND_RETURN_IF_NOT_READY;

	_is_ready=FALSE;
//	_enable();

	int fileHeaderIndex=NO_HEADER;
	FS_STATUS stat=FindFile(filename,&fileHeaderIndex);
	if(stat==FS_SUCCESS && fileHeaderIndex!=NO_HEADER){
		stat= removeFileHeader(fileHeaderIndex);
	}
	_is_ready=TRUE;
	return stat;
}

/**

Description:
Return the size of a file.
File size is defined as the length of the file content only.

Arguments:
filename - the name of the file.
length - an out parameter to hold the file size.

*/
FS_STATUS fs_filesize(const char* filename, unsigned* length){
	if(_is_ready!=TRUE) return FS_NOT_READY;
	int fileHeaderIndex=NO_HEADER;
	FS_STATUS stat=FindFile(filename,&fileHeaderIndex);
	//	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_FS_STATUS(stat);
	*length=_files[fileHeaderIndex].onDisk.length;
	return FS_SUCCESS;
}
/**

Description:
Return the file count in the file system
Arguments:
file_count - an out argument to hold the number of files in the file system.

*/
FS_STATUS fs_count(unsigned* file_count){
	if(_is_ready!=TRUE) return FS_NOT_READY;
	*file_count=0;
	for(int i=0;i<_lastFile;i++){
		if(_files[i].onDisk.valid==USED){
			(*file_count)=(*file_count)+1;
		} else {
			FS_STATUS stat=removeFileHeader(i);
			CHK_FS_STATUS(stat);
			i--;
		}
	}
	return FS_SUCCESS;
}

/**
* read the data from of fileHeaderIndex
*/
FS_STATUS readDataFromHeaderIndex(int fileHeaderIndex, unsigned* length, char* data){
	memset(data,0,*length);
	result_t flashStat=readDataFromFlash(_files[fileHeaderIndex].data_start_pointer,(uint16_t)_files[fileHeaderIndex].onDisk.length, (uint8_t*)data);
	CHK_RESUALT_T_STATUS(flashStat);
	*length=_files[fileHeaderIndex].onDisk.length;
	return FS_SUCCESS;
}

/**

Description:
Read the content of a file.

Arguments:
filename - the name of the file.
length - when calling the function this argument should hold the size of the 'data' input buffer.
when the function return this argument will hold the file size, i.e. the actual used space size of the 'data' buffer.
data - a pointer for a buffer to hold the file content.


*/
FS_STATUS fs_read(const char* filename, unsigned* length, char* data){
	if(_is_ready!=TRUE) return FS_NOT_READY;
	int fileHeaderIndex=NO_HEADER;
	FS_STATUS stat=FindFile(filename,&fileHeaderIndex);
	//	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_FS_STATUS(stat);
	stat=readDataFromHeaderIndex(fileHeaderIndex,length,data);
	return stat;
}
/**

Description:
List all the files exist in the file system.
Arguments:
length - when calling the function this argument should hold the size of the "files" buffer.
when the function return this argument will hold the the actual used space size of the 'files' buffer, including the last null byte.
files - a series of continuous null terminated strings, each representing a file name in the file system

*/
FS_STATUS fs_list(unsigned* length, char* files){
	if(_is_ready!=TRUE) return FS_NOT_READY;
	memset(files,0,*length);
	unsigned usedLen=0;
	for(int i=0;i<_lastFile;i++){
		if(_files[i].onDisk.valid==USED) {
			unsigned namelen=strlen(_files[i].onDisk.name)+1;
			if(usedLen+namelen>(*length)) return COMMAND_PARAMETERS_ERROR ; // not sufficient place at files buffer
			strcpy(&(files[usedLen]),_files[i].onDisk.name);
			usedLen+=namelen;
		}
		else {
			FS_STATUS stat=removeFileHeader(i);
			CHK_FS_STATUS(stat);
			i--;
		}
	}
	*length=usedLen;
	return FS_SUCCESS;
}
/*
 * write dataBuffer to flash at nextHalf_end_of_avilable_data_pos
 * move nextHalf_end_of_avilable_data_pos according to it and zeroed dataBuffer_usedSize
 */
char TMP_dataBufferToWrite[(KB/2)]; // used for writing data to flash
FS_STATUS write_dataBuffer_toFlash(uint16_t * nextHalf_end_of_avilable_data_pos,uint16_t *dataBuffer_usedSize){
	uint16_t address=(uint16_t)(*nextHalf_end_of_avilable_data_pos-*dataBuffer_usedSize);
	const uint8_t * data=	(uint8_t*)(TMP_dataBufferToWrite+(READING_WRITING_DATA_SIZE-*dataBuffer_usedSize));
	FS_STATUS stat=writeDataToFlash(address,*dataBuffer_usedSize,data);
	CHK_FS_STATUS(stat);
	*nextHalf_end_of_avilable_data_pos-=*dataBuffer_usedSize;
	*dataBuffer_usedSize=0;
	return stat;
}
/**
*  copy the files header and data from the current half to next half
*/
unsigned tmp;
FS_STATUS copyFilesAndDataToNextHalf(uint16_t* nextHalf_next_avilable_header_pos,uint16_t* nextHalf_end_of_avilable_data_pos){

	FS_STATUS stat=FS_SUCCESS;
	fillArrayWith1ones((void*)TMP_dataBufferToWrite,sizeof(TMP_dataBufferToWrite));
	fillArrayWith1ones(TMP_readedWritedFilesHeaders,sizeof(TMP_readedWritedFilesHeaders));
	uint16_t dataBuffer_usedSize=0;
	int readedWritedFilesHeaders_usedIndex=0;

	for(int i=0;i<_lastFile;i++){
		//write the data:
		//chk if need to write to flash the data buffer
		if(((unsigned)dataBuffer_usedSize)+_files[i].onDisk.length>READING_WRITING_DATA_SIZE){
			// there is no place at the data buffer:
			// write dataBuffer to flash at nextHalf_end_of_avilable_data_pos
			stat+=write_dataBuffer_toFlash(nextHalf_end_of_avilable_data_pos,&dataBuffer_usedSize);
			CHK_FS_STATUS(stat);
		}
		//read the data to data buffer:
		char * fileData=TMP_dataBufferToWrite+((READING_WRITING_DATA_SIZE-dataBuffer_usedSize)
				-_files[i].onDisk.length);
		stat+=readDataFromHeaderIndex(i,&tmp,fileData);
		CHK_FS_STATUS(stat);
		dataBuffer_usedSize+=_files[i].onDisk.length;

		//chk if need to write to flash the fileHeadrs buffer
		if(readedWritedFilesHeaders_usedIndex>=READING_WRITING_HEADRS_SIZE){
			stat+=writeDataToFlash(*nextHalf_next_avilable_header_pos,
					(uint16_t)(SIZE_OF_FILEHEADRS_IN_CHARS*readedWritedFilesHeaders_usedIndex),
					(uint8_t*)TMP_readedWritedFilesHeaders);
			CHK_FS_STATUS(stat);
			*nextHalf_next_avilable_header_pos+=SIZE_OF_FILEHEADRS_IN_CHARS*readedWritedFilesHeaders_usedIndex;
			readedWritedFilesHeaders_usedIndex=0;
		}

		// write the FileHeader to FileHeader buffer
		strcpy(TMP_readedWritedFilesHeaders[readedWritedFilesHeaders_usedIndex].name,_files[i].onDisk.name);
		TMP_readedWritedFilesHeaders[readedWritedFilesHeaders_usedIndex].length=_files[i].onDisk.length;
		TMP_readedWritedFilesHeaders[readedWritedFilesHeaders_usedIndex].valid=_files[i].onDisk.valid;
	}

	// write the buffers if needed:
	if(dataBuffer_usedSize>0){
		// write dataBuffer to flash at nextHalf_end_of_avilable_data_pos
		write_dataBuffer_toFlash(nextHalf_end_of_avilable_data_pos,&dataBuffer_usedSize);
	}
	if(readedWritedFilesHeaders_usedIndex>0){
		stat+=writeDataToFlash(*nextHalf_next_avilable_header_pos,
				(uint16_t)(SIZE_OF_FILEHEADRS_IN_CHARS*readedWritedFilesHeaders_usedIndex),
				(uint8_t*)TMP_readedWritedFilesHeaders);
		CHK_FS_STATUS(stat);
		nextHalf_next_avilable_header_pos+=SIZE_OF_FILEHEADRS_IN_CHARS*readedWritedFilesHeaders_usedIndex;
	}

	return stat;
}
/*
 * change the half.
 * move it to the other Half
 */
FS_STATUS changehalf(){
	//	;*******************************************
	//	writeDataToFlash
	//	copyFilesAndDataTo
	//	changehalf
	//	***********************
	FS_STATUS stat;
	HALF nextHalf;
	if(_currentHalf==FIRST_HALF)nextHalf=SECOND_HALF;
	else nextHalf=FIRST_HALF;
	uint16_t nextHalf_next_avilable_header_pos; // the empty place on the flash start here
	uint16_t nextHalf_end_of_avilable_data_pos; // from this place the flash is used (data is written)
	setHalfPointers(nextHalf,&nextHalf_next_avilable_header_pos,&nextHalf_end_of_avilable_data_pos);

	stat=eraseHalf(nextHalf);
	CHK_FS_STATUS(stat);

	stat+=copyFilesAndDataToNextHalf(&nextHalf_next_avilable_header_pos,&nextHalf_end_of_avilable_data_pos);
	CHK_FS_STATUS(stat);

	stat+=VALIDATE_HALF(nextHalf);
	CHK_FS_STATUS(stat);

	stat+=UNVALIDATE_HALF(_currentHalf);
	CHK_FS_STATUS(stat);

	//TODO no need only for debugging: {
	stat=eraseHalf(_currentHalf);
	CHK_FS_STATUS(stat);
	//TODO end }
	//

	stat+=restoreFileSystem(nextHalf);
	CHK_FS_STATUS(stat);

	setHalf(nextHalf);
	return  stat;
}


/**
==========================================================================
Usage Sample
(a naive fs usage, with all buffers declared with max expected size)
==========================================================================
*/
#define MAX_FILES_COUNT (10)
#define MAX_FILE_SIZE (500)
char files[MAX_FILES_COUNT*MAX_FILE_SIZE];
FS_STATUS schoolTest(){

	FS_SETTINGS settings;
	const char* file1data = "somthing else1234564878564564564564sajfldhgklsdhgklshfklghsklghklshgklshgklsdhklghsdklghdklhgkls64sajfldhgklsdhgklshfklghsklghklshgklshgklsdhklghsdklghdklhgklsdhgklshl56456456dhgklshl56456456456464564564564564564564565464";
	const char* file2data = "b456456456456465456y4645646545645645645645645664sajfldhgklsdhgklshfklgh64sajfldhgklsdhgklshfklghsklghklshgklshgklsdhklghsdk64sajfldhgklsdhgklshfklghsklghklshgklshgklsdhklghsdklghdklhgklsdhgklshl56456456lghdklhgklsdhgklshl56456456sklghklshgklshgklsdhklghsdklghdklhgklsdhgklshl56456456456456456456e";
	char data[MAX_FILE_SIZE];
	unsigned length=MAX_FILES_COUNT*MAX_FILE_SIZE;
	unsigned count=-1;
	char *fileName;

	settings.block_count = 16;
	FS_STATUS stat=0;

	stat+=fs_list(&length, files);
	if (FS_SUCCESS != stat){
		return FS_NOT_READY;
	}
	stat+=stat;
	//	char * pointerTodata;
	//	pointerTodata=data;
	stat+=fs_count(&count);
	if (FS_SUCCESS != stat){
		return count;
	}
	for( fileName=files ; count>0 ; count-- ) {
		//
		if(fileName>=(length+files)){
			return -1;
		}
		unsigned length = sizeof(data);
		stat = fs_read(fileName, &length, data);
		if (stat!=FS_SUCCESS){

			return stat;
		}
		fileName+=strlen(fileName)+1;

	}

	stat=fs_write("file0", strlen(file1data), file1data);

	if ( stat!=FS_SUCCESS){
		return FS_NOT_READY;
	}
	stat+=stat;
	for(int kl=0;kl<15;kl++){
		stat+=fs_write("file2", strlen(file2data), file2data);
		if (FS_SUCCESS != stat){
			return FS_NOT_READY;
		}
	}
	stat+=stat;
	stat+=fs_list(&length, files);
	if (FS_SUCCESS != stat){
		return FS_NOT_READY;
	}
	stat+=stat;
	//	char * pointerTodata;
	//	pointerTodata=data;
	stat+=fs_count(&count);
	if (FS_SUCCESS != stat){
		return count;
	}
	for( fileName=files ; count>0 ; count-- ) {
		//
		if(fileName>=(length+files)){
			return -1;
		}
		unsigned length = sizeof(data);
		stat = fs_read(fileName, &length, data);
		if (stat!=FS_SUCCESS){

			return stat;
		}
		fileName+=strlen(fileName)+1;

	}
	return stat;

}
