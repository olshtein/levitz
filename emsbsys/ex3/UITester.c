/*
 * UiTester.c
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */
#define STACK_SIZE (0x1000/5)
#include "UI.h"
#include "messages.h"
#include <stdio.h>
#include <string.h>
#include "timer.h"
void none(){
	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEe\n");
}
void intHARDWARE(){
	lcd_init(none);
	initUI();
		struct message m ;
	for (int i=0;i<99;i++){

		for (int j=0;j<i;j++){
			m.content[j]=(char)(j+40);
		}
		for (int k=0;k<8;k++){
			m.numberFromTo[k]=(char)(48+k);
		}
		if(i%2==0)m.inOrOut=IN;
		else m.inOrOut=OUT;
//		addMessage(m);
	}

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
TX_THREAD thread_0;
ULONG inputText=16;
	char stack1[STACK_SIZE];

void tx_application_define(void *first_unused_memory) {
	/* Create the event flags. */
	status=timer0_register(1,true,none);
	status=tx_thread_create(&thread_0, "_Thread1", mainloop, inputText,&stack1, STACK_SIZE,16, 16, 4, TX_AUTO_START);
}
