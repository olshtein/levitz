/*
 * input_panel.h
 *
 * Descriptor: API of the LCD screen.
 *
 */

#ifndef LCD_H_
#define LCD_H_
#include "tebahpla.h"





/**********************************************************************
 *
 * Function:    lcd_init
 *
 * Descriptor:  Initialize the LCD screen.
 *
 * Parameters:  flush_complete_cb: call back whenever flush request was done.
 *
 * Notes:       Default character during initialization should be ' ' (0x20).
 *
 * Return:      OPERATION_SUCCESS:      Initialization successfully done.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t lcd_init(void (*flush_complete_cb)(void));

/**********************************************************************
 *
 * Function:    lcd_set_row
 *
 * Descriptor:  Write "length" character that find in the given buffer "line"
 *              start from the beginning of line "row_number".
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Write request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *              NOT_READY:              The device is not ready for a new request.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *
 ***********************************************************************/
result_t lcd_set_row(uint8_t row_number, bool selected, char const line[], uint8_t length);
/**********************************************************************
 *
 * Function:    lcd_set_new_buffer
 *
 * Descriptor:  Write the full screen buffer to lcd screen
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Write request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *              NOT_READY:              The device is not ready for a new request.
 *
 ***********************************************************************/

result_t lcd_set_new_buffer(ScreenBuffer* sb);
#endif /* LCD_H_ */
