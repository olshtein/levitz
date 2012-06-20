/*
 * ex3.c
 *
 *  Created on: Jun 11, 2012
 *      Author: levio01
 */
//#include "tx_port.h"
#include "LCD.h"
#include "tx_api.h"
#include "timer.h"
#include "UI.h"
#include "input_panel.h"
#include "tx_port.h"
#include "network.h"
//typedef unsigned long                           ULONG;
//#include "string.h"
//#include "stdio.h"

TX_THREAD thread_0;
TX_THREAD thread_1;
TX_QUEUE queue_0;
TX_EVENT_FLAGS_GROUP event_flags_0;
//TX_BYTE_POOL byte_pool_0;
//TX_BLOCK_POOL block_pool_0;
void threadStartMethod1(ULONG string);
void threadStartMethod2(ULONG string);
int kl=0;
void none(){
	//	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE%d\n",kl++);
}
void intHARDWARE(){
	lcd_init(none);
	initUI();
	ip_init(inputPanelCallBack);
	network_init(NULL);
	ip_enable();

}
int main(int argc, char **argv) {
	intHARDWARE();
	tx_kernel_enter();
	return 0;
}
void threadStartMethod1(ULONG s){
	for(int i=0;i>-1;i++){
		if (i%2000==0){
			//			printf("%d 11\n",s);
			/* Set event flag 0 to wakeup thread . */
			//			int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
			tx_thread_sleep(1);
		}
	}
}

void threadStartMethod2(ULONG s){
	for(int i=0;i>-1;i++){
		if (i%1000==0){
			ULONG actual_flags;
			//			int status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
			//			if ((status != TX_SUCCESS) || (actual_flags != 0x1))break;
			tx_thread_sleep(1);
			//			printf("%d 22\n",s);
		}
	}
}
int status;
ULONG inputText=16;
char stack1[STACK_SIZE];
char stack2[STACK_SIZE];
void tx_application_define(void *first_unused_memory) {

	status=timer0_register(1,true,none);

	status=tx_thread_create(&thread_1, "Thread2", threadStartMethod2, inputText,&stack2, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	status=tx_thread_create(&thread_0, "_Thread1", threadStartMethod1, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
	status=tx_event_flags_create(&event_flags_0, "event flags 0");
	//	_enable();
}
