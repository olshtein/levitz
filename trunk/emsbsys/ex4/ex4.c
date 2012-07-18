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
#include "fs.h"
#define QUEUE_SIZE (SEND_LIST_SIZE)

TX_QUEUE receiveQueue; // the receive for the smsClient messages queue
TX_QUEUE ToSendQueue; // the to send for the smsClient messages queue

int status;

TX_THREAD receiveThread; // the receive Thread
TX_THREAD sendThread; // the send Thread
TX_THREAD GUI_thread; // the gui thread
TX_THREAD FS_thread; // the fs thread

char guistack[STACK_SIZE];
char receiveThreadStack[STACK_SIZE];
char fsThreadStack[STACK_SIZE];
ULONG receiveQueueStack[QUEUE_SIZE];
ULONG sendQueueStack[QUEUE_SIZE];
ULONG inputText=16;


void none(){} // none method

/*
 * initalaize the Hardware
 */
int intHARDWARE(){
	lcd_init(none);
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
void fsMain(ULONG filename){
	int headerLoc=0;
	FS_SETTINGS fs_set;
	fs_set.block_count=16;
	FS_STATUS status =fs_init(fs_set);
	if(status!=SUCCESS){
		headerLoc=(int) status;
		headerLoc*=18;
	}
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

	// FS_thread
	tx_thread_create(&FS_thread, "FS_thread", fsMain, inputText,&fsThreadStack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
}
