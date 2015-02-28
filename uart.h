#ifndef __UART_H__
#define __UART_H__
	void configureUCA0(void);
	void configureUCA1(void);
	void uart_send_char( unsigned char );
	void uart_send_string( const char * );
	void disable_UCA0_interrupt( void );
	void enable_UCA0_interrupt( void );
	void disable_UCA1_interrupt( void );
	void enable_UCA1_interrupt ( void );
	void enable_UCA3_interrupt ( void );
	void disable_UCA3_interrupt ( void );
	//extern int bufferReady;
	extern char gsmBuf[1024];
#endif

