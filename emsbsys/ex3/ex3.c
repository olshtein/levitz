/*
 * ex3.c
 *
 *  Created on: Jun 11, 2012
 *      Author: levio01
 */
#include "tx_port.h"
#include "tx_api.h"
//#include "LCD.h"
#include "timer.h"
//#include <stdio.h>
#define STACK_SIZE 0x1000
#define DEMO_BYTE_POOL_SIZE 0x10000

TX_THREAD thread_0;
TX_THREAD thread_1;
//TX_THREAD thread_2;
//TX_THREAD thread_3;
//TX_THREAD thread_4;
//TX_THREAD thread_5;
//TX_THREAD thread_6;
//TX_THREAD thread_7;
//TX_QUEUE queue_0;
//TX_SEMAPHORE semaphore_0;
//TX_MUTEX mutex_0;
TX_EVENT_FLAGS_GROUP event_flags_0;
TX_BYTE_POOL byte_pool_0;
TX_BLOCK_POOL block_pool_0;
void threadStartMethod1(ULONG string);
void threadStartMethod2(ULONG string);
int kl=0;
void none(){
	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE%d\n",kl++);
}
int main(int argc, char **argv) {
	tx_kernel_enter();
	return 0;
}
void threadStartMethod1(ULONG s){
	for(int i=0;i>-1;i++){
		if (i%2000==0){
			printf("%d 11\n",s);
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
			printf("%d 22\n",s);
		}
	}
}
int status;
	ULONG inputText=16;
	char stack1[STACK_SIZE];
	char stack2[STACK_SIZE];
void tx_application_define(void *first_unused_memory) {
	/* Create the event flags. */
	//	tx_event_flags_create(&gLaserEventFlags, "laser_event");
	//	tx_event_flags_create(&gRFEventFlags, "RF_event");
	/* Initialize the hardware. */
//	int status=tx_byte_pool_create(&byte_pool_0, "byte pool 0", first_unused_memory,DEMO_BYTE_POOL_SIZE);
//	status=tx_byte_allocate(&byte_pool_0, &stackPointer, STACK_SIZE, TX_NO_WAIT);
//	status=tx_byte_allocate(&byte_pool_0, &stackPointer, STACK_SIZE, TX_NO_WAIT);
	status=timer0_register(1,true,none);

	status=tx_thread_create(&thread_1, "Thread2", threadStartMethod2, inputText,&stack2, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	status=tx_thread_create(&thread_0, "_Thread1", threadStartMethod1, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
	status=tx_event_flags_create(&event_flags_0, "event flags 0");
//	_enable();
}
