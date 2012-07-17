;/* Define the ARC Xtimer registers. */
;
.extAuxRegister aux_timer, 0x21, r|w
.extAuxRegister aux_tcontrol, 0x22, r|w
.extAuxRegister aux_tlimit, 0x23, r|w
.equ MY_TIMER_INT_ENABLE, 0x3
.equ MY_TIMER_INT_VALUE, 50000 ;millisecond
.equ TIMER_INT_VALUE, 50000 ;millisecond



.seg "INTERRUPT_TABLE", text
.global InterruptTable
InterruptTable:
       FLAG 0                     ; IRQ 0
       b      _start
       jal    MEM_ERROR_ISR       ; IRQ 1
       jal    BAD_INSTRUCTION_ISR ; IRQ 2
       jal    tx_timer_interrupt_3; IRQ 3
       jal    NO_HANDLER          ; IRQ 4
       jal    NO_HANDLER     ; IRQ 5
       jal    NO_HANDLER          ; IRQ 6
       jal    NO_HANDLER   		  ; IRQ 7
       jal    NO_HANDLER	      ; IRQ 8
       jal    INPUT_PANEL      ; IRQ 9
       jal    NO_HANDLER	      ; IRQ 10
       jal    NO_HANDLER	      ; IRQ 11
       jal    NO_HANDLER	      ; IRQ 12
       jal    NO_HANDLER	      ; IRQ 13
       jal    NETWORK	      ; IRQ 14
       jal    LCD	      ; IRQ 15

NO_HANDLER:
MEM_ERROR_ISR:
BAD_INSTRUCTION_ISR:
       flag   1
       nop
       nop
       nop
tx_timer_interrupt_3:
		sub sp, sp, 172 ; Allocate an interrupt stack frame
		st r0, [sp, 0] ; Save r0
		st r1, [sp, 4] ; Save r1
		st r2, [sp, 8] ; Save r2
		sr MY_TIMER_INT_VALUE, [aux_tlimit] ; Setup timer count
		sr MY_TIMER_INT_ENABLE, [aux_tcontrol] ; Enable timer interrupts
		; End of target specific timer interrupt handling.
		; Jump to generic ThreadX timer interrupt handler
		b _tx_timer_interrupt
FLASH:
	sub sp,sp,172; Allocate an interrupt stack frame
	st blink, [sp,16]; Save blink (
	bl _tx_thread_context_save; Save interrupt context
	sub sp,sp,16; Allocate 16 bytes of stack space
	;bl flash_interrupt
	add sp,sp,16; Recover 16 bytes of stack space
	jal _tx_thread_context_restore; Restore interrupt context
INPUT_PANEL:
	sub sp,sp,172; Allocate an interrupt stack frame
	st blink, [sp,16]; Save blink (
	bl _tx_thread_context_save; Save interrupt context
	sub sp,sp,16; Allocate 16 bytes of stack space
	bl InputPanel_ISR
	add sp,sp,16; Recover 16 bytes of stack space
	jal _tx_thread_context_restore; Restore interrupt context

NETWORK:
	sub sp,sp,172; Allocate an interrupt stack frame
	st blink, [sp,16]; Save blink (
	bl _tx_thread_context_save; Save interrupt context
	sub sp,sp,16; Allocate 16 bytes of stack space
	bl network_ISR
	add sp,sp,16; Recover 16 bytes of stack space
	jal _tx_thread_context_restore; Restore interrupt context

LCD:
	sub sp,sp,172; Allocate an interrupt stack frame
	st blink, [sp,16]; Save blink (
	bl _tx_thread_context_save; Save interrupt context
	sub sp,sp,16; Allocate 16 bytes of stack space
	bl lcd_done; Call an ISR written in C
	add sp,sp,16; Recover 16 bytes of stack space
	jal _tx_thread_context_restore; Restore interrupt context
TIMER1:
	sub sp,sp,172; Allocate an interrupt stack frame
	st blink, [sp,16]; Save blink (
	bl _tx_thread_context_save; Save interrupt context
	sub sp,sp,16; Allocate 16 bytes of stack space
	;bl timer1ISR; Call an ISR written in C
	add sp,sp,16; Recover 16 bytes of stack space
	jal _tx_thread_context_restore; Restore interrupt context
