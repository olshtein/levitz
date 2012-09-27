//code from our flash.c

// register locations
#define FLASH_STATUS_REG    (0x150)
#define FLASH_CONTROL_REG    (0x151)
#define FLASH_ADDRESS (0x152)
#define FLASH_DATA (0x153)
#define DATA_CELLS_NUM      16
#define DATA_CELL_SIZE_BYTE 4

 typedef unsigned char uint8_t;
    typedef unsigned short int uint16_t;
    typedef unsigned long int uint32_t;

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
#define CODE_MEMORY_LOCATION (0x00080D80)
#define FW_STACK_ADDR        0x000A25A8

#define MIN(X,Y) ( ((X) <= (Y)) ? (X) : (Y) )
#define ERROR (-1)
union SR{

	unsigned int sr             : 32;

	struct{
		unsigned int SCIP       : 1;
		unsigned int CYCLE_DONE : 1;
		unsigned int            : 30; //reserved
	}bitField;

};


union CR{

	unsigned int cr             : 32;

	struct{
		unsigned int SCGO          : 1;
		unsigned int SME           : 1; //IR enable register
		unsigned int               : 6; //reserved
		unsigned int COMMAND       : 8;
		unsigned int FD_Byte_Count : 6;
		unsigned int               : 10;//reserved
	}bitField;
};

int flash_is_ready(void){
	return ((_lr(FLASH_STATUS_REG)&(FLASH_STATUS_CYCLE_DONE|FLASH_STATUS_CYCLE_IN_PROGRESS))==0);
}

myFlashRead(int sizeToRead,int currentStartAddress){
//	int currentStartAddress=0;
	unsigned int tmpComannd=CMD_READ|FLASH_CONTROL_CYCLE_GO | (sizeToRead-1)<<16|FLASH_CONTROL_INTERRUPT_ENABLE;
	_sr(currentStartAddress,FLASH_ADDRESS);
	_sr(tmpComannd,FLASH_CONTROL_REG);



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

void copyResultToBuffer(uint8_t* to, uint32_t start, uint32_t size){
	unsigned int datReg=FLASH_DATA;
	unsigned int data=0;
	unsigned int dataByte;

	for(uint8_t i=0;i<size;i++){
		if(i%4==0) {
			data=changeEndian(_lr(datReg));
			datReg++;
		}
		dataByte=WRITE_TO_BUFFER_MASK & data;
		dataByte=dataByte>>24;
		to[start+i]|=(uint8_t)dataByte;
		data=data<<8;
	}

}
int fw_stack_addr   = FW_STACK_ADDR;
int code_memory_location   = CODE_MEMORY_LOCATION;

//int _flash_readWritePos=0;
int main(){
	int bytesLeft=65536;
	int currentSourcePos=0;
	int timeout=15000;
	int currDestinationPos=CODE_MEMORY_LOCATION;
	//should read whole flash file into dest or TIMEOUT
	while(bytesLeft > 0){

		uint32_t byte_to_read = MIN(bytesLeft, DATA_CELLS_NUM * DATA_CELL_SIZE_BYTE);
		myFlashRead(byte_to_read,currentSourcePos);

		while(!flash_is_ready()){
			timeout--;
			if (timeout<0)return ERROR;
		}

		timeout=15000;//reset timer

		copyResultToBuffer((uint8_t*)currDestinationPos, 0, byte_to_read);
		currentSourcePos+=byte_to_read;
		currDestinationPos+=byte_to_read;
		bytesLeft-=byte_to_read;
	}

	__asm__("mov %r1, fw_stack_addr");
	__asm__("ld  %sp, [%r1]");//make sp point to stack.
	__asm__("mov %r1, code_memory_location");
	__asm__("ld  %r1, [%r1]");
	__asm__("j   [%r1]");//Jump to loc of start of code
	__asm__("nop");
	return 0;
}
