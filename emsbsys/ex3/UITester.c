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
void mainloop(ULONG a);
char mess[]="abcdefghijklmnopqrst01234567890ABCDEFGHIJKLMNOPQRZabcdefghijklmnopqrst01234567890ABCDEFGHIJKLMNOPQRZ";
void none(){
	//	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEe\n");
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
	intHARDWARE();
	tx_kernel_enter();
	return 0;

}
int status;
TX_THREAD NetworkReceiveThread;
TX_THREAD GUI_thread;
TX_THREAD PingThread;
TX_TIMER my_timer;
char Guistack[STACK_SIZE];
char NetworkReceivestack[STACK_SIZE];
char Pingstack[STACK_SIZE];
ULONG receiveQueueStack[QUEUE_SIZE];
ULONG sendQueueStack[QUEUE_SIZE];
ULONG inputText=16;
int kk=0;
void addMessages(){
	Message m ;
	for (int i=0;i<2;i++){

		for (int j=0;j<i;j++){
			m.content[j]=(char)(j+40);
		}
		m.numberFromTo[0]=(char)('0'+i/10);
		m.numberFromTo[1]=(char)('0'+i%10);
		for (int k=0;k<6;k++){
			m.numberFromTo[k+2]=(char)(48+k);
		}
		if(i%2==0){
			m.inOrOut=IN;
			for(int k=0;k<TIME_STAMP_DIGITS;k++){
				m.timeStamp[k]='5';
			}
		}
		else m.inOrOut=OUT;
		m.size=(i*15+i)%MESSAGE_SIZE;
		memcpy(&m.content[0],&mess[0],m.size);
		addMessage(&m);
	}
}
void tx_application_define(void *first_unused_memory) {
	/* Create the event flags. */
	status=timer0_register(1,true,none);
	addMessages();
	//GUI_thread
	status=tx_thread_create(&GUI_thread, "GUI_thread", startUI, inputText,&Guistack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//PingThread
	//	status=tx_thread_create(&PingThread, "PingThread", pingLoop, inputText,&Pingstack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//reciveThread
	status=tx_thread_create(&NetworkReceiveThread, "NetworkReceiveThread", receiveLoop, inputText,&NetworkReceivestack, STACK_SIZE,	16, 16, 4, TX_AUTO_START);
	//	if (status != TX_SUCCESS)printf("adc %d",status);
	//		status=tx_thread_create(&NetworkReciveThread, "NetworkReciveThread", NetworkInit, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
	status=tx_queue_create(&receiveQueue, "receiveQueue", TX_1_ULONG, &receiveQueueStack, QUEUE_SIZE*sizeof(ULONG));
	status=tx_queue_create(&ToSendQueue, "ToSendQueue", TX_1_ULONG, &sendQueueStack, QUEUE_SIZE*sizeof(ULONG));
	//	status = tx_timer_create(&my_timer,"my_timer_name",ping, 0x0, 5, 5,TX_AUTO_ACTIVATE);
	//	status = tx_timer_activate(&my_timer);
	//		printf("status %d",status);
}
