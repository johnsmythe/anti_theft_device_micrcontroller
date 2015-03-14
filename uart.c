#include "uart.h"
#include <msp430.h>
#include <stdio.h>
#include <string.h>

//modulation parameters to get 9600 baud
unsigned divider = 104;
unsigned char fM = 1;
unsigned char sM = 0;

int char_count = 0;

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

void uart_send_string( const char * command ){
	startTransmission = 1;
	do{
		P3OUT |= 0x40;
		while( *command != '\0' ){
			while (!(UCA0IFG & UCTXIFG));
			//printf("transmitting: %c\n", *command);
			UCA0TXBUF = *command;
			command++;
		}
		P3OUT &= 0xBF; //enable rts to get something back
		while( startTransmission );
	} while( !strcmp( gsmBuf, "ERROR" ));
}

void sendText( const char * message, const char * number ){
	char sendbuf[100];
	snprintf( sendbuf, 100, "AT+CMGS=\"%s\"\r%s%c", number, message, 26 );
	printf( "sending message %s\n", sendbuf);
	uart_send_string(sendbuf);
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
    		if( bufIndex < 1024 ){
    			gsmBuf[ bufIndex++ ] = temp;
    			gsmBuf[ bufIndex ] = '\0';
    			//printf( "buf is now: %s\n", gsmBuf);
    		}
    		//printf ( "rxed char %c\n", temp);
    		//gsmBuf[bufIndex++] = temp;
    		if( startTransmission ){
				if( strstr( gsmBuf, "OK" ) ){
					//end of transmission
					//printf("in here\n");
					//printf( "ending transmission\n");
					//snprintf( gsmBuf, 3, "OK" );
					startTransmission = 0;
					bufIndex = 0;
				}

				//any better ideas to parse this?
				else if ( strstr(gsmBuf, "ERROR")){
					//printf("error\n");
					snprintf( gsmBuf , 6, "ERROR");
					startTransmission = 0;
					bufIndex = 0;
				}

				//gsmBuf[0] = '\0';
    		}
    		else{
    			//we are not confirming a sent command
    			//printf("shit\n");
    			//char textmessage[20];
    			if(strstr(gsmBuf, "+CMT")){
    				bufIndex = 0;
    				gsmBuf[0] = '\0';
    				char_count=0;
    			}
    			else if(strstr( gsmBuf, "Arm" )){
							printf("im leaving LPM0\n");
							//startTransmission = 0;
							//bufIndex = 0;
							//configureTimer();
							disable_UCA0_interrupt();
							char_count = 0;
							bufIndex = 0;
							gsmBuf[0] = '\0';
							printf("exiting\n");
							//disable_UCA0_interrupt();
							__bic_SR_register_on_exit(LPM0_bits);
				}
    		}


    		/*if( startTransmission && bufIndex < 1024 ){
    			gsmBuf[ bufIndex++ ] = temp;
    		}*/



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
    		while (!(UCA3IFG & UCTXIFG));
    		UCA3TXBUF = temp; //write to the terminal
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
    P3OUT &=0xBF;
    //reenable the rts line
}

#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    //while (!(UCA1IFG & UCTXIFG));

    //printf( "char: %c\n", (char)UCA0RXBUF );
    //UCA1IFG |= UCTXIFG;

    printf("interrupt from usca3\n");
    //unsigned char tx_char;
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
    		printf( "case 4 txbuf is ready\n");
    	  //__delay_cycles(5000);                 					// Add small gap between TX'ed bytes
    	  //tx_char = P5IN;
    	  //tx_char = tx_char >> 4;
    	  //UCA1TXBUF = tx_char;                  					// Transmit character
    	  break;
    	default: break;
    }
}
