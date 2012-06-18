/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "timer.h"
#define FIRST_LINE (0)
#define SECOND_LINE (1)
#define BOTTOM_LINE (LCD_LINE_LENGTH-1)
typedef enum state{
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;
CHARACTER message1[]={206,229,247,160,160,160,196,229,236,229,244,229,176};
MessagesBuffer messages;
ScreenBuffer screenBuffer;
State curState;

void emptyLine(CHARACTER * line){
	for (int i=0;i<LCD_LINE_LENGTH;i++){
		*line=EMPTY;
		line++;
	}
}
void menuLine(State state,CHARACTER * line){
	switch (state) {
	case MESSAGE_LIST:
		memcpy(line,message1,LCD_LINE_LENGTH);
		break;
	case MESSAGE_SHOW:

		break;
	case MESSAGE_WRITE_TEXT:

		break;
	case MESSAGE_WRITE_NUMBER:

		break;
	default:
		break;
	}
}
int drawNum(CHARACTER * line,int messageNumber, bool selected){
	for (int i=0;i<NUMBER_DIGTS;i++){
		*(line++)=getCHAR(messages.Messages[messageNumber].numberFromTo[i],selected);
	}
	return NUMBER_DIGTS;

}
void getMessage(int messageNumber,CHARACTER * line){
	bool selected=(messageNumber==messages.currentMessage);
	*(line++)=getCHAR((char)('0'+messageNumber/10),selected);
	*(line++)=getCHAR((char)('0'+messageNumber%10),selected);
	*(line++)=getCHAR(' ',selected);
	line+=drawNum(line,messageNumber,selected);
	*(line)=getCHAR(messages.Messages[messageNumber].inOrOut,selected);
}
int showMessageSource(CHARACTER * line){
	for(int i=drawNum(line,messages.currentMessage,true);i<LCD_LINE_LENGTH;i++){
		*(line+i)=getCHAR(' ',true);
	}
	return LCD_LINE_LENGTH;
}

int showTimeRecived(CHARACTER * line){
	for(int i=0;i<TIME_STAMP_DIGITS;i++){
		if(messages.Messages[messages.currentMessage].inOrOut==IN){

			*line++=getCHAR((messages.Messages[messages.currentMessage]).timeStamp[i],true);
		}
		else *line++=EMPTY;
	}
	return TIME_STAMP_DIGITS;
}
int showMessageContent(CHARACTER * line){
	int i=0;
	for(i=0;i<messages.Messages[messages.currentMessage].size;i++){
		*line++=getCHAR(messages.Messages[messages.currentMessage].content[i],false);
	}
	for(;i<LCD_TOTAL_CHARS-3*LCD_LINE_LENGTH;i++){
		*line++=EMPTY;
	}

	return i;
}

void showMessage(){
	CHARACTER * line=screenBuffer.buffer;
	line+=showMessageSource(line);
	line+=showTimeRecived(line);
	line+=showMessageContent(line);
	while(lcd_set_new_buffer(&screenBuffer)!=OPERATION_SUCCESS);;
}
void showListScreen(ULONG a){
	CHARACTER* line=screenBuffer.buffer;
	for (int i=0; i<LCD_NUM_LINES-1;i++){
		if (i<messages.size){
			getMessage((messages.topMessage+i)%messages.size,line);

		}
		else {
			emptyLine(line);
		}
		line+=LCD_LINE_LENGTH;

	}
	menuLine(curState,line);
	while(lcd_set_new_buffer(&screenBuffer)!=OPERATION_SUCCESS);;
}
int l=0;
void noneUI(){
	//	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE%d\n",l++);
}

void initUI(){
	messages.size=0;
	messages.currentMessage=0;
	messages.topMessage=0;
	curState=MESSAGE_LIST;
	//	lcd_init(noneUI);
}
void goUp(){
	if(messages.currentMessage==messages.topMessage && (messages.size>=LCD_NUM_LINES)){
		messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
		messages.topMessage=messages.currentMessage;
	}
	else messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
}
void goDown(){
	if(messages.currentMessage==(messages.topMessage+LCD_NUM_LINES-2)%messages.size && (messages.size>=LCD_NUM_LINES)){
		messages.topMessage=(messages.topMessage+1)%messages.size;
	}
	messages.currentMessage=((messages.currentMessage+1)%messages.size);
}
void deleteMess(){
	messages.size--;
	for (int i=messages.currentMessage;i<messages.size;i++){
		messages.Messages[i]=messages.Messages[i+1];
	}
	if(messages.currentMessage==messages.size){
		messages.topMessage=0;
		messages.currentMessage=0;
	}
}

void inputPanelCallBack(Button button ){
	switch (curState){
	case MESSAGE_LIST:
		if (button==BUTTON_STAR){
			//			createMessage();
			curState=MESSAGE_WRITE_TEXT;
			//TODO refresh screen
		}
		else if(messages.size>0){//2,8 up down in screen, # to delete
			if( button==BUTTON_OK){
				curState=MESSAGE_SHOW;
				showMessage();

			}
			else{
				if( button==BUTTON_2)goUp();
				if( button==BUTTON_8)goDown();
				if( button==BUTTON_NUMBER_SIGN)deleteMess();
				showListScreen(0);
			}
		}

		break;
	case MESSAGE_SHOW:
		if (button==BUTTON_STAR){

			curState=MESSAGE_LIST;
			//TODO refresh screen
		}
		if (button==BUTTON_STAR){
			//			deleteMessage();
			curState=MESSAGE_LIST;

		}
		break;
	case MESSAGE_WRITE_TEXT:
		if (button==BUTTON_STAR){
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
		}
		if( button==BUTTON_OK){
			curState=MESSAGE_WRITE_NUMBER;

			//TODO refresh screen
		}
		break;
	case MESSAGE_WRITE_NUMBER:
		if( button==BUTTON_OK){
			//TODO send message and add to message buffer
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
			//TODO refresh screen
		}
		if (button==BUTTON_STAR){//cancel
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
		}
		break;
	default:
		//should happen
		break;
	}
}


void addMessage(Message m){
	memcpy(&messages.Messages[messages.size],&m,sizeof(Message));
	messages.size++;
}




