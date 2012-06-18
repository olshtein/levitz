/*
 * DisplayMessageList.h
 *
 *  Created on: Jun 11, 2012
 *      Author: issarh
 */

#ifndef DISPLAYMESSAGELIST_H_
#define DISPLAYMESSAGELIST_H_
#include "LCD.h"
ScreenBuffer getCurrentListScreenBuffer();
ScreenBuffer getInput(keyPressed);
ScreenBuffer newMessageArrived();
// private
ScreenBuffer messageDown();
ScreenBuffer messageUp();
int getCurrentMessage();
ScreenBuffer createMessage();
ScreenBuffer deleteMessage();
#endif /* DISPLAYMESSAGELIST_H_ */
