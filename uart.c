#include "uart.h"
#include <msp430.h>
#include <stdio.h>
#include <string.h>

//modulation parameters to get 9600 baud
unsigned divider = 104;
unsigned char fM = 1;
unsigned char sM = 0;

//int bufferReady = 0;

char gsmBuf[1024];
int bufIndex = 0;
int startTransmission = 0;

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

void disable_UCA0_interrupt(){
	UCA0IE &= ~UCRXIE;
}

void enable_UCA0_interrupt(){
	UCA0IE |= UCRXIE;
}

void disable_UCA1_interrupt(){
	UCA1IE &= ~UCRXIE;
}

void enable_UCA1_interrupt(){
	UCA1IE |= UCRXIE;
}

void uart_send_string( const char * command ){
	startTransmission = 1;
	do{
		while( *command != '\0' ){
			while (!(UCA0IFG & UCTXIFG));
			//printf("transmitting: %c\n", *command);
			UCA0TXBUF = *command;
			command++;
		}
		while( startTransmission );
	} while( !strcmp( gsmBuf, "ERROR" ));
}

void uart_send_char( unsigned char command ){
	while(!(UCA0IFG & UCTXIFG));
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
    		//printf( "case 0 no interrupt\n");
    		break;                          				// Vector 0 - no interrupt
    	case 2:
    		//while ( !(UCA0IFG & UCRXIFG ));// Vector 2 - RXIFG, rx buffer is ready
    		//printf( "recieved char: %d\n", (char)UCA0RXBUF );                  // RXBUF1 to TXBUF1
    		/*if( (char)UCA0RXBUF == '\r' ){
    			flag = 1;
    			//P3OUT &= 0xBF;
    			break;
    		}*/
    		temp = (char) UCA0RXBUF;
    		printf ( "rxed char %c\n", temp);
    		//gsmBuf[bufIndex++] = temp;
    		if( temp == 'K' && bufIndex >= 1 && gsmBuf[bufIndex - 1 ] == 'O' ){
    			//end of transmission
    			//printf("in here\n");
    			gsmBuf[ bufIndex - 1 ]='\0';
    			startTransmission = 0;
    			bufIndex = 0;
    		}

    		//any better ideas to parse this?
    		else if ( temp == 'R' && bufIndex >= 4 ){
    			if( gsmBuf[bufIndex - 1] == 'O' &&
    					gsmBuf[bufIndex - 2] == 'R' &&
    					gsmBuf[bufIndex -3 ] == 'R' &&
    					gsmBuf[bufIndex - 4] == 'E' ){
    				printf("error\n");
    				snprintf( gsmBuf , 6, "ERROR");
        			startTransmission = 0;
        			bufIndex = 0;
    			}
    		}

    		if( startTransmission && bufIndex < 1024 ){
    			gsmBuf[ bufIndex++ ] = temp;
    		}
    		//if( temp = 'O')

    		/*if( temp == '\r' ){
    			if( startTransmission ){
    				gsmBuf[bufIndex++] = '\0';
    				startTransmission = 0;
    				bufIndex = 0;
    				printf("buf is now: %s\n", gsmBuf);
    				bufferReady = 1;
    			}
    			else{
    				startTransmission = 1;
    				bufferReady = 0;
    			}
    		}

    		if ( startTransmission ){
    			gsmBuf[bufIndex++] = temp;
    		}*/
    		while (!(UCA1IFG & UCTXIFG));
    		UCA1TXBUF = temp; //write to the terminal
    		//printf( "character is : %c\n", temp );
    		//buff[count++] = temp;
    		//__delay_cycles(600000);
    		//P3OUT &=0xBF; //reset the rts flag
    		break;
    	case 4:                                 				// Vector 4 - TXIFG
    		printf( "case 4 txbuf is ready\n");
    		//while( !(UCA0IFG & UCTXIFG ));
    		//__delay_cycles( 5000 );
    		//__delay_cycles(5000);                 					// Add small gap between TX'ed bytes
    		//tx_char = P3IN;
    		//tx_char = tx_char >> 4;
    		//UCA0TXBUF = tx_char;                  					// Transmit character
    		break;
    	default: break;
    }
    //printf("char: %c\n", temp);
    P3OUT &=0xBF; //reenable the rts line
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    //while (!(UCA1IFG & UCTXIFG));

    //printf( "char: %c\n", (char)UCA0RXBUF );
    //UCA1IFG |= UCTXIFG;

    printf("interrupt from usca1\n");
    //unsigned char tx_char;
    unsigned char tx_char;
    switch(__even_in_range(UCA1IV,4)){
    	case 0:
    		printf( "case 0 no interrupt\n");
    		break;                          				// Vector 0 - no interrupt
    	case 2:                                 				// Vector 2 - RXIFG, rx buffer is ready
    	  tx_char = UCA1RXBUF;
    	  printf( "Rxed char: %c\n", tx_char );                  // RXBUF1 to TXBUF1
    	  uart_send_char( tx_char );
    	  break;
    	case 4:                                 				// Vector 4 - TXIFG
    		printf( "case 4 txbuf is ready\n");
    	  //__delay_cycles(5000);                 					// Add small gap between TX'ed bytes
    	  //tx_char = P5IN;
    	  //tx_char = tx_char >> 4;
    	  //UCA1TXBUF = tx_char;                  					// Transmit character
    	  break;
    	default: break;
    }
}
