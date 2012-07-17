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
#define FIRST_HALF (0)
#define SECOND_HALF ((_flashSize_in_chars/2)*sizeof(char))
#define SIZE_OF_FILEHEADRS_IN_CHARS (12*NUM_OF_CHARS_IN_KB)
#define NUM_OF_CHARS_IN_BLOCK (4*NUM_OF_CHARS_IN_KB)
#define FILE_HEADRES_SIZE (sizeof(FileHeader))
#define CHK_STATUS(n) if(n!=FS_SUCCESS)return n;

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
unsigned _headerStartPos;
unsigned _dataStartPos;
unsigned _next_avilable_header_pos;
FileHeader _lastAndUnusedHeaderFile;
unsigned _next_avilable_data_pos;
unsigned _headerFiles_num;


result_t restoreFileSystem(uint16_t startAdress){
	FileHeader files[READING_HEADRS_SIZE];
	result_t status=flash_read(startAdress, sizeof(FileHeader)*READING_HEADRS_SIZE,
			(uint8_t[]) files);
	CHK_STATUS(status);
	_headerStartPos=startAdress;
	_headerFiles_num=0;
	int i=0;
	for(;files[i].valid!=UNUSED;i++){
		if(i==READING_HEADRS_SIZE){ //need to read another headrsFiles
			startAdress+=sizeof(FileHeader)*READING_HEADRS_SIZE;
			status+=flash_read(startAdress, sizeof(FileHeader)*READING_HEADRS_SIZE,
					(uint8_t[]) files);
			CHK_STATUS(status);
			i=0;
		}
		if(files[i].valid==USED)_headerFiles_num++;
	}
	_next_avilable_data_pos=files[i].dataPointer;
	_next_avilable_header_pos=startAdress+(i*sizeof(FileHeader));
	memcpy(&_lastAndUnusedHeaderFile,&files[i],sizeof(FileHeader));
	return status;
}

void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size){
}
void flash_request_done_cb(){
}
/*

  Description:
	Initialize the file system.void tx_application_define(void *first_unused_memory) {
	status=intHARDWARE();


	//GUI_thread
	status+=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&guistack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

	//NetworkThread
	status+=tx_thread_create(&receiveThread, "NetworkReceiveThread", sendReceiveLoop, inputText,&receiveThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

	//create recive and send queue
	status=tx_queue_create(&receiveQueue, "receiveQueue", TX_1_ULONG, &receiveQueueStack, QUEUE_SIZE*sizeof(ULONG));
	status=tx_queue_create(&ToSendQueue, "ToSendQueue", TX_1_ULONG, &sendQueueStack, QUEUE_SIZE*sizeof(ULONG));
}

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
		_dataStartPos=SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
		status+=restoreFileSystem(FIRST_HALF+sizeof(Signature));

	}
	else{
		status+= flash_read((uint16_t)SECOND_HALF, sizeof(Signature), ((uint8_t*) TmpBuffer));
		CHK_STATUS(status);
		Signature* secondHalf=TmpBuffer;

		if(secondHalf->valid==USED){
			_dataStartPos=SECOND_HALF+SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
			status+=restoreFileSystem((uint16_t)(SECOND_HALF+sizeof(Signature)));
		}
		else{ //init first
			status+=flash_bulk_erase_start();
			uint8_t toWrite[2+(sizeof(FileHeader)/8)+(sizeof(Signature)/8)];
			Signature* firstHalf=(Signature*)toWrite;
			firstHalf->valid=USED;
			_dataStartPos=SIZE_OF_FILEHEADRS_IN_CHARS*sizeof(char);
			void *t =(firstHalf+1);
			FileHeader * currentHeader=(FileHeader *)(t);
			currentHeader->dataPointer=(uint16_t)_dataStartPos;
			currentHeader->valid=UNUSED;
			_next_avilable_header_pos=sizeof(Signature);
			_next_avilable_data_pos=currentHeader->dataPointer;
			_headerStartPos=_next_avilable_header_pos;
			_headerFiles_num=0;
			memcpy(&_lastAndUnusedHeaderFile,currentHeader,sizeof(FileHeader));
			status+=flash_write(FIRST_HALF, sizeof(Signature)+sizeof(uint16_t)+2,toWrite);
		}
	}
	CHK_STATUS(status);
	return FS_SUCCESS;

}
// goto Header and check file length (header[i].length-header[i+1].length)
FS_STATUS getLength(uint16_t headerNum ,uint16_t length){
	int stat=5;
//	readHeader(headerNum,Header & data);
//	length=data.legnth;
	return stat;
}
// loop over headers and compare header.filename to filename return headerNum or fail
FS_STATUS FindFile(const char* filename,uint16_t * headerNum){
	int i=0;
//
//	while(header[i].valid==USED){//HEADERS
//		if (filename==header[i].filename){
//			headerNum=i;
//			return SUCCESS;
//		}
//		i++;
//	}
	return FILE_NOT_FOUND;
}


FS_STATUS unactivateFile(uint16_t headerNum){
//	header[i].valid==DELETED;
	return FILE_NOT_FOUND;

}
FS_STATUS writeNewData(uint16_t length,char * data){
	return FILE_NOT_FOUND;

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
FileHeader file[2];
file[0]->valid=USED;
file[0]->dataPointer=_lastAndUnusedHeaderFile->dataPointer;
copyFileName(&file[0].name,filename);
file[1]->valid=UNUSED;
file[1]->dataPointer=file[0]->dataPointer+length*sizeof(char);
result_t
	//	int headerLoc=0;
//	int stat=FindFile(filename,headerLoc);
//	if (stat!=FILE_NOT_FOUND){
//		stat=	writeNewData(length,data);
//		if (stat!=SUCCESS) 	return stat;
//		stat=unactivateFile(headerLoc);
//		return stat;
//	}
//	return	writeNewData(length,data);
	return FILE_NOT_FOUND;

}
FS_STATUS fs_filesize(const char* filename, unsigned* length){
	/*int headerLoc=0;
	int stat=FindFile(filename,headerLoc);
	if (stat!=FILE_NOT_FOUND){
		return getLength(headerLoc,length);
	}void tx_application_define(void *first_unused_memory) {
	status=intHARDWARE();


	//GUI_thread
	status+=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&guistack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

	//NetworkThread
	status+=tx_thread_create(&receiveThread, "NetworkReceiveThread", sendReceiveLoop, inputText,&receiveThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

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




