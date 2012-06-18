/*
 * input_panel.h
 *
 * Descriptor: API of the input panel.
 *
 */


#include "input_panel.h"


// list of buttons that can be pressed on the input panel.
#define PADIER (0x1E1)
#define PADSTAT (0x1E0)
#define INTRRUPTS_ON (0x1fff)
#define INTRRUPTS_OFF (0x0)
void (*cb_func)(Button);
/**********************************************************************
 *
 * Function:    ip_init
 *
 * Descriptor:  Initialize the input panel.
 *
 * Parameters:  button_pressed_cb: call back whenever a button was pressed.
 *
 * Notes:       This device should work by default with no interrupts enabled.
 *
 * Return:      OPERATION_SUCCESS:      Initialization done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t ip_init(void (*button_pressed_cb)(button)){
	if (button_pressed_cb==NULL)return NULL_POINTER;
	cb_func=button_pressed_cb;
	return OPERATION_SUCCESS;
}
/**********************************************************************
 *
 * Function:    ip_enable
 *
 * Descriptor:  Enable interrupts when a button pressed.
 *
 * Notes:
 *
 * Return:
 *
 ***********************************************************************/
void ip_enable(){
	_sr(INTRRUPTS_ON,PADIER);
}

/**********************************************************************
 *
 * Function:    ip_disable
 *
 * Descriptor:  Disable interrupts when a button pressed.
 *
 * Notes:
 *
 * Return:
 *
 ***********************************************************************/
void ip_disable(){
	_sr(INTRRUPTS_OFF,PADIER);
}
 void InputPanel_ISR(){
	button mybutton=(Button)_lr(PADSTAT);
	cb_func(mybutton);
}
