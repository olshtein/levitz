#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#include "tx_api.h"
#include "timer.h"
#include "string.h"
//#include "stdio.h"

#define BLOCK_SIZE (4*KB)
#define NUM_OF_CHARS_IN_KB (1024/sizeof(char)) // num of chars in KB
#define USED (0x1)
#define UNUSED (0x3)
#define DELETED (0x0)
#define EMPTY_CHAR  (0)
#define MAX_FILES_SIZE (1000)
#define HALF_SIZE ((_flashSize_in_chars/2)*sizeof(char))
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_ON_DISK_SIZE (sizeof(FileHeaderOnDisk))
#define CHK_STATUS(n) if(n!=FS_SUCCESS)return n;
#define FLASH_CALL_BACK (0x01)
#define NO_HEADER (-1)
// wait for flash done call back- n is the returned flag
#define WAIT_FOR_FLASH_CB(n) {ULONG n;tx_event_flags_get(&fsFlag,FLASH_CALL_BACK,TX_OR_CLEAR,&n,TX_WAIT_FOREVER);};
#define READING_WRITING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)/FILE_HEADRES_ON_DISK_SIZE) // number of FileHeaders that can be read/write from the flash in a single read/write command
#define READING_WRITING_DATA_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)/sizeof(char)) // number of chars that can be read/write from the flash in a single read/write command

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

FileHeaderOnDisk UNUSED_FILEHEADER_ON_DISK;
/**
* global data types they are initialized at init and updated every write
*/
uint16_t _headerStartPos;
uint16_t _dataStartPos;
uint16_t _next_avilable_header_pos;
uint16_t _next_avilable_data_pos;
unsigned _block_count;

FileHeaderOnMemory _files[MAX_FILES_SIZE+10];
unsigned  _lastFile;
HALF _currentHalf;
unsigned _flashSize_in_chars;

/*
 * Wrapper write header to flash
 * to ensure no FileHeaderOnMemory write to disk
 */

result_t writeFileUsedForWrtitingToFlash(uint16_t address){
	return flash_write(address, (uint16_t)FILE_HEADRES_ON_DISK_SIZE, (uint8_t*) &fileUsedForWrtitingToFlash);
}
///*
// * Wrapper data header to flash
// * to ensure ....
// */
//result_t writeDataToFlash(uint16_t address,unsigned size,const char * data){
//	return flash_write(address, (uint16_t)size, (uint8_t*) data);
//
//}
/**
*    Must be called before any other operation on the file system.
*  Arguments:
*    settings - initialization information required to initialize the file system.
**/

void fillArrayWith1ones(void * pointer,size_t numOfbytes){
	memset(pointer,0xFF,numOfbytes);
}
/*
 * set header file on the flash to DELETED
 * headerPosOnDisk - pointer to the starting position on the flash
 */
FS_STATUS unactivateFileHeaderOnFlash(uint16_t headerPosOnDisk){
	fillArrayWith1ones((void*)(&fileUsedForWrtitingToFlash),FILE_HEADRES_ON_DISK_SIZE);
	fileUsedForWrtitingToFlash.valid=DELETED;
	result_t stat=writeFileUsedForWrtitingToFlash(headerPosOnDisk);
	if(stat!=OPERATION_SUCCESS) return FAILURE_ACCESSING_FLASH;
	return FS_SUCCESS;

}

/*
 * clear 1-2 duplicate files
 */
FS_STATUS clearDuplicateFiles(){
	FS_STATUS stat=FS_SUCCESS;
	int numOfDuplicateFilesDeleted=0;
	for(int i=0;i<_lastFile;i++){
		for(int j=i+1;j<_lastFile&&numOfDuplicateFilesDeleted<2;j++){
			if(strcmp(_files[i].onDisk.name,_files[j].onDisk.name)==0){
				//remove file i
				stat+= removeFileHeader(i);
				CHK_STATUS(stat);
				numOfDuplicateFilesDeleted++;
				i=i-1;
				j=_lastFile; // TODO should be break/continue
			}
		}
	}
	return stat;
}


/*
 * add a FileHeaderOnDisk to the memory.
 * assume the FileHeaderOnDisk isn't empty isusedOrCrashedOrDeletedHeader( f)!=0
 * if the FileHeaderOnDisk has a failure deleted it.
 *
 */

FS_STATUS addHeaderFileToMemory(FileHeaderOnDisk f){
	FS_STATUS stat=FS_SUCCESS;
	if(f.valid==UNUSED) {
		// failure - the headerfile is unused and not empty - should be DELETED
		stat=unactivateFileHeaderOnFlash(_next_avilable_header_pos);
		CHK_STATUS(stat);
	}
	else if(f.valid==USED){
		if(_lastFile>=MAX_FILES_SIZE+2) {
			stat=clearDuplicateFiles();
			CHK_STATUS(stat);
		}
		_files[_lastFile].onDisk.valid=USED;
		memcpy(_files[_lastFile].onDisk.name,f.name,sizeof(f.name));
		_files[_lastFile].onDisk.length=f.length;
		//		_files[_lastFile].data_end_pointer=_next_avilable_data_pos;
		_files[_lastFile].data_start_pointer=(uint16_t)(_next_avilable_data_pos-f.length);
		_files[_lastFile].adrress_of_header_on_flash=_next_avilable_header_pos;
		_lastFile++;
	}
	else{
		// the headerfile is DELETED
	}
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;
	_next_avilable_data_pos-=f.length;
	return stat;
}
/*
 * return 0 iff f is an empty(unused and unwritten ) FileHeaderOnDisk
 * meaning the FileHeaderOnDisk has only one's
 */
int isusedOrCrashedOrDeletedHeader(FileHeaderOnDisk f){
	return memcmp(&f,&UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE);
}
/*
 * set the pointers to the this half
 */
void setHalf(HALF half){
	_headerStartPos=sizeof(Signature);
	_next_avilable_header_pos	=sizeof(Signature);
	_dataStartPos=(uint16_t)(HALF_SIZE-1);
	_next_avilable_data_pos=(uint16_t)(HALF_SIZE-1);

	if(half==SECOND_HALF){
		_headerStartPos+=HALF_SIZE;
		_next_avilable_data_pos+=HALF_SIZE;
		_dataStartPos+=HALF_SIZE;
		_next_avilable_data_pos+=HALF_SIZE;

	}
}
/**
* restoreFileSystem if system crashed with a valid file system restore it by reading all of the fs header
* and indexing them also set global variables aboud data for eg num of files ect.
* @param startAdress where the file sytem starts in the flash
* @return success or failure reason
*/

FileHeaderOnDisk readedWritedFiles[READING_WRITING_HEADRS_SIZE];
result_t restoreFileSystem(HALF half){
	_currentHalf=half;
	_lastFile=0;
	setHalf(_currentHalf);
	memset(readedWritedFiles,0,sizeof(readedWritedFiles));
	result_t status=flash_read(_headerStartPos, FILE_HEADRES_ON_DISK_SIZE*READING_WRITING_HEADRS_SIZE,(uint8_t[]) readedWritedFiles);
	CHK_STATUS(status);
	int i=0;
	for(;isusedOrCrashedOrDeletedHeader(readedWritedFiles[i])!=0;i++){
		if(i==READING_WRITING_HEADRS_SIZE){ //need to read another headrsFiles
			memset(readedWritedFiles,0,sizeof(readedWritedFiles));
			status+=flash_read(_next_avilable_header_pos,FILE_HEADRES_ON_DISK_SIZE*READING_WRITING_HEADRS_SIZE,(uint8_t[]) readedWritedFiles);
			CHK_STATUS(status);
			i=0;
		}

		addHeaderFileToMemory(readedWritedFiles[i]);

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
Signature TmpBuffer[2];
FS_STATUS fs_init(const FS_SETTINGS settings){
	//	if(settings.block_count!=16)return COMMAND_PARAMETERS_ERROR; //TODO
	//TODO to change when connecting to the UI and NETWORK

	_block_count=settings.block_count*2; // *2 for using the Log-based file system
	fillArrayWith1ones((void*)&UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE); // setting a static file header that will be used at isEmptyFileHader(fh) method
	result_t status=flash_init(flash_data_recieve_cb,fs_wakeup);
	CHK_STATUS(status);
	status+=tx_event_flags_create(&fsFlag,"fsFlag");
	CHK_STATUS(status);
	_flashSize_in_chars=(_block_count)*NUM_OF_CHARS_IN_BLOCK;

	//read the first half Signature
	status+= flash_read(0, sizeof(Signature)*2,  (uint8_t*)TmpBuffer); //read first half Signature
	CHK_STATUS(status);
	if(TmpBuffer[0].valid==USED){
		status+=restoreFileSystem(FIRST_HALF);

	}
	else{
		// read the second half Signature
		status+= flash_read((uint16_t)HALF_SIZE, sizeof(Signature), ((uint8_t*) TmpBuffer));//read second half Signature
		CHK_STATUS(status);
		if(TmpBuffer[0].valid==USED){
			status+=restoreFileSystem(SECOND_HALF);
		}
		else{
			//init first
			status+=flash_bulk_erase_start();
			CHK_STATUS(status);
			WAIT_FOR_FLASH_CB(actualFlag1);
			_currentHalf=FIRST_HALF;
			//			Signature toWrite[2];
			fillArrayWith1ones(TmpBuffer,2*(sizeof(Signature)));
			TmpBuffer[0].valid=USED;
			status+=writeDataToFlash(0, sizeof(Signature),(char*)TmpBuffer);
			setHalf(_currentHalf);
			_lastFile=0;
			_files[0].onDisk.valid=UNUSED;
			_files[0].onDisk.length=0;
			//			_files[0].data_end_pointer=_next_avilable_data_pos;
		}
	}
	return status;

}

/*
 * remove file header from memory. and set it to DELETED on the flash
 * fileHeaderIndex - the index of the removed header file on the memory
 */
FS_STATUS removeFileHeader(int fileHeaderIndex){
	//remove from flash
	FS_STATUS status=unactivateFileHeaderOnFlash(_files[fileHeaderIndex].adrress_of_header_on_flash);
	CHK_STATUS(status);
	// remove from memory
	memmove((_files+fileHeaderIndex),(_files+fileHeaderIndex+1),
			(_lastFile-fileHeaderIndex)*sizeof(FileHeaderOnMemory));
	_lastFile--;
	return status;
}

/*
 *  loop over headers and compare header.filename to filename return headerNum or FILE_NOT_FOUND
 */
FS_STATUS FindFile(const char* filename, int *fileHeaderIndex){
	*fileHeaderIndex=NO_HEADER;
	// the bigest index is the relavent file
	for(int i=_lastFile;i>=0;i--){
		if(_files[i].onDisk.valid==USED && strcmp(_files[i].onDisk.name,filename)==0){
			if(*fileHeaderIndex!=NO_HEADER) {
				//the same file appeared twice - the lower index is irrelevant, delete it
				FS_STATUS status=removeFileHeader(i);
				CHK_STATUS(status);
				i--;
			}
			else *fileHeaderIndex=i;
		}
	}
	if(*fileHeaderIndex!=NO_HEADER) return FS_SUCCESS;
	else return FILE_NOT_FOUND;
}

/*
 * erase half of the flash
 */
FS_STATUS eraseHalf(HALF half){
	FS_STATUS stat;
	unsigned startAtblock=0;
	if(half==SECOND_HALF){
		startAtblock=_block_count/2;
	}
	for(int i=0;i<_block_count/2;i++){
		stat=flash_block_erase_start((uint16_t)((startAtblock+i)*BLOCK_SIZE));
		WAIT_FOR_FLASH_CB(flashFinshed1);
		CHK_STATUS(stat);
	}
	return stat;
}
/*
 * write data to flash and wait for finished call back
 */
FS_STATUS writeData(uint16_t start_address, uint16_t size, const uint8_t buffer[]){
	FS_STATUS stat=flash_write_start(start_address, size, buffer);
	WAIT_FOR_FLASH_CB(writeDataToFlash);
	return stat;
}
char dataBuffer[READING_WRITING_DATA_SIZE];
FS_STATUS copyFilesAndDataTo(uint16_t nextHalf_headerStartPos,uint16_t nextHalf_next_avilable_data_pos,
		uint16_t nextHalf_dataStartPos,uint16_t nextHalf_next_avilable_data_pos){

	FS_STATUS stat;
	fillArrayWith1ones((void*)dataBuffer,sizeof(dataBuffer));
	fillArrayWith1ones(readedWritedFiles,sizeof(readedWritedFiles));
	int dataBufferIndex=READING_WRITING_DATA_SIZE-1;
	int readedWritedFilesIndex=0;
	FileHeaderOnMemory *  currentFile=NULL;

	for(int i=0;i<_lastFile;i++){
		currentFile=&(_files[i]);

		if(dataBufferIndex-currentFile->onDisk.length<0){
			//write dataBuffer to flash
			stat+=wr(nextHalf_dataStartPos-sizeof(dataBuffer),sizeof(dataBuffer),dataBuffer);
			WAIT_FOR_FLASH_CB(writeDataBuffer);
			CHK_STATUS(stat);
			nextHalf_dataStartPos-=dataBufferIndex;

			fillArrayWith1ones((void*)dataBuffer,sizeof(dataBuffer));
			dataBufferIndex=READING_WRITING_DATA_SIZE-1;
		}
		if(readedWritedFilesIndex>=READING_WRITING_HEADRS_SIZE){
			// write readedWritedFiles to flash
			stat+=flash_write_start(nextHalf_headerStartPos,SIZE_OF_FILEHEADRS_IN_CHARS*readedWritedFilesIndex,readedWritedFiles);
			WAIT_FOR_FLASH_CB(writeHeadrFiles);
			CHK_STATUS(stat);

			//TODO  gjh

			fillArrayWith1ones(readedWritedFiles,sizeof(readedWritedFiles));
			readedWritedFilesIndex=0;
		}

		//copy FileHeaderOnMemory to next half

		CHK_STATUS(stat);
	}
	return stat;
}
//FS_STATUS validateHalf(HALF half){
//	return FAILURE;
//}
//FS_STATUS unValidateHalf(HALF half){
//	return FAILURE;;
//}
/*
 * change the half.
 * move it to the other Half
 */
FS_STATUS changehalf(){
	;*******************************************
	writeDataToFlash
	copyFilesAndDataTo
	changehalf
	***********************
	FS_STATUS stat;
	HALF nextHalf;
	if(_currentHalf==FIRST_HALF){
		nextHalf=SECOND_HALF;
	}
	else{//(half==SECOND_HALF)
		nextHalf=FIRST_HALF;
	}
	unsigned toadd=HALF_SIZE*nextHalf;
	uint16_t nextHalf_headerStartPos=(uint16_t)(toadd+sizeof(Signature));
	uint16_t nextHalf_next_avilable_data_pos=nextHalf_headerStartPos;
	uint16_t nextHalf_nextHalf_dataStartPos=(uint16_t)((HALF_SIZE-1)+toadd);
	uint16_t nextHalf_nextHalf_next_avilable_data_pos=nextHalf_nextHalf_dataStartPos;

	stat=eraseHalf(nextHalf);
	CHK_STATUS(stat);
	stat+=copyFilesAndDataTo(nextHalf_headerStartPos,nextHalf_next_avilable_data_pos,
			nextHalf_nextHalf_dataStartPos,nextHalf_nextHalf_next_avilable_data_pos);
	CHK_STATUS(stat);
	stat+=validateHalf(nextHalf);
	CHK_STATUS(stat);
	stat+=unValidateHalf(_currentHalf);
	setHalf(nextHalf);

	return  stat;
}

/*
 *
 * change the data of an existing file or write a new file to the file system.
 * write it to the flash
 * if fileHeaderIndex==NO_HEADER this is a new file.
 */
FileHeaderOnDisk fileUsedForWrtitingToFlash;
FS_STATUS writeNewDataToFlash(const char* filename, unsigned length,const char *data,int fileHeaderIndex){
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
		if(_lastFile>=MAX_FILES_SIZE){
			stat=clearDuplicateFiles();
			CHK_STATUS(stat);
		}
		if(_lastFile>=MAX_FILES_SIZE){
			//still no place
			return MAXIMUM_FILE_SIZE_EXCEEDED;
		}

		// create new FileHeaderOnMemory
		else {
			file=&_files[_lastFile];
			strcpy(file->onDisk.name,filename);
		}
	}

	//chk for flash size limit
	if(_next_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE) {
		// no place for the new/changed file
		stat=changehalf();
		CHK_STATUS(stat);
		if(_next_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE){
			//still no place
			return MAXIMUM_FLASH_SIZE_EXCEEDED;
		}
		return fs_write(filename, length, data); // try to write again
	}

	// write length and name to flash
	fillArrayWith1ones((void*)&fileUsedForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	strcpy(fileUsedForWrtitingToFlash.name,filename);
	fileUsedForWrtitingToFlash.length=length;
	file->onDisk.length=length;
	stat+=writeFileUsedForWrtitingToFlash(file->adrress_of_header_on_flash);
	CHK_STATUS(stat);

	// write data to flash
	//	file->data_end_pointer=_next_avilable_data_pos;
	_next_avilable_data_pos-=length;
	file->data_start_pointer=(uint16_t)(_next_avilable_data_pos+1);
	stat+=writeDataToFlash(file->data_start_pointer,length,data);
	CHK_STATUS(stat);

	// set the header on the flash to USED
	fillArrayWith1ones((void*)&fileUsedForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	fileUsedForWrtitingToFlash.valid=USED;
	stat+=writeFileUsedForWrtitingToFlash(file->adrress_of_header_on_flash);
	CHK_STATUS(stat);
	file->onDisk.valid=USED;

	file->adrress_of_header_on_flash=_next_avilable_header_pos;
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;
	if(fileHeaderIndex==NO_HEADER){
		_lastFile++;
	}

	return stat;

}
/*

  Description:
    Write a file.

  Arguments:
    filename - the name of the file.
    length - the size of the 'data' input buffer.
    data - a buffer holding the file content.

 */
volatile int is_writing=0;
FS_STATUS fs_write(const char* filename, unsigned length, const char* data){
	_disable();
	if(is_writing!=0){
		_enable();
		return FS_NOT_READY;
	}
	else {
		is_writing++;
		_enable();
	}

	length=length*sizeof(char); // length = # of byte to write
	if(length>0.5*KB) return MAXIMUM_FILE_SIZE_EXCEEDED;
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat!=FILE_NOT_FOUND){ // Existing file
		uint16_t headerOldPositon=_files[fileHeaderIndex].adrress_of_header_on_flash;
		stat=    writeNewDataToFlash(filename,length,data,fileHeaderIndex);
		CHK_STATUS(stat);
		// delete the previous header on the flash
		stat=unactivateFileHeaderOnFlash(headerOldPositon);
	}
	else stat= writeNewDataToFlash(filename,length,data,NO_HEADER);
	is_writing--;
	return stat;
}
/*

  Description:
	Erase a file.
  Arguments:
	filename - the name of the file.

 */
FS_STATUS fs_erase(const char* filename){
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	return removeFileHeader(fileHeaderIndex);
}

/*

  Description:
	Return the size of a file.
	File size is defined as the length of the file content only.

  Arguments:
	filename - the name of the file.
	length - an out parameter to hold the file size.

 */
FS_STATUS fs_filesize(const char* filename, unsigned* length){
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	*length=_files[fileHeaderIndex].onDisk.length;
	return FS_SUCCESS;
}
/*

  Description:
	Return the file count in the file system
  Arguments:
	file_count - an out argument to hold the number of files in the file system.

 */
FS_STATUS fs_count(unsigned* file_count){
	*file_count=0;
	FS_STATUS stat=FS_SUCCESS;
	for(int i=0;i<_lastFile;i++){
		if(_files[i].onDisk.valid==USED) (*file_count)=(*file_count)+1;
		else {
			stat=removeFileHeader(i);
			CHK_STATUS(stat);
			i--;
		}
	}
	return stat;
}
//char readData[0.5*KB];
/*
 * read the data from of fileHeaderIndex
 */
FS_STATUS readDataFromHeaderIndex(int fileHeaderIndex, unsigned* length, char* data){
	memset(data,0,*length);
	result_t flashStat=flash_read(_files[fileHeaderIndex].data_start_pointer,(uint16_t) _files[fileHeaderIndex].onDisk.length, (uint8_t*)data);
	*length=_files[fileHeaderIndex].onDisk.length;
	return flashStat;
}

/*

  Description:
	Read the content of a file.

  Arguments:
	filename - the name of the file.
	length - when calling the function this argument should hold the size of the 'data' input buffer.
			 when the function return this argument will hold the file size, i.e. the actual used space size of the 'data' buffer.
	data - a pointer for a buffer to hold the file content.


 */
FS_STATUS fs_read(const char* filename, unsigned* length, char* data){
	int fileHeaderIndex=NO_HEADER;
	FS_STATUS stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	stat=readDataFromHeaderIndex(fileHeaderIndex,length,data);
	return stat;
}
/*

  Description:
	List all the files exist in the file system.
  Arguments:
	length - when calling the function this argument should hold the size of the "files" buffer.
                 when the function return this argument will hold the the actual used space size of the 'files' buffer, including the last null byte.
	files - a series of continuous null terminated strings, each representing a file name in the file system

 */
FS_STATUS fs_list(unsigned* length, char* files){
	memset(files,0,*length);
	unsigned usedLen=0;
	FS_STATUS stat=FS_SUCCESS;
	for(int i=0;i<_lastFile;i++){
		if(_files[i].onDisk.valid==USED) {
			unsigned namelen=strlen(_files[i].onDisk.name)+1;
			if(usedLen+namelen>*length) return COMMAND_PARAMETERS_ERROR ; // not sufficient place at files buffer
			strcpy(&(files[usedLen]),_files[i].onDisk.name);
			usedLen+=namelen;
		}
		else {
			stat=removeFileHeader(i);
			CHK_STATUS(stat);
			i--;
		}
	}
	*length=usedLen;
	return stat;
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
	const char* file1data = "hello";
	const char* file2data = "bye";
	char data[MAX_FILE_SIZE];
	unsigned count=MAX_FILES_COUNT*MAX_FILE_SIZE;
	char *fileName;

	settings.block_count = 16;
	FS_STATUS stat=fs_init(settings);
	if ((stat )!=FS_SUCCESS) {
		return FS_NOT_READY;
	}
	stat+=stat;
	stat+=fs_write("file0", strlen(file1data), file1data);

	if ( stat!=FS_SUCCESS){
		return FS_NOT_READY;
	}
	stat+=stat;
	stat+=fs_write("file2", strlen(file2data), file2data);
	if (FS_SUCCESS != stat){
		return FS_NOT_READY;
	}
	stat+=stat;
	stat+=fs_list(&count, files);
	if (FS_SUCCESS != stat){
		return FS_NOT_READY;
	}
	stat+=stat;
	//	char * pointerTodata;
	//	pointerTodata=data;
	for( fileName=files ; count>0 ; count-- ) {
		//
		unsigned length = sizeof(data);
		stat = fs_read(fileName, &length, data);
		if (FS_SUCCESS != stat){
			stat+=stat;
			return FS_NOT_READY;
		}
		fileName+=strlen(fileName)+1;

	}
	return FS_SUCCESS;

}
