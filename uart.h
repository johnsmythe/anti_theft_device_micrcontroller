#ifndef __UART_H__
#define __UART_H__
	void configureUCA0(void);
	void configureUCA1(void);
	void uart_send_char( unsigned char );
	void uart_send_string( const char * );
#endif

