///*
// * ex3.c
// *
// *  Created on: Jun 11, 2012
// *      Author: levio01
// */
////#include "tx_port.h"
//#include "LCD.h"
//#include "tx_api.h"
//#include "timer.h"
//#include "UI.h"
//#include "input_panel.h"
//#include "tx_port.h"
//#include "network.h"
//#include "embsys_sms_protocol.h"
//#include "messages.h"
////typedef unsigned long                           ULONG;
////#include "string.h"
////#include "stdio.h"
//
//TX_THREAD thread_0;
//TX_THREAD thread_1;
//TX_QUEUE queue_0;
//TX_EVENT_FLAGS_GROUP event_flags_0;
////TX_BYTE_POOL byte_pool_0;
////TX_BLOCK_POOL block_pool_0;
//void threadStartMethod1(ULONG string);
//void threadStartMethod2(ULONG string);
//#define QUEUE_NUM_OF_MESSAGES (8)
//#define QUEUE_SIZE ((sizeof(desc_t *) )*QUEUE_NUM_OF_MESSAGES)
//desc_t * queueBuff[QUEUE_SIZE];
//int kl=0;
//void none(){
//	//		printf("sizeof%d \4= %d\n",sizeof(Message),sizeof(Message)/4);
//}
//void intHARDWARE(){
//	lcd_init(none);
//	initUI();
//	ip_init(inputPanelCallBack);
//	network_init(NULL);
//	ip_enable();
//}
//int main(int argc, char **argv) {
//	//none();
//	intHARDWARE();
//	tx_kernel_enter();
//	return 0;
//}
//void threadStartMethod1(ULONG s){
//	for(int i=0;i>-1;i++){
//		if (i%2000==0){
//			//			printf("%d 11\n",s);
//			/* Set event flag 0 to wakeup thread . */
//			//			int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
//			tx_thread_sleep(1);
//		}
//	}
//}
//
//void threadStartMethod2(ULONG s){
//	for(int i=0;i>-1;i++){
//		if (i%1000==0){
//			ULONG actual_flags;
//			//			int status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
//			//			if ((status != TX_SUCCESS) || (actual_flags != 0x1))break;
//			tx_thread_sleep(1);
//			//			printf("%d 22\n",s);
//		}
//	}
//}
//int status;
//ULONG inputText=16;
//char stack1[STACK_SIZE];
//char stack2[STACK_SIZE];
//void tx_application_define(void *first_unused_memory) {
//	status= tx_queue_create(&queue_0, "recived desc_t*",sizeof(desc_t *),&queueBuff, QUEUE_SIZE);
//	status=tx_queue_flush(&queue_0);
//
//
//	status=timer0_register(1,true,none);
//
//	status=tx_thread_create(&thread_1, "Thread2", threadStartMethod2, inputText,&stack2, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
//	status=tx_thread_create(&thread_0, "_Thread1", threadStartMethod1, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
//	status=tx_event_flags_create(&event_flags_0, "event flags 0");
//	//	_enable();
//}
/*
 * UiTester.c
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */

#include "tx_api.h"
#include "UI.h"
#include "messages.h"
#include "input_panel.h"
#include "timer.h"
#include "smsClient.h"
#define QUEUE_SIZE (SEND_LIST_SIZE)

TX_QUEUE receiveQueue;
TX_QUEUE ToSendQueue;

int status;

TX_THREAD receiveThread;
TX_THREAD sendThread;
TX_THREAD GUI_thread;
TX_THREAD PingThread;

char guistack[STACK_SIZE];
char receiveThreadStack[STACK_SIZE];
char sendThreadStack[STACK_SIZE];
//char Pingstack[STACK_SIZE];
ULONG receiveQueueStack[QUEUE_SIZE];
ULONG sendQueueStack[QUEUE_SIZE];
ULONG inputText=16;
int kk=0;

void mainloop(ULONG a);
void none(){
}
void intHARDWARE(){
	lcd_init(none);
	initUI();
	ip_init(inputPanelCallBack);
	initSmsClient();

	ip_enable();

	//fill message buffer
	//allow user to move and delete message
	// make sure it does not behave like school solution and crash

}
//self testing main mabye better in its own file
int main(int argc, char **argv) {

	tx_kernel_enter();
	return 0;

}
void tx_application_define(void *first_unused_memory) {
	/* Create the event flags. */
	intHARDWARE();
	status=timer0_register(1,true,none);
	//GUI_thread
	status=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&guistack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//PingThread
	//	status=tx_thread_create(&PingThread, "PingThread", pingLoop, inputText,&Pingstack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//reciveThread
	status=tx_thread_create(&receiveThread, "NetworkReceiveThread", sendReceiveLoop, inputText,&receiveThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
//	status=tx_thread_create(&sendThread, "NetworkSendThread", sendLoop, inputText,&sendThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//	if (status != TX_SUCCESS)printf("adc %d",status);
	//		status=tx_thread_create(&NetworkReciveThread, "NetworkReciveThread", NetworkInit, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
	status=tx_queue_create(&receiveQueue, "receiveQueue", TX_1_ULONG, &receiveQueueStack, QUEUE_SIZE*sizeof(ULONG));
	status=tx_queue_create(&ToSendQueue, "ToSendQueue", TX_1_ULONG, &sendQueueStack, QUEUE_SIZE*sizeof(ULONG));
	//	status = tx_timer_create(&my_timer,"my_timer_name",ping, 0x0, 5, 5,TX_AUTO_ACTIVATE);
	//	status = tx_timer_activate(&my_timer);
	//		printf("status %d",status);
}
