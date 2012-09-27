/*
 * UiTester.c
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */

#include "tx_api.h"
//#include "flash.h"
#include "UI.h"
#include "messages.h"
#include "input_panel.h"
#include "timer.h"
#include "smsClient.h"
//#include "fs.h"
#define QUEUE_SIZE (SEND_LIST_SIZE)

TX_QUEUE receiveQueue; // the receive for the smsClient messages queue
TX_QUEUE ToSendQueue; // the to send for the smsClient messages queue

int status;

TX_THREAD receiveThread; // the receive Thread
TX_THREAD GUI_thread; // the gui thread
TX_THREAD FS_thread; // the fs thread

char guistack[STACK_SIZE];
char receiveThreadStack[STACK_SIZE];
//char fsThreadStack[STACK_SIZE];
ULONG receiveQueueStack[QUEUE_SIZE];
ULONG sendQueueStack[QUEUE_SIZE];
ULONG inputText=16;
void fs_wakeup();

void none(){} // none method

/*
 * initalaize the Hardware
 */
int intHARDWARE(){
	lcd_init(none);
//	flash_init(flash_read_done_cb,fs_wakeup);
	initUI();
	ip_init(inputPanelCallBack);
	initSmsClient();
	ip_enable();
	return timer0_register(1,true,none); // set the timer for the first time

}


int main(int argc, char **argv) {
	tx_kernel_enter();
	return 0;

}

void tx_application_define(void *first_unused_memory) {
	status=intHARDWARE();

	//GUI_thread
	status+=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&guistack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

	//NetworkThread
	status+=tx_thread_create(&receiveThread, "NetworkReceiveThread", sendReceiveLoop, inputText,&receiveThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);

	//create recive and send queue
	status=tx_queue_create(&receiveQueue, "receiveQueue", TX_1_ULONG, &receiveQueueStack, QUEUE_SIZE*sizeof(ULONG));
	status=tx_queue_create(&ToSendQueue, "ToSendQueue", TX_1_ULONG, &sendQueueStack, QUEUE_SIZE*sizeof(ULONG));

}
