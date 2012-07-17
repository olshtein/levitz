
#include "flash.h"
# define NULL (0)
// register locations
#define FLASH_STATUS_REG    (0x150)
#define FLASH_CONTROL_REG    (0x151)
#define FLASH_ADDRESS (0x152)
#define FLASH_DATA (0x153)

// flags in the status register definition
#define FLASH_STATUS_CYCLE_DONE (0x02)
#define FLASH_STATUS_CYCLE_IN_PROGRESS (0x01) // hardware


// flags in the control register definition
#define CMD_READ (0x03<<8)
#define CMD_WRITE (0x05<<8)
#define CMD_ERASE_BLOCK (0xd8<<8)
#define CMD_ERASE_ALL (0xc7<<8)
#define FLASH_CONTROL_INTERRUPT_ENABLE (0x02)
#define FLASH_CONTROL_CYCLE_GO (0x01)
#define FLASH_CONTROL_DATA_BYTE_COUNT (0x3F<<16)
#define WRITE_TO_BUFFER_MASK (0xff000000)
#define BYTE (8)
#define LEFT_BYTE (0xff000000)
#define FLASH_CAPACITY (65536)
#define FLASH_NUM_OF_BLOCK (16)

//#define RESET_STATUS_VALUE (0x0)
//#define RESET_VALUE (0x0)

// constants for loading/storing data to the flash data array
#define LOAD_DATA (0)
#define STORE_DATA (1)
// the mask using for readqwrite
int writeFromBufferMask[]={0x000000ff,0x0000ffff,0x00ffffff};
int fillWithOnesMask[]={0x00000000,0x000000ff,0x0000ffff,0x00ffffff};
#pragma pack(1)

typedef enum {NONE,READ_START,READ_BLOCKING,WRITE_START,WRITE_BLOCKING,BULK_EARAZE,BLOCK_ERASE} OPERATION;
volatile OPERATION _flash_operation; // used for under stand if flush ready and what operation it working on
volatile uint32_t _flash_readWritePos=0; // the curent position on the _flush_sourceBuffer
volatile uint32_t _flash_readWriteSize; // the size of the opration needed to be done
volatile uint16_t _flash_currentCommandStartAddress; // the start address of the opration needed to be done
volatile uint32_t _flash_sizeWasRead; // the size that was readed at the prvious operation;
uint8_t const * volatile _flash_sourceBuffer ; // pointer to sthe buferr we read from/write to
uint8_t  * volatile _flash_targetBuffer ; // pointer to the buferr we write to
uint8_t _flash_internalBuffer[MAX_REQUEST_BUFFER_SIZE]; // internal buffer nonblocking for read/write

#pragma pack()



void (*my_flash_readDone_cb)(uint8_t const *buffer, uint32_t size)=NULL;// call back whenever asynchrony read operation done.
void (*my_flash_request_done_cb)(void)=NULL;//  call back whenever asynchrony operation don beside read

# define DISABLE_INTERRUPTS (_disable()) // used since disable interrupts dosn't working
# define ENABLE_INTERRUPTS (_enable()) // used since disable interrupts dosn't working
//
///*
// * return true if the arguments are Invalid
// */
//bool isInvalidArguments(uint16_t start_address, uint16_t size){
//	if(size>MAX_REQUEST_BUFFER_SIZE ||
//			size==0 ||
////			start_address>=FLASH_CAPACITY ||
//			((uint32_t)start_address+size)>FLASH_CAPACITY) return true;
//
//	return false;
//}

/*
 * wrtie result to buffer
 * assume all paremters arre legal
 * write size elemnts at start point
 */

void copyResultToBuffer(uint8_t buffer[], uint32_t start, uint32_t size){

	unsigned int datReg=FLASH_DATA;
	unsigned int data;
	unsigned int dataByte;

	for(uint8_t i=0;i<size;i++){
		if(i%4==0) {
			data=changeEndian(_lr(datReg));
			datReg++;
		}
		dataByte=WRITE_TO_BUFFER_MASK & data;
		dataByte=dataByte>>24;
		buffer[start+i]|=(uint8_t)dataByte;
		data=data<<8;
	}

}
/*
 * copy form buffer to data adress
 * assume all parametrs are legal
 * copy size elments  from start point
 */
void copyFromBufferToDataRegs(const uint8_t buffer[], uint32_t start, uint32_t size){
	unsigned int datReg=FLASH_DATA;
	unsigned int data=0;
	unsigned int sizeInt=size;
	unsigned int newSizeInt=size;

	if (sizeInt%4!=0)newSizeInt=((sizeInt/4)+1)*4;
	for(unsigned int i=0;i<newSizeInt;i++){
		data=data<<BYTE;
		if(i<sizeInt){
			data|=buffer[start+i];
		}
		if(i%4==3){
			if(i>=sizeInt){

				data|=fillWithOnesMask[newSizeInt-sizeInt];
			}
			_sr(changeEndian(data),datReg);
			data=0;

			datReg++;
		}
	}
}
/*
 * write max(_flush_readWriteSize-_flush_readWritePos,MAX_DATA_READ_WRITE_SIZE) from _flush_sourceBuffer to the flush start at (start_address+_flush_readWritePos) address
 */
void myFlushWrite(){
	if(_flash_readWriteSize>_flash_readWritePos){
		uint32_t size_to_write =_flash_readWriteSize-_flash_readWritePos;
		if(size_to_write>=MAX_DATA_READ_WRITE_SIZE) size_to_write=MAX_DATA_READ_WRITE_SIZE;
		copyFromBufferToDataRegs(_flash_sourceBuffer,_flash_readWritePos,size_to_write);
		unsigned int tmpComannd=CMD_WRITE|FLASH_CONTROL_CYCLE_GO | (size_to_write-1)<<16;
		if(_flash_operation==WRITE_START)tmpComannd|=FLASH_CONTROL_INTERRUPT_ENABLE;
		_sr(_flash_currentCommandStartAddress+_flash_readWritePos,FLASH_ADDRESS);
		_sr(tmpComannd,FLASH_CONTROL_REG);
		_flash_readWritePos+=size_to_write;
	}

}

/*_Interrupt1*/ void flash_interrupt(){
	_sr(FLASH_STATUS_CYCLE_DONE,FLASH_STATUS_REG);// acknolege
	switch(_flash_operation){
	case READ_START:
		// copy readed data to buffer
		copyResultToBuffer(_flash_targetBuffer,_flash_readWritePos,_flash_sizeWasRead);// copy the elemnts to the buffer
		_flash_readWritePos+=_flash_sizeWasRead;
		if(_flash_readWriteSize>_flash_readWritePos)	myFlushRead();
		else {
			_flash_operation=NONE;
			_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG); // enable interrupts
			my_flash_readDone_cb(_flash_targetBuffer,_flash_readWriteSize);
		}
		break;
	case WRITE_START:
		if(_flash_readWriteSize>_flash_readWritePos){
			myFlushWrite();
		}
		else {
			_flash_operation=NONE;
			_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG); // enable interrupts
			my_flash_request_done_cb();
		}
		break;
	case BULK_EARAZE:
	// do nothing because the interrupt can't be shout down
	case BLOCK_ERASE:
		_flash_operation=NONE;
		_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG); // enable interrupts
		my_flash_request_done_cb();
		break;
	case READ_BLOCKING:
	case WRITE_BLOCKING:
		break;
	case NONE:
		break;
	}
}


/*
 * change from little endian to big and otherwise
 *
 *
 */

unsigned int changeEndian(unsigned int data){
	unsigned int newData=0;
	for (int i=0;i<4;i++){
		newData=newData>>BYTE;
		newData|=(data&LEFT_BYTE);
		data=data<<BYTE;
	}
	return newData;
}



/*
 *  copy size elemnets form the array source to the target.
 */
copyArray( const uint8_t source[],uint8_t target[],uint32_t size){
	for (uint32_t i=0;i<size;i++){
		target[i]=source[i];
	}
}

/*
 *
 * reset the flash
 */
void resetFlash(){
	//	_sr(FLASH_STATUS_CYCLE_DONE,FLASH_STATUS_REG);
	_sr(0,FLASH_STATUS_REG);
	_flash_operation=NONE;
	_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG); // enable interrupts
}
/**********************************************************************
 *
 * Function:    flash_init
 *
 * Descriptor:  Initialize the flash device.
 *
 * Parameters:  flash_data_recieve_cb: call back whenever asynchrony read operation done.
 *              flash_request_done_cb: call back whenever asynchrony operation done
 *                                     (besides read).
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Initialization successfully done.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_init( void (*flash_data_recieve_cb)(uint8_t const *buffer, uint32_t size),void (*flash_request_done_cb)(void)){
	if(flash_data_recieve_cb==NULL || flash_request_done_cb==NULL) return NULL_POINTER;
	my_flash_readDone_cb=flash_data_recieve_cb;
	my_flash_request_done_cb=flash_request_done_cb;
	resetFlash();
	return OPERATION_SUCCESS;
}

/**********************************************************************
 *
 * Function:    flash_is_ready
 *
 * Descriptor:
 *
 * Notes:
 *
 * Return:      true in case the FW ready to accept a new request.
 *
 ***********************************************************************/
bool flash_is_ready(void){
	if ((_lr(FLASH_STATUS_REG)&(FLASH_STATUS_CYCLE_DONE|FLASH_STATUS_CYCLE_IN_PROGRESS))==0){
		return _flash_operation==NONE;
	}
	return false;
}
/*
 * write max(_flush_readWriteSize-_flush_readWritePos,MAX_DATA_READ_WRITE_SIZE) from the flush start at (start_address+_flush_readWritePos) address to the _flush_targetBuffer
 */

myFlushRead(){
	if(_flash_readWriteSize>_flash_readWritePos){
		// read max(MAX_DATA_READ_WRITE_SIZE,size)

		uint32_t size_to_read=_flash_readWriteSize-_flash_readWritePos;
		if(size_to_read>=MAX_DATA_READ_WRITE_SIZE) size_to_read=MAX_DATA_READ_WRITE_SIZE;

				unsigned int tmpComannd=CMD_READ|FLASH_CONTROL_CYCLE_GO | (size_to_read-1)<<16;
		if(_flash_operation==READ_START)tmpComannd|=FLASH_CONTROL_INTERRUPT_ENABLE;
		_sr(_flash_currentCommandStartAddress+_flash_readWritePos,FLASH_ADDRESS);
		_sr(tmpComannd,FLASH_CONTROL_REG);
		_flash_sizeWasRead=size_to_read;

	}
	else _flash_sizeWasRead=0;


}

/**********************************************************************
 *
 * Function:    flash_read_start
 *
 * Descriptor:  Read "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_read_start(uint16_t start_address, uint16_t size){


	if(size==0) return INVALID_ARGUMENTS;
	//	if(buffer==NULL) return NULL_POINTER;
	if(!flash_is_ready()) return NOT_READY;

	DISABLE_INTERRUPTS;
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=READ_START;
	ENABLE_INTERRUPTS;

	// start read

	_flash_readWritePos=0;
	_flash_readWriteSize=size;
	_flash_currentCommandStartAddress=start_address;
	_flash_targetBuffer=_flash_internalBuffer;
	_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG);// wnable interrupts
	if(_flash_readWriteSize>_flash_readWritePos){
		myFlushRead();

	}
	return OPERATION_SUCCESS;

}


/**********************************************************************
 *
 * Function:    flash_read
 *
 * Descriptor:  Same as flash_read_start except the data received in the supplied
 *              buffer.
 *              This function is blocking.

 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_read(uint16_t start_address, uint16_t size, uint8_t buffer[]){


	if(size==0) return INVALID_ARGUMENTS;
	if(buffer==NULL) return NULL_POINTER;
	if(!flash_is_ready()) return NOT_READY;

	DISABLE_INTERRUPTS;
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=READ_BLOCKING;
	ENABLE_INTERRUPTS;
	// start read

	_flash_readWritePos=0;
	_flash_readWriteSize=size;
	_flash_currentCommandStartAddress=start_address;
	_flash_targetBuffer=buffer;
	while(_flash_readWriteSize>_flash_readWritePos){
		myFlushRead();
		while((_lr(FLASH_STATUS_REG)&FLASH_STATUS_CYCLE_IN_PROGRESS)!=0){};//busy wait
		//		while((_lr(FLASH_STATUS_REG)&FLASH_STATUS_CYCLE_IN_PROGRESS)!=0){};//busy wait
		copyResultToBuffer(_flash_targetBuffer,_flash_readWritePos,_flash_sizeWasRead);// copy the elemnts to the buffer
		_flash_readWritePos+=_flash_sizeWasRead;

	}
	_flash_operation=NONE;
	_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG);// wnable interrupts

	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_write_start
 *
 * Descriptor:  Write "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_write_start(uint16_t start_address, uint16_t size, const uint8_t buffer[]){
	if(size==0) return INVALID_ARGUMENTS;
	if(buffer==NULL) return NULL_POINTER;
	if(!flash_is_ready()) return NOT_READY;

	DISABLE_INTERRUPTS;
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=WRITE_START;
	ENABLE_INTERRUPTS;

	// start write
	_flash_readWritePos=0;
	_flash_readWriteSize=size;
	_flash_currentCommandStartAddress=start_address;
	copyArray(buffer,(uint8_t[])_flash_internalBuffer,size);
	_flash_sourceBuffer=_flash_internalBuffer;
	if(_flash_readWriteSize>_flash_readWritePos){
		myFlushWrite();
	}

	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_write
 *
 * Descriptor:  Same as flash_write_start.
 *              This function is blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/

result_t flash_write(uint16_t start_address, uint16_t size, const uint8_t buffer[]){
	if(size==0) return INVALID_ARGUMENTS;
	if(buffer==NULL)return NULL_POINTER;
	if(!flash_is_ready()) return NOT_READY;
	DISABLE_INTERRUPTS;
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=WRITE_BLOCKING;
	ENABLE_INTERRUPTS;
	// start write
	_flash_readWritePos=0;
	_flash_readWriteSize=size;
	_flash_currentCommandStartAddress=start_address;
	//	copyArray(buffer,(uint8_t[])_flush_internalBuffer,size);
	_flash_sourceBuffer=buffer;
	_flash_currentCommandStartAddress=start_address;
	while(_flash_readWriteSize>_flash_readWritePos){
		myFlushWrite();
		while((_lr(FLASH_STATUS_REG)&FLASH_STATUS_CYCLE_IN_PROGRESS)!=0){};//busy wait
	}
	_flash_operation=NONE;
	_sr(FLASH_CONTROL_INTERRUPT_ENABLE,FLASH_CONTROL_REG);// wnable interrupts


	return OPERATION_SUCCESS;
}

/**********************************************************************
 *
 * Function:    flash_bulk_erase_start
 *
 * Descriptor:  Erase all the content that find on the flash device.
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_bulk_erase_start(void){
	if(!flash_is_ready())return  NOT_READY; //chk if the flash isn't busy
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=BLOCK_ERASE;
	unsigned int tmp=CMD_ERASE_ALL|FLASH_CONTROL_CYCLE_GO | FLASH_CONTROL_INTERRUPT_ENABLE;
	_sr(tmp,FLASH_CONTROL_REG);
	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_block_erase_start
 *
 * Descriptor:  Erase the content that find on the block that find on address
 *              "start_address".
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_block_erase_start(uint16_t start_address){
	if(!flash_is_ready())return  NOT_READY; //chk if the flash isn't busy
	_sr(0,FLASH_CONTROL_REG);// disable interrupts
	_flash_operation=BLOCK_ERASE;
	unsigned int tmp=CMD_ERASE_BLOCK|FLASH_CONTROL_CYCLE_GO | FLASH_CONTROL_INTERRUPT_ENABLE;
	_sr(start_address,FLASH_ADDRESS);
	_sr(tmp,FLASH_CONTROL_REG);
	return OPERATION_SUCCESS;
}

