#include "uart.h"
#include <msp430.h>
#include <stdio.h>
#include <string.h>

//modulation parameters to get 9600 baud
unsigned divider = 104;
unsigned char fM = 1;
unsigned char sM = 0;

char gsmBuf[1024];
int bufIndex = 0;

void configureUCA0(){
    /* Place UCA0 in Reset to be configured */
    UCA0CTL1 = UCSWRST;

    /* Configure */
    UCA0CTL1 = UCSSEL1;
    UCA0BR0 = divider & 0xFF;
    UCA0BR1 = divider >> 8;
    UCA0MCTL = ( (fM & 0xF) << 4 |(sM & 0x7) << 1);

    /* Take UCA0 out of reset */
    UCA0CTL1 &= ~UCSWRST;

	//enable rx interrupt on UCA0, after taking it out of reset
	enable_UCA0_interrupt();
}

void configureUCA1(){
    /* Place UCA1 in Reset to be configured */
    UCA1CTL1 = UCSWRST;

    /* Configure */
    UCA1CTL1 = UCSSEL1;
    UCA1BR0 = divider & 0xFF;
    UCA1BR1 = divider >> 8;
    UCA1MCTL = ( (fM & 0xF) << 4 |(sM & 0x7) << 1);

    /* Take UCA1 out of reset */
    UCA1CTL1 &= ~UCSWRST;

	//enable rx interrup on UCA1
	enable_UCA1_interrupt();

}

void configureUCA3(){
    /* Place UCA3 in Reset to be configured */
    UCA3CTL1 = UCSWRST;

    /* Configure */
    UCA3CTL1 = UCSSEL1;
    UCA3BR0 = divider & 0xFF;
    UCA3BR1 = divider >> 8;
    UCA3MCTL = ( (fM & 0xF) << 4 |(sM & 0x7) << 1);

    /* Take UCA3 out of reset */
    UCA3CTL1 &= ~UCSWRST;

	//enable rx interrup on UCA1
	enable_UCA3_interrupt();

}

void disable_UCA0_interrupt(){
	UCA0IE &= ~UCRXIE;
}

void enable_UCA0_interrupt(){
	UCA0IE |= UCRXIE;
}

void enable_UCA3_interrupt(){
	UCA3IE |= UCRXIE;
}

void disable_UCA3_interrupt(){
	UCA3IE &= ~UCRXIE;
}

void disable_UCA1_interrupt(){
	UCA1IE &= ~UCRXIE;
}

void enable_UCA1_interrupt(){
	UCA1IE |= UCRXIE;
}

void clear_buf(){
	gsmBuf[0]='\0';
	bufIndex = 0;
}



//mode 0 is slow, mode 1 is faster
int uart_send_string_with_timeout( char * command, int mode, int timeout ){
	//printf("cmd: '%s'\n", command);
	char * command_cpy = command;
	do{
		command = command_cpy;
		clear_buf();
		enable_UCA0_interrupt();
		P3OUT |= 0x40;	//disable rts to not interrupt transmission
		while( *command != '\0' ){
			while (!(UCA0IFG & UCTXIFG));
			UCA0TXBUF = *command;
			command++;
		}
		P3OUT &= 0xBF; //enable rts to get something back
		//CRUCIAL, if i dont get interrupted during delay_cycles by the uart, I am screwed
		//seriously if this part does not get through, there is no way to handle message transmission
		if( mode ){
			__delay_cycles(600000);
		}
		else{
			__delay_cycles(7000000);
		}
		disable_UCA0_interrupt();
		printf("woken up, gsmbuf is: %s\n", gsmBuf);
		if(strstr(gsmBuf, "OK")){
			return 1;
		}
	}while(timeout--);
	return 0;
}

void uart_send_char( unsigned char command ){
	while(!(UCA0IFG & UCTXIFG));
	//printf("transmitting: %c\n", command);
	UCA0TXBUF = command;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{

	P3OUT |= 0x40; //set pin 41 --> rts to high to prevent gsm from sending extra shit cause im busy;
	//P3OUT &= 0xBF;
	char temp;
    switch(__even_in_range(UCA0IV,4)){
    	case 0:
    		break;                          				// Vector 0 - no interrupt
    	case 2:
    		//while ( !(UCA0IFG & UCRXIFG ));// Vector 2 - RXIFG, rx buffer is ready
    		//printf( "recieved char: %d\n", (char)UCA0RXBUF );                  // RXBUF1 to TXBUF1
    		temp = (char) UCA0RXBUF;

			if( bufIndex >= 1024 ){
				clear_buf();
			}
    		gsmBuf[ bufIndex++ ] = temp;
    		gsmBuf[ bufIndex ] = '\0';
    		//printf( "buf is now: %s\n", gsmBuf);
    		//printf ( "rxed char %c\n", temp);

			//any better ideas to parse this?
    		if(strstr( gsmBuf, "Arm" )){
				printf("im leaving LPM0\n");
				disable_UCA0_interrupt();
				clear_buf();
				printf("exiting LPM0\n");
				__bic_SR_register_on_exit(LPM0_bits);
			}

    		while (!(UCA3IFG & UCTXIFG));
    		UCA3TXBUF = temp; //write to the terminal
    		//printf( "character is : %c\n", temp );
    		break;
    	case 4:                                 				// Vector 4 - TXIFG
    		//printf( "case 4 txbuf is ready\n");
    		break;
    	default: break;
    }
    P3OUT &=0xBF;
    //reenable the rts line
}

#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    printf("interrupt from usca3\n");
    unsigned char tx_char;
    switch(__even_in_range(UCA3IV,4)){
    	case 0:
    		printf( "case 0 no interrupt\n");
    		break;                          				// Vector 0 - no interrupt
    	case 2:                                 				// Vector 2 - RXIFG, rx buffer is ready
    	  tx_char = UCA3RXBUF;
    	  printf( "Rxed char: %c\n", tx_char );                  // RXBUF1 to TXBUF1
    	  uart_send_char( tx_char );
    	  break;
    	case 4:                                 				// Vector 4 - TXIFG
    		//printf( "case 4 txbuf is ready\n");
    	  break;
    	default: break;
    }
}
