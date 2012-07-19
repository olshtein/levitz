#include "fs.h"
#include "flash.h"
#include "common_defs.h"
#include "tx_api.h"
#include "timer.h"
#include "string.h"
//#include "stdio.h"

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
#define READING_HEADRS_SIZE ((MAX_DATA_READ_WRITE_SIZE*2)/FILE_HEADRES_ON_DISK_SIZE) // number of FileHeaders that can be read from the flash in a single read command

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
FileHeaderOnDisk fileUsedForWrtitingToFlash;

/*
 * Wrapper write header to flash
 * to ensure no FileHeaderOnMemory write to disk
 */

result_t writeFileUsedForWrtitingToFlash(uint16_t address){
	return flash_write(address, (uint16_t)FILE_HEADRES_ON_DISK_SIZE, (uint8_t*) &fileUsedForWrtitingToFlash);
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

// clear 1-2 duplicate files
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
	return FS_SUCCESS;
}
/*
 * return 0 iff f is an empty(unused and unwritten ) FileHeaderOnDisk
 *
 */
int isusedOrCrashedOrDeletedHeader(FileHeaderOnDisk f){
	return memcmp(&f,&UNUSED_FILEHEADER_ON_DISK,FILE_HEADRES_ON_DISK_SIZE);
}
/**
* restoreFileSystem if system crashed with a valid file system restore it by reading all of the fs header
* and indexing them also set global variables aboud data for eg num of files ect.
* @param startAdress where the file sytem starts in the flash
* @return success or failure reason
*/
FileHeaderOnDisk f[READING_HEADRS_SIZE];

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
	memset(f,0,sizeof(f));
	result_t status=flash_read(_headerStartPos, FILE_HEADRES_ON_DISK_SIZE*READING_HEADRS_SIZE,(uint8_t[]) f);
	CHK_STATUS(status);
	int i=0;
	for(;isusedOrCrashedOrDeletedHeader(f[i])!=0;i++){
		if(i==READING_HEADRS_SIZE){ //need to read another headrsFiles
			memset(f,0,sizeof(f));
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
Signature TmpBuffer[2];
FS_STATUS fs_init(const FS_SETTINGS settings){
	if(settings.block_count!=16)return COMMAND_PARAMETERS_ERROR; //TODO
	//TODO to change when connecting to the UI and NETWORK
_block_count=settings.block_count;
	fillArrayWith1ones((void*)&UNUSED_FILEHEADER_ON_DISK,sizeof(FileHeaderOnDisk)); // setting a static file header that will be used at isEmptyFileHader(fh) method
	result_t status=flash_init(flash_data_recieve_cb,fs_wakeup);
	CHK_STATUS(status);
	status+=tx_event_flags_create(&fsFlag,"fsFlag");
	CHK_STATUS(status);
	_flashSize_in_chars=(settings.block_count)*NUM_OF_CHARS_IN_BLOCK;
	status+= flash_read(0, sizeof(Signature)*2,  (uint8_t*)TmpBuffer); //read first half Signature
	CHK_STATUS(status);
	//	Signature* firstHalf=(Signature*)TmpBuffer;
	if(TmpBuffer[0].valid==USED){
		status+=restoreFileSystem(FIRST_HALF);

	}
	else{
		status+= flash_read((uint16_t)HALF_SIZE, sizeof(Signature), ((uint8_t*) TmpBuffer));//read second half Signature
		CHK_STATUS(status);
		//		Signature* secondHalf=TmpBuffer;

		if(TmpBuffer[0].valid==USED){
			status+=restoreFileSystem(SECOND_HALF);
		}
		else{ //init first
			status+=flash_bulk_erase_start();
			CHK_STATUS(status);
			WAIT_FOR_FLASH_CB(actualFlag1);
			_currentHalf=FIRST_HALF;
			//			Signature toWrite[2];
			fillArrayWith1ones(TmpBuffer,2*(sizeof(Signature)));
			TmpBuffer[0].valid=USED;
			_lastFile=0;
			_headerStartPos=sizeof(Signature);
			_next_avilable_header_pos=sizeof(Signature);
			_dataStartPos=(uint16_t)(HALF_SIZE-1);
			_next_avilable_data_pos=(uint16_t)(HALF_SIZE-1);
			status+=writeDataToFlash(0, sizeof(Signature),(char*)TmpBuffer);
			_files[0].onDisk.valid=UNUSED;
			_files[0].onDisk.length=0;
			//			_files[0].data_end_pointer=_next_avilable_data_pos;
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


FS_STATUS eraseHalf(HALF half){
	FS_STATUS stat=FAILURE;
unsigned startAtblock=0;
if(half==SECOND_HALF){
	startAtblock=_block_count/2;
}
for(int i=0;i<_block_count/2;i++){
//	flash_block_erase_start((startAtblock+i)*);
}
	return stat;
}
FS_STATUS copyFilesAndDataTo(uint16_t nextHalf_headerStartPos,uint16_t nextHalf_next_avilable_data_pos,
		uint16_t nextHalf_nextHalf_dataStartPos,uint16_t nextHalf_nextHalf_next_avilable_data_pos){
	return FAILURE;;
}
FS_STATUS validateHalf(HALF half){
	return FAILURE;
}
FS_STATUS unValidateHalf(HALF half){
	return FAILURE;;
}
void setHalf(HALF nextHalf,uint16_t nextHalf_headerStartPos,uint16_t nextHalf_next_avilable_data_pos,
		uint16_t nextHalf_nextHalf_dataStartPos,uint16_t nextHalf_nextHalf_next_avilable_data_pos)
{}
/*
 * change the half.
 * move it to the other Half
 */
FS_STATUS changehalf(){
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
	uint16_t nextHalf_next_avilable_data_pos=(uint16_t)(toadd+sizeof(Signature));
	uint16_t nextHalf_nextHalf_dataStartPos=(uint16_t)((HALF_SIZE-1)+toadd);
	uint16_t nextHalf_nextHalf_next_avilable_data_pos=(uint16_t)((HALF_SIZE-1)+toadd);

	stat=eraseHalf(nextHalf);
	CHK_STATUS(stat);
	stat+=copyFilesAndDataTo(nextHalf_headerStartPos,nextHalf_next_avilable_data_pos,
			nextHalf_nextHalf_dataStartPos,nextHalf_nextHalf_next_avilable_data_pos);
	CHK_STATUS(stat);
	stat+=validateHalf(nextHalf);
	CHK_STATUS(stat);
	stat+=unValidateHalf(_currentHalf);
	setHalf(nextHalf,nextHalf_headerStartPos,nextHalf_next_avilable_data_pos,
			nextHalf_nextHalf_dataStartPos,nextHalf_nextHalf_next_avilable_data_pos);

	return  stat;
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


	if(_next_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE) {
		// no place for the new/changed file
		if(fileHeaderIndex==NO_HEADER)_lastFile--;

		stat=changehalf();
		CHK_STATUS(stat);

		if(_next_avilable_data_pos-length<_next_avilable_header_pos+FILE_HEADRES_ON_DISK_SIZE)
			//still no place
			return MAXIMUM_FLASH_SIZE_EXCEEDED;

		return fs_write(filename, length, data); // try to write again
	}

	// write length and name to flash
	file->adrress_of_header_on_flash=_next_avilable_header_pos;
	_next_avilable_header_pos+=FILE_HEADRES_ON_DISK_SIZE;

	fillArrayWith1ones((void*)&fileUsedForWrtitingToFlash,FILE_HEADRES_ON_DISK_SIZE);
	strcpy(fileUsedForWrtitingToFlash.name,filename);
	fileUsedForWrtitingToFlash.length=length;
	strcpy(file->onDisk.name,filename);
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
	length=length*sizeof(char); // length = # of byte to write
	if(length>0.5*KB) return MAXIMUM_FILE_SIZE_EXCEEDED;
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
FS_STATUS fs_erase(const char* filename){
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	return removeFileHeader(fileHeaderIndex);
}

FS_STATUS fs_filesize(const char* filename, unsigned* length){
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	*length=_files[fileHeaderIndex].onDisk.length;
	return FS_SUCCESS;
}
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
FS_STATUS fs_read(const char* filename, unsigned* length, char* data){
	int fileHeaderIndex=NO_HEADER;
	int stat=FindFile(filename,&fileHeaderIndex);
	if (stat==FILE_NOT_FOUND) return FILE_NOT_FOUND; // no need for this row , but it look nicer with it
	CHK_STATUS(stat);
	memset(data,0,*length);
	result_t flashStat=flash_read(_files[fileHeaderIndex].data_start_pointer,(uint16_t) _files[fileHeaderIndex].onDisk.length, (uint8_t*)data);
	CHK_STATUS(stat);
	*length= (_files[fileHeaderIndex].onDisk.length)/sizeof(char);
	//	memcpy(data,readData,*length);
	return FS_SUCCESS;
}
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
