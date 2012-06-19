/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "tx_api.h"
#define FIRST_LINE (0)
#define SECOND_LINE (1)
#define BOTTOM_LINE (LCD_LINE_LENGTH-1)
#define PRINT_SCREEN while(lcd_set_new_buffer(&screenBuffer)!=OPERATION_SUCCESS);
//#define MILISECOND (50000)
//#define TIME_TO_MOVE_CURSOR (MILISECOND*1000/120) //half a second
#define TIME_TO_MOVE_CURSOR (10) //half a second
typedef enum state{
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;
CHARACTER message1[]={206,229,247,160,160,160,196,229,236,229,244,229,176};
CHARACTER message2[]={194,225,227,235,160,160,196,229,236,229,244,229,176};

Message toSend;
MessagesBuffer messages;
ScreenBuffer screenBuffer;
volatile State curState;

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
	case MESSAGE_WRITE_TEXT:
	case MESSAGE_WRITE_NUMBER:
		memcpy(line,message2,LCD_LINE_LENGTH);
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
	return i;
}

int showTimeRecived(CHARACTER * line){
	int i=0;
	for(;i<TIME_STAMP_DIGITS;i++){
		if(messages.Messages[messages.currentMessage].inOrOut==IN){

			*line++=getCHAR((messages.Messages[messages.currentMessage]).timeStamp[i],true);
		}
		else *line++=EMPTY;
	}
	for(;i<LCD_LINE_LENGTH;i++){
		if(messages.Messages[messages.currentMessage].inOrOut==IN){

			*line++=getCHAR(' ',true);
		}
		else *line++=EMPTY;

	}
	return i;
}
int showMessageContent(CHARACTER * line){
	int i=0;
	for(;i<messages.Messages[messages.currentMessage].size;i++){
		*line++=getCHAR(messages.Messages[messages.currentMessage].content[i],false);
	}
	for(;i<LCD_TOTAL_CHARS-3*LCD_LINE_LENGTH;i++){
		*line++=EMPTY;
	}

	return i;
}

void showMessage(){
	//	ScreenBuffer * sc=&(screenBuffer);
	//	CHARACTER* line1=sc->buffer;
	int i;
	for (i=0;i<NUMBER_DIGTS;i++){
		screenBuffer.buffer[i]=getCHAR(messages.Messages[messages.currentMessage].numberFromTo[i],true);
	}// show number
	for(;i<LCD_LINE_LENGTH;i++){
		screenBuffer.buffer[i]=getCHAR(' ',true);
	}//fill the line

	for(i=0;i<TIME_STAMP_DIGITS;i++){
		if(messages.Messages[messages.currentMessage].inOrOut==IN){
			screenBuffer.buffer[i+LCD_LINE_LENGTH]=getCHAR((messages.Messages[messages.currentMessage]).timeStamp[i],true);
		}
		else screenBuffer.buffer[i+LCD_LINE_LENGTH]=EMPTY;
	}// show time stamp
	for(;i<LCD_LINE_LENGTH;i++){
		if(messages.Messages[messages.currentMessage].inOrOut==IN){
			screenBuffer.buffer[i+LCD_LINE_LENGTH]=getCHAR(' ',true);
		}
		else screenBuffer.buffer[i+LCD_LINE_LENGTH]=EMPTY;
	}//fill the line

	for(i=0;i<messages.Messages[messages.currentMessage].size;i++){
		screenBuffer.buffer[i+2*LCD_LINE_LENGTH]=getCHAR(messages.Messages[messages.currentMessage].content[i],false);
	} // show the message content
	for(;i<LCD_TOTAL_CHARS-3*LCD_LINE_LENGTH;i++){
		screenBuffer.buffer[i+2*LCD_LINE_LENGTH]=EMPTY;
	}// fill empty

	//	line1+=showMessageSource(line1);
	//	line1+=showTimeRecived(line1);
	//	line1+=showMessageContent(line1);
	menuLine(curState,/*line1);*/&screenBuffer.buffer[LCD_TOTAL_CHARS-LCD_LINE_LENGTH]); // show bottom line message
	PRINT_SCREEN;
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
	PRINT_SCREEN;
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
//void goUp(){
//}
//void goDown(){
//}
//void deleteMess(){
//
//}
volatile Button currButton;
volatile int numOfTimes;
volatile char currChar ='*';
#define MOVE_CURSOR_INTERVAL (1)
//volatile int timer;

char button1[]=".,?1";
char button2[]="abc2";
char button3[]="def3";
char button4[]="ghi4";
char button5[]="jkl5";
char button6[]="mno6";
char button7[]="pqrs7";
char button8[]="tuv8";
char button9[]="wxyz9";
char button0[]=" 0";
char getLetter(Button button,int numOftimes){
	//
	if(button== BUTTON_1){
		return button1[numOftimes%4];
	}
	if(button== BUTTON_2){
		return button2[numOftimes%4];
	}
	if(button== BUTTON_3){
		return button3[numOftimes%4];
	}
	if(button== BUTTON_4){
		return button4[numOftimes%4];
	}
	if(button== BUTTON_5){
		return button5[numOftimes%4];
	}
	if(button== BUTTON_6){
		return button6[numOftimes%4];
	}
	if(button== BUTTON_7){
		return button7[numOftimes%5];
	}
	if(button== BUTTON_8){
		return button8[numOftimes%4];
	}
	if(button== BUTTON_9){
		return button9[numOftimes%5];
	}
	if(button== BUTTON_0){
		return button0[numOftimes%2];
	}
	if(button== BUTTON_NUMBER_SIGN){
		return ' ';
	};
	return ']';
}
void createNewMessage(){
	toSend.inOrOut=OUT;
	toSend.size=-1;
	currButton=BUTTON_STAR;
	//	messageEditMoveCursor();
	for(int i=0;i<LCD_TOTAL_CHARS-LCD_LINE_LENGTH;i++)screenBuffer.buffer[i]=EMPTY;
	ScreenBuffer * s2=&screenBuffer;
	CHARACTER* line2=s2->buffer+LCD_TOTAL_CHARS-LCD_LINE_LENGTH;
	menuLine(curState,line2);
	PRINT_SCREEN;
}
ULONG lastTime=0;
void writeLetter(Button button){
	ULONG current_time= tx_time_get();

	if(button!=currButton || (current_time-lastTime)>TIME_TO_MOVE_CURSOR/*&& clock jump*/){
		numOfTimes=0;
		currButton=button;
		toSend.size++;
	}
	else{
		numOfTimes++;
	}
	lastTime=current_time;
	//reset clock
	currChar=getLetter(currButton,numOfTimes);
	screenBuffer.buffer[toSend.size]=getCHAR(currChar,false);
	toSend.content[toSend.size]=currChar;
	//	//TODO show
	PRINT_SCREEN;
}
int newMessageNumberPos;
createNewMessageNumber(){
	newMessageNumberPos=-1;
	currButton=BUTTON_STAR;
	//	messageEditMoveCursor();
	for(int i=0;i<LCD_TOTAL_CHARS-LCD_LINE_LENGTH;i++)screenBuffer.buffer[i]=EMPTY;
	ScreenBuffer * s2=&screenBuffer;
	CHARACTER* line2=s2->buffer+LCD_TOTAL_CHARS-LCD_LINE_LENGTH;
	menuLine(curState,line2);
	PRINT_SCREEN;

}
void writeDigit(Button button){
//	if(button!=currButton /*&& clock jump*/){
		currButton=button;
		if(newMessageNumberPos<NUMBER_DIGTS-1)newMessageNumberPos++;
//	}
	//reset clock
	int num=3;
	if(currButton==BUTTON_7 || currButton==BUTTON_9) num=4;
	currChar=getLetter(currButton,num);
	screenBuffer.buffer[newMessageNumberPos]=getCHAR(currChar,false);
	toSend.numberFromTo[newMessageNumberPos]=currChar;

	//	//TODO show
	PRINT_SCREEN;
}

void inputPanelCallBack(Button button ){
	switch (curState){
	case MESSAGE_LIST:
		if (button==BUTTON_STAR){
			curState=MESSAGE_WRITE_TEXT;
			createNewMessage();
		}
		else if(messages.size>0){//2,8 up down in screen, # to delete
			if( button==BUTTON_OK){
				curState=MESSAGE_SHOW;
				showMessage();

			}
			else{
				if( button==BUTTON_2){//goUp();
					if(messages.currentMessage==messages.topMessage && (messages.size>=LCD_NUM_LINES)){
						messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
						messages.topMessage=messages.currentMessage;
					}
					else messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
				}
				if( button==BUTTON_8){//goDown();
					if(messages.currentMessage==(messages.topMessage+LCD_NUM_LINES-2)%messages.size && (messages.size>=LCD_NUM_LINES)){
						messages.topMessage=(messages.topMessage+1)%messages.size;
					}
					messages.currentMessage=((messages.currentMessage+1)%messages.size);

				}
				if( button==BUTTON_NUMBER_SIGN){//deleteMess();
					messages.size--;
					for (int i=messages.currentMessage;i<messages.size;i++){
						messages.Messages[i]=messages.Messages[i+1];
					}
					if(messages.currentMessage==messages.size){
						messages.topMessage=0;
						messages.currentMessage=0;
					}
				}
				showListScreen(0);
			}
		}

		break;
	case MESSAGE_SHOW:
		if (button==BUTTON_STAR){

			curState=MESSAGE_LIST;
			showListScreen(0);
		}
		if (button==BUTTON_NUMBER_SIGN){
			//			deleteMessage();
			curState=MESSAGE_LIST;
			messages.size--;
			for (int i=messages.currentMessage;i<messages.size;i++){
				messages.Messages[i]=messages.Messages[i+1];
			}
			if(messages.currentMessage==messages.size){
				messages.topMessage=0;
				messages.currentMessage=0;
			}
			showListScreen(0);

		}
		break;
	case MESSAGE_WRITE_TEXT:
		if(button==BUTTON_NUMBER_SIGN){
			if(toSend.size>=0){
				screenBuffer.buffer[toSend.size]=EMPTY;
				currButton=BUTTON_STAR;
				toSend.size--;
				PRINT_SCREEN;
			}
		}
		else if (button==BUTTON_STAR){

			curState=MESSAGE_LIST;
			showListScreen(0);
		}
		else if( button==BUTTON_OK){
			curState=MESSAGE_WRITE_NUMBER;
			createNewMessageNumber();
			//TODO refresh screen
		}
		else writeLetter(button);
		break;
	case MESSAGE_WRITE_NUMBER:
		if(button==BUTTON_NUMBER_SIGN){
			if(newMessageNumberPos>=0){
				screenBuffer.buffer[newMessageNumberPos]=EMPTY;
				currButton=BUTTON_STAR;
				newMessageNumberPos--;
				PRINT_SCREEN;
			}
		}
		else if (button==BUTTON_STAR){

			curState=MESSAGE_LIST;
			showListScreen(0);
		}
		else if( button==BUTTON_OK){
			toSend.size++;
			for(int i=newMessageNumberPos+1;i<NUMBER_DIGTS;i++){
				toSend.numberFromTo[i]=' ';
			}// fill the number with ' '
			if(messages.size==MAX_MESSAGES) messages.size--;
			addMessage(toSend);
			curState=MESSAGE_LIST;
			//TODO sendToNetwrok
			showListScreen(0);

		}
		else writeDigit(button);
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




