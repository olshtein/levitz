/*
 * network.h
 *
 *  Descriptor: API of the network device.
 *
 */

#include "network.h"
#define NETWORK_BASE_ADRESS (0x200000)
#define BYTE (8)
#define NULL (0)
#define NONE (0)
//--------------------------------------------------------------------------//
#pragma pack(1)

typedef struct {
	uint32_t  NTXBP; //Network Transmit Circular Buffer Pointer
	uint32_t  NTXBL; //Network Transmit Circular Buffer Length
	uint32_t  NTXFH; //Network Transmit FIFO Head Index
	uint32_t  NTXFT; //Network Transmit FIFO Tail Index
	uint32_t  NRXBP; //Network Receive Circular Buffer Pointer
	uint32_t  NRXBL; //Network Receive Circular Buffer Length
	uint32_t  NRXFH; //Network Receive FIFO Head Index
	uint32_t  NRXFT; //Network Receive FIFO Tail Index
	struct{
		unsigned int NetworkBusyBit					:1; // (NBSY)block any receive activity on the network device. while this bit is asserted new packets arriving at the hardware are dropped,(without packet dropped interrupt).
		unsigned int EnableTXInterrupt 				:1;//  assert IRQ14 upon packet TX completion
		unsigned int EnableRXInterrupt 				:1;//  assert IRQ14 upon packet RX completion
		unsigned int EnablePacketDoppedInterrupt 	:1;//  assert IRQ14 upon PacketDopped
		unsigned int EnableTransmitErrorInterrupt	:1;//  assert IRQ14 upon Transmit Error
		unsigned int reserved 						:24;//  rserved
		unsigned int NetworkOperatingMode			:3;//

	}NCTRL;//Network Control Register
	struct{
		unsigned int NetworkTransmitInProgress 	:1;//Hardware sets this bit when software sets the NTXL register. This bit remains set until the data transmission completes.
		unsigned int reserved0 					:1;//
		unsigned int NetworkReceiveInProgress 	:1;//Hardware automatically asserts this bit when a data packet receiving is in progress.
		unsigned int NetworkReceivebufferOverRun:1;//this bit is set to 1 when new packet was dropped because the Circular Buffer was full or too small
		unsigned int reserved1 					:4;//
		unsigned int NetworkInterruptRxComplete 	:1;// hosting system can read the value of this register upon interrupt, to determine the reason for the interrupt
		unsigned int NetworkInterruptTxComplete 	:1;
		unsigned int NetworkInterruptRxBufferSmall 	:1;
		unsigned int NetworkInterruptRxBufferFull 	:1;
		unsigned int NetworkInterruptTxBadDescriptor 	:1;
		unsigned int NetworkInterruptTxNetworkError 	:1;
		unsigned int NetworkInterruptReserved 	:2;
		unsigned int reserved2 					:16;//
	}NSTAT; //Network Status Register
} volatile REGISTERS;

//NSTAT - Network Status Register	READ ONLY
typedef enum NetStatus{//BITS 8-15
	statusNone=				(0x0),// None
			RxComplete=				(0x1) << 0,// Rx Complete (Successfully)
			TxComplete=				(0x1) << 1,// Tx Complete (Successfully)
			RxBufferSmall=			(0x1) << 2,// Rx Packet dropped because Rx buffer was too small
			RxBufferFull=			(0x1) << 3,// Rx Packet dropped because Circular Buffer was full
			TxBadDescriptor=		(0x1) << 4,// Tx failed due to bad descriptor
			TxNetworkError=			(0x1) << 5,// Tx failed due to network error

}NetStatus;
typedef struct
{
	uint8_t*        pBuffer;
	uint8_t 		buffer_size;
	uint8_t         used_size;
	uint8_t        status;

} Descriptor;

#pragma pack()

REGISTERS* my_network_regs = (REGISTERS*) NETWORK_BASE_ADRESS ;
network_call_back_t my_network_callbacks;

/*
 * save the callbacks function.
 * return true iff all the callbacks aren't null
 */
bool copyCallBacksSucess(network_call_back_t cb){
	if ((my_network_callbacks.dropped_cb=cb.dropped_cb)==NULL) return false;
	if ((my_network_callbacks.recieved_cb=cb.recieved_cb)==NULL) return false;
	if ((my_network_callbacks.transmit_error_cb=cb.transmit_error_cb)==NULL) return false;
	if ((my_network_callbacks.transmitted_cb=cb.transmitted_cb)==NULL) return false;
	return true;
}


/**********************************************************************
 *
 * Function:    network_init
 *
 * Descriptor:  Initialize the network device according to the given parameters.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Initialization done successfully.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *              NULL_POINTER:           One of the arguments points to NULL,
 *                                      (this return value should be returned only in case
 *                                      a given call back != NULL and the supplied buffer
 *                                      points to NULL ).
 *
 ***********************************************************************/
result_t network_init(const network_init_params_t *init_params){
	if(init_params==NULL||
			init_params-> size_r_buffer<2 ||
			init_params-> size_t_buffer<2 ||
			copyCallBacksSucess(init_params->list_call_backs)==false) return INVALID_ARGUMENTS;
	if(init_params->recieve_buffer==NULL || init_params->transmit_buffer==NULL) return NULL_POINTER;
	//
	//	trasmit buffer
	my_network_regs->NTXBL=init_params->size_t_buffer;
	my_network_regs->NTXBP=(uint32_t)(init_params->transmit_buffer);
	my_network_regs->NTXFH=0;
	my_network_regs->NTXFT=0;
	//recive buffer
	my_network_regs->NRXBL=init_params->size_r_buffer;
	my_network_regs->NRXBP=(uint32_t)init_params->recieve_buffer;
	my_network_regs->NRXFH=0;
	my_network_regs->NRXFT=0;
	//Network control register
	my_network_regs->NCTRL.NetworkOperatingMode=NETWORK_OPERATING_MODE_NORMAL;
	my_network_regs->NCTRL.EnablePacketDoppedInterrupt=1;
	my_network_regs->NCTRL.EnableTXInterrupt=1;
	my_network_regs->NCTRL.EnableRXInterrupt=1;
	my_network_regs->NCTRL.EnableTransmitErrorInterrupt=1;
	my_network_regs->NCTRL.NetworkBusyBit=0;
	//	 ack any interrupts
	//	my_network_regs->NSTAT.NetworkInterruptReason=NONE;
	return OPERATION_SUCCESS;

}
_Interrupt1 void network_ISR(){

	 Descriptor descriptor;
	 NetStatus netStatus;
	if (my_network_regs->NSTAT.NetworkInterruptTxComplete|my_network_regs->NSTAT.NetworkInterruptTxBadDescriptor|my_network_regs->NSTAT.NetworkInterruptTxNetworkError!=0){ //case is TRASMIT
		descriptor=((Descriptor*)my_network_regs->NTXBP)[my_network_regs->NTXFT];
		netStatus=descriptor.status;

		my_network_regs->NTXFT=(my_network_regs->NTXFT+1)%my_network_regs->NTXBL;
		if (netStatus &TxNetworkError){
			my_network_regs->NSTAT.NetworkInterruptTxNetworkError=1;
			my_network_callbacks.transmit_error_cb(NETWORK_ERROR, descriptor.pBuffer,descriptor.buffer_size,descriptor.used_size  );
		}
		else if (netStatus &TxComplete){
			my_network_regs->NSTAT.NetworkInterruptTxComplete=1;
			my_network_callbacks.transmitted_cb( descriptor.pBuffer, descriptor.buffer_size);
		}
		else if (netStatus &TxBadDescriptor){
			my_network_regs->NSTAT.NetworkInterruptTxBadDescriptor=1;
			my_network_callbacks.transmit_error_cb(BAD_DESCRIPTOR, descriptor.pBuffer, descriptor.buffer_size,descriptor.used_size );
		}
	}
	else if (my_network_regs->NSTAT.NetworkInterruptRxBufferFull|my_network_regs->NSTAT.NetworkInterruptRxBufferSmall|my_network_regs->NSTAT.NetworkInterruptRxComplete!=0){ //case is Recive

		if(my_network_regs->NSTAT.NetworkInterruptRxComplete==1){
			descriptor=((Descriptor*)my_network_regs->NRXBP)[my_network_regs->NRXFT];
			my_network_regs->NRXFT=(my_network_regs->NRXFT+1)%my_network_regs->NRXBL;
			netStatus=descriptor.status;
			my_network_regs->NSTAT.NetworkInterruptRxComplete=1;
			my_network_regs->NCTRL.NetworkBusyBit=0;

			my_network_callbacks.recieved_cb( descriptor.pBuffer,descriptor.buffer_size,descriptor.used_size  );
		}
		else if (my_network_regs->NSTAT.NetworkInterruptRxBufferSmall==1){
			my_network_regs->NSTAT.NetworkInterruptRxBufferSmall=1;
			my_network_regs->NCTRL.NetworkBusyBit=0;

			my_network_callbacks.dropped_cb(RX_BUFFER_TOO_SMALL );
		}
		else if (my_network_regs->NSTAT.NetworkInterruptRxBufferFull==1){
			my_network_regs->NSTAT.NetworkInterruptRxBufferFull=1;
			my_network_regs->NCTRL.NetworkBusyBit=0;

			my_network_callbacks.dropped_cb(CIRCULAR_BUFFER_FULL);
		}
	}
	else {//should NEVER happen
		network_set_operating_mode(NETWORK_OPERATING_MODE_RESERVE);
	}
}

/**********************************************************************
 *
 * Function:    network_set_operating_mode
 *
 * Descriptor:  Set the operating mode of the device to new_mode
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Setting the new mode done successfully.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *
 ***********************************************************************/
result_t network_set_operating_mode(network_operating_mode_t new_mode){
	if(new_mode!=NETWORK_OPERATING_MODE_RESERVE &&
			new_mode!=NETWORK_OPERATING_MODE_NORMAL &&
			new_mode!=NETWORK_OPERATING_MODE_SMSC &&
			new_mode!= NETWORK_OPERATING_MODE_INTERNAL_LOOPBACK) return INVALID_ARGUMENTS;
	my_network_regs->NCTRL.NetworkOperatingMode=new_mode;
	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    network_send_packet_start
 *
 * Descriptor:  Send "length" bytes from the buffer "buffer" whose size is
 *              "size" over the network.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      The request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *              NETWORK_TRANSMIT_BUFFER_FULL: There is no available descriptor for this request
 *
 ***********************************************************************/
result_t network_send_packet_start(const uint8_t buffer[], uint32_t size, uint32_t length){
	if(buffer==NULL) return NULL_POINTER;
	if(length>size ||length>NETWORK_MAXIMUM_TRANSMISSION_UNIT) return INVALID_ARGUMENTS;
	if(length==0) {
		return INVALID_ARGUMENTS;//TODO
	}
	if (((my_network_regs->NTXFH+1)%my_network_regs->NTXBL)==my_network_regs->NTXFT)return NETWORK_TRANSMIT_BUFFER_FULL;
	Descriptor*  Tranmisson_discripter=((Descriptor*)my_network_regs->NTXBP)+my_network_regs->NTXFH;
	Tranmisson_discripter->buffer_size=(uint8_t)size;
	Tranmisson_discripter->pBuffer=(uint8_t*)buffer;
	Tranmisson_discripter->used_size=(uint8_t)length;
	my_network_regs->NTXFH=((my_network_regs->NTXFH+1)%my_network_regs->NTXBL);

	return OPERATION_SUCCESS;
}


