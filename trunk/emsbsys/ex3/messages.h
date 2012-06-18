/*
 * messages.h
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_
#define NUMBER_DIGTS (8)
#define MESSAGE_SIZE (160)
typedef enum InOrOut{
	IN='I',
			OUT='O'
}InOrOut;
#pragma pack(1)
typedef struct message{
	char numberFromTo[NUMBER_DIGTS];//cannot be int 052->52 etc.
	InOrOut inOrOut;
	char content[MESSAGE_SIZE];
}Message;
#pragma pack()

typedef struct messagesBuffer{
	Message Messages[100];
 int size; // num of messages
	int currentMessage; // the selected message (-1 if none selected)
	int topMessage;// the top message
}MessagesBuffer;


#endif /* MESSAGES_H_ */
