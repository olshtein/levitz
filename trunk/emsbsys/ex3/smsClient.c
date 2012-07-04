/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"

#define PING_TIME (50) // the device send prob every PING_TIME
#define TIMER_EXPIRED (0x1) // timer expired event flag
#define RECIVED_MESSAGE (0x2)  // recieved message event flag
#define RECIVED_SUMBIT_ACK (0x3) // recieved submit ack event flag
#define TRANSMITED_SUCCSSES (0x1) // transmited success event flag
#define TRANSMITED_ERROR (0x2) // transmited error  event flag
#define ONE_SHOT (0)  // timer run only one shot
#define BUFF_SIZE (5) // the buffer size
#define RECIVED_LIST_SIZE (SEND_LIST_SIZE)
char myId[]={'7','7','0','7','7','0','7','7'}; // the device_id

SMS_PROBE probe_ack; // the prob/prob_ack that the smsClient sending to the server
unsigned int len12; // used for sending the prob/prob_ack
char probeBuffer12[NETWORK_MAXIMUM_TRANSMISSION_UNIT]; // used for sending the prob/prob_ack

//TODO some of this could be deleted
desc_t transmit_buffer[BUFF_SIZE];// transmited sms buffer
desc_t recieve_buffer[BUFF_SIZE]; // Received sms buffer
uint8_t recevedMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT]; // for network init
volatile int toSendListHead=0; // the head of toSend List - index to the next empty place
SMS_SUBMIT toSendList[SEND_LIST_SIZE]; // the toSend List
volatile int recivedListHead=0; // the head of received messages List - index to the next empty place
SMS_DELIVER recivedList[RECIVED_LIST_SIZE]; // the received messages List

volatile bool sendAckRecived; //TODO delete
volatile int data_length; // used for debugging
TX_EVENT_FLAGS_GROUP NetworkWakeupFlag; // the network flag
SMS_SUBMIT *volatile messageThatWasSent= NULL; // the last message that was send and we didn't get ack on him
//
extern TX_QUEUE ToSendQueue; // the messages to send queue
extern TX_QUEUE receiveQueue; // the received  messages  queue
TX_TIMER my_timer; // the timer


/*
 * wakeup the smsClient .
 * reason is TIMER_EXPIRED/RECIVED_MESSAGE/RECIVED_SUMBIT_ACK
 */
void wakeUp(ULONG reason){
	UINT status=0;
	//restart th timer
	status=tx_timer_deactivate(&my_timer);
	status+= tx_timer_change(&my_timer,PING_TIME,0);
	status+=tx_timer_activate(&my_timer);

	// set the event flag
	status+=tx_event_flags_set(&NetworkWakeupFlag,reason,TX_OR);
	if(status!=SUCCESS){
		//TODO handle error
		data_length=(int)status;
	}
}


/**
 * initlaise to smsClient
 * @ return SUCCESS if success
 */
result_t initSmsClient(){
	result_t result ;

	// create the network params
	network_init_params_t myCoolNetworkParms;
	for(int i=0;i<BUFF_SIZE ; i++){
		recieve_buffer[i].pBuffer = (uint32_t)recevedMsg[i];
		recieve_buffer[i].buff_size = (uint8_t)NETWORK_MAXIMUM_TRANSMISSION_UNIT;
		recieve_buffer[i].reserved =(uint16_t)0;
		transmit_buffer[i].reserved =(uint16_t)0;
	}
	myCoolNetworkParms.recieve_buffer=(desc_t*)recieve_buffer;
	myCoolNetworkParms.size_r_buffer=(uint32_t)BUFF_SIZE;
	myCoolNetworkParms.size_t_buffer=(uint32_t)BUFF_SIZE;
	myCoolNetworkParms.transmit_buffer=(desc_t*)transmit_buffer;
	myCoolNetworkParms.list_call_backs.dropped_cb=network_packet_dropped_cb1;
	myCoolNetworkParms.list_call_backs.recieved_cb=network_packet_received_cb1;
	myCoolNetworkParms.list_call_backs.transmit_error_cb=network_transmit_error_cb1;
	myCoolNetworkParms.list_call_backs.transmitted_cb=network_packet_transmitted_cb1;
	network_init_params_t * myCoolNetworkParmsPointer=&myCoolNetworkParms;
	//init the network
	result=network_init(myCoolNetworkParmsPointer);
	network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);

	// reset the smsClient attributes
	toSendListHead=0;
	recivedListHead=0;
	data_length=0;
	messageThatWasSent= NULL;
	memcpy(probe_ack.device_id,myId,sizeof(char)*ID_MAX_LENGTH);

	// create the event flag
	result+=tx_event_flags_create(&NetworkWakeupFlag,"NetworkWakeupFlag");

	//create the timer
	result+=tx_timer_create(&my_timer,"my_timer_name",wakeUp, TIMER_EXPIRED, PING_TIME, PING_TIME,TX_AUTO_ACTIVATE);
	return result;
}
/**
 * call back when a packet was transmissited.
 * @param buffer
 * @param size
 */

void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size){
	//    tx_event_flags_set(&NetworkWakeupFlag,TRANSMITED_SUCCSSES,TX_OR);
}
/**
 * call back when a packet was dropped during transmission.
 */
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length ){
	if(t==BAD_DESCRIPTOR)messageThatWasSent=NULL; // bad descriptor => Message can't be send
	//    tx_event_flags_set(&NetworkWakeupFlag,TRANSMITED_ERROR,TX_OR);
}

/*
 * set the mes_timeStamp according to the deliver_timestamp
 */
void setTime(char* mes_timeStamp,char* deliver_timestamp){
	int mes_index=0;
	mes_timeStamp[mes_index++]=deliver_timestamp[7];
	mes_timeStamp[mes_index++]=deliver_timestamp[6];
	mes_timeStamp[mes_index++]=':';
	mes_timeStamp[mes_index++]=deliver_timestamp[9];
	mes_timeStamp[mes_index++]=deliver_timestamp[8];
	mes_timeStamp[mes_index++]=':';
	mes_timeStamp[mes_index++]=deliver_timestamp[11];
	mes_timeStamp[mes_index++]=deliver_timestamp[10];
}


void reciveSms(SMS_DELIVER* deliver){
	// create a new Message
	Message mes;
	memcpy(mes.numberFromTo,(deliver->sender_id),sizeof(char)*ID_MAX_LENGTH);
	setTime(mes.timeStamp,(deliver->timestamp));
	//	memcpy(&mes.timeStamp,&(deliver->timestamp),sizeof(char)*ID_MAX_LENGTH);

	memcpy((mes.content),(deliver->data),sizeof(char)*deliver->data_length);
	mes.size=deliver->data_length;
	mes.inOrOut=IN;
	addNewMessageToMessages(&mes); //add it to the UI MessageList



}

/**
 * call back when a packet was received.
 * @param buffer
 * @param size
 * @param length
 */
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length){

	// try to parse the packet as SMS_DELIVER
	SMS_DELIVER deliver;
	if(embsys_parse_deliver((char*)buffer,&deliver)==SUCCESS){ // deliver message
		// add it to the received List and set the event flag
		recivedList[recivedListHead]=deliver;
		if(tx_queue_send(&receiveQueue, (void *)&(recivedListHead), TX_NO_WAIT)==TX_SUCCESS){
			recivedListHead=(recivedListHead+1)%RECIVED_LIST_SIZE;
			reciveSms(&deliver);
			wakeUp(RECIVED_MESSAGE);
		}
		else {
			//TODO handle error
			data_length=13;
		}


	}
	else{
		// try to parse the packet as SMS_SUBMIT_ACK

		SMS_SUBMIT_ACK subm_ack;
		if(embsys_parse_submit_ack((char*)buffer,&subm_ack)==SUCCESS){
			//chk if the is on the message that was submit
			if(messageThatWasSent->msg_reference==subm_ack.msg_reference){
				for(int k=0;k<ID_MAX_LENGTH ;k++){
					if(messageThatWasSent->recipient_id[k]!=subm_ack.recipient_id[k]) return;
					if(subm_ack.recipient_id[k]==0) break;
				}
				// set the event flag
				messageThatWasSent=NULL; //it was sended to the server
				wakeUp(RECIVED_SUMBIT_ACK);
			}
		}
		else {
			//it's probe ack

		}
	}
}

/**
 * call back when a packet was dropped during receiving.
 * @param t
 */
void network_packet_dropped_cb1(packet_dropped_reason_t t){
	data_length=t;
	//TODO handle error

}
/*
 * set the sms_recipient_id according to the mes_numberFromTo
 */
void getNumberTo(char* sms_recipient_id,char *mes_numberFromTo){
	int i;
	for(i=0;i<ID_MAX_LENGTH;i++){
		if(mes_numberFromTo[i]!=NULL_DIGIT)sms_recipient_id[i]=mes_numberFromTo[i];
		else break;
	}
	for(;i<ID_MAX_LENGTH;i++) sms_recipient_id[i]=0;

}

/**
 * send Message mes
 * @param mes the Message to send
 * @return SUCCESS if the message can be added to the tosend queue
 */
EMBSYS_STATUS sendMessage(Message *mes){
	ULONG sizeFreeToSend;
	if(tx_queue_info_get(&ToSendQueue,TX_NULL,TX_NULL,&sizeFreeToSend,TX_NULL,TX_NULL,TX_NULL)==TX_SUCCESS && sizeFreeToSend>0){
		// have place at the toSend queue
		int im_put_this=toSendListHead;
		toSendListHead=(toSendListHead+1)%SEND_LIST_SIZE;
		SMS_SUBMIT* toSend= &toSendList[im_put_this];
		memcpy(toSend->data,mes->content,mes->size*sizeof(char));
		toSend->data_length=mes->size;
		memcpy(toSend->device_id,myId,sizeof(char)*ID_MAX_LENGTH);
		getNumberTo(toSend->recipient_id,mes->numberFromTo);
		if(tx_queue_send(&ToSendQueue, (&toSend), TX_NO_WAIT)==TX_SUCCESS) return SUCCESS;
	}
	return FAIL;
}
/**
 * send Message from toSend List
 * send messageThatWasSending
 * @return
 */
result_t sendToSMSC(){
	//    SMS_SUBMIT sms;
	//    sms.data_length=SmsMessage->size;
	//    memcpy(&sms.data,&SmsMessage->content,sms.data_length*sizeof(char));
	//    memcpy(&sms.recipient_id,&SmsMessage->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
	//    memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	unsigned char buffer[NETWORK_MAXIMUM_TRANSMISSION_UNIT];
	unsigned length=NETWORK_MAXIMUM_TRANSMISSION_UNIT;
	SMS_SUBMIT* mymessage=messageThatWasSent;
	embsys_fill_submit((char *)buffer, mymessage, &length);

	result_t res=network_send_packet_start(buffer, NETWORK_MAXIMUM_TRANSMISSION_UNIT, length);

	return res;
}


/**
 * send a prob/prob_ack
 * @param deliver - send prob iff deliver==null
 */
void sendProbe(SMS_DELIVER *deliver){
	char isAck=0;
	if(deliver!=NULL){
		// prob_ack
		memcpy(probe_ack.sender_id,(deliver->sender_id),sizeof(char)*ID_MAX_LENGTH);
		memcpy(probe_ack.timestamp,(deliver->timestamp),sizeof(char)*TIMESTAMP_MAX_LENGTH);
		isAck='Y';
	}
	EMBSYS_STATUS  res1=embsys_fill_probe(probeBuffer12, &probe_ack, isAck ,&len12);
	result_t res2=network_send_packet_start((unsigned char *)probeBuffer12, NETWORK_MAXIMUM_TRANSMISSION_UNIT, len12);
	if(res1!=SUCCESS||res2!=SUCCESS){
		//TODO handle error
		data_length=res1;
		//            break;
	}
}

/*
 * the main smsClient thread loop . send and recive messages
 */
void sendReceiveLoop(){
	ULONG received_message;
	UINT status;



	SMS_SUBMIT* mymess;
	SMS_DELIVER* prob;
	ULONG actualFlags;
	while(1){
		// wait for timer/flag set
		tx_event_flags_get(&NetworkWakeupFlag,(TIMER_EXPIRED|RECIVED_MESSAGE|RECIVED_SUMBIT_ACK)
				,TX_OR_CLEAR,&actualFlags,PING_TIME);
		if(messageThatWasSent== NULL){ // if all sended message has been sended
			// chk if there is anonther messahe to send
			status = tx_queue_receive(&ToSendQueue, &mymess, TX_NO_WAIT);
			if (status==TX_SUCCESS){
				messageThatWasSent=mymess;
			}
		}
		if(messageThatWasSent!= NULL){ // if has message to send
			status=sendToSMSC();
			if (status!=OPERATION_SUCCESS){
				if(status==NETWORK_TRANSMIT_BUFFER_FULL){ // if NETWORK_TRANSMIT_BUFFER_FULL try again later
					//                         tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_ERROR|TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
				}
				else { //  message has error can't be sended
					messageThatWasSent=NULL;
				}
			}
			//                 else { // message was send toDriver
			//                     tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
			//                     tx_thread_sleep(80);
			//                 }
		}
		else {// no message to send , send ping/recived_ack
			status = tx_queue_receive(&receiveQueue, &received_message, TX_NO_WAIT);
			if (status == TX_QUEUE_EMPTY){//send ping
				prob=NULL;//send ping
			}
			else if (status==TX_SUCCESS){//send recived ack
				prob=&recivedList[received_message];
			}
			else{
				break;//TODO error
			}
			sendProbe(prob); //send the ping/recived_ack
		}

	}
}

//TODO delete?
void sendLoop(ULONG nothing){
	UINT status;
	ULONG actualFlags;
	while(1){
		SMS_SUBMIT* mymess;
		status = tx_queue_receive(&ToSendQueue, &mymess, TX_WAIT_FOREVER);

		if (status==TX_SUCCESS){
			messageThatWasSent=mymess;
			while (messageThatWasSent!= NULL ){
				status=sendToSMSC();
				if (status!=OPERATION_SUCCESS){
					if(status==NETWORK_TRANSMIT_BUFFER_FULL){
						tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_ERROR|TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);

					}
					else { //  message errore;
						messageThatWasSent=NULL;
					}
				}
				else { // message was send toDriver
					tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
					tx_thread_sleep(80);
				}

			}
		}
		else {
			break;//TODO shouldn't happened
		}
	}
}
