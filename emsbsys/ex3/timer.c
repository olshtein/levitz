
#include "timer.h"
//#include <stdio.h>//-----------------------------------------------------------------------------------

#define NULL (0)
# define COUNT0_REG (0x21)
# define CONTROL0_REG (0x22)
# define LIMIT0_REG (0x23)
# define COUNT1_REG (0x100)
# define CONTROL1_REG (0x101)
# define LIMIT1_REG (0x102)
// control register's byte
# define INTEREUPT_ENABLE_FLAG (0x01) //is interrupt will be generated after the timer has reached its limit condition.
# define NONE_HALT_MODE (0x02)// is cycles counted only when the processor isn't halted
# define WATCHDOG_MODE (0x04) // is Watchdog reset generation enabled
# define INTEREUPT_PENDING_FLAG (0x08) // is the value of the interrupt line is high - RO
#define CYCLES_PER_MILISECOND (50000) // number of cycles per millisecond


#define RESET_VALUE (0x0)
#define LIMIT_RESET_VALUE (0x00FFFFFF)


void(*my_timer0_cb)(void) ;
void(*my_timer1_cb)(void) ;
 bool my_isOneShot0,my_isOneShot1;


/**********************************************************************
 *
 * Function:    timer0_register
 *
 * Descriptor:  Set expiration interval from now in milliseconds
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Timer request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL.
 *
 ***********************************************************************/
result_t timer0_register(uint32_t interval, bool one_shot, void(*timer_cb)(void)){
	if(timer_cb==NULL) return NULL_POINTER;
	//	if(interval==0) return INVALID_ARGUMENTS;
	_sr(RESET_VALUE,CONTROL0_REG);
	my_timer0_cb=timer_cb;
	my_isOneShot0=one_shot;
	_sr(RESET_VALUE,COUNT0_REG);
	_sr(LIMIT_RESET_VALUE,LIMIT0_REG);
	_sr(CYCLES_PER_MILISECOND*interval,LIMIT0_REG);
	_sr(INTEREUPT_ENABLE_FLAG,CONTROL0_REG);
	return OPERATION_SUCCESS;
}

/**********************************************************************
 *
 * Function:    timer1_register
 *
 * Descriptor:  Set expiration interval from now in milliseconds
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Timer request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL.
 *
 ***********************************************************************/
result_t timer1_register(uint32_t interval, bool one_shot, void(*timer_cb)(void)){
	if(timer_cb==NULL) return NULL_POINTER;
	//	if(interval==0) return INVALID_ARGUMENTS;
	_sr(RESET_VALUE,CONTROL1_REG);
	my_timer1_cb=timer_cb;
	my_isOneShot1=one_shot;
	_sr(RESET_VALUE,COUNT1_REG);
	_sr(LIMIT_RESET_VALUE,LIMIT1_REG);
	_sr(CYCLES_PER_MILISECOND*interval,LIMIT1_REG);
	_sr(INTEREUPT_ENABLE_FLAG,CONTROL1_REG);
	return OPERATION_SUCCESS;
}

/*_Interrupt1*/ void timer0ISR()
{
	/* acknowledge the interrupt - disable the interrupt flag*/
	unsigned control0val=_lr(CONTROL0_REG);
	_sr((control0val&(~INTEREUPT_ENABLE_FLAG)),CONTROL0_REG);
	/* reset the interrupt flag if _oneShot=1 */
	if(!my_isOneShot0){
		_sr((control0val|INTEREUPT_ENABLE_FLAG),CONTROL0_REG);
	}

	my_timer0_cb();
}

/*_Interrupt2 void */timer1ISR()
{
	/* acknowledge the interrupt - disable the interrupt flag*/
		unsigned control1val=_lr(CONTROL1_REG);
		_sr((control1val&(~INTEREUPT_ENABLE_FLAG)),CONTROL1_REG);
		/* reset the interrupt flag if _oneShot=1 */
		if(!my_isOneShot1){
			_sr((control1val|INTEREUPT_ENABLE_FLAG),CONTROL1_REG);
		}

		my_timer1_cb();
}

