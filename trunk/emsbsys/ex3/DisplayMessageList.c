/*
 * DisplayMessageList.cpp
 *
 *  Created on: Jun 11, 2012
 *      Author: issarh
 */

#include "DisplayMessageList.h"
enum InOrOut{
	IN='I',
	OUT='O'
}InOrOut;
typedef struct message{
	int MessageNumber;
	char  numberFrom[8];//cannot be int 052->52 etc.
	char * content[160];
	InOrOut inOrOut;
}Message;
typedef struct messageBuffer{
	Message Messages[100];
	int currentMessage;
}MessageBuffer;

