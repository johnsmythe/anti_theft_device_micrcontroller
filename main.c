#include <msp430.h> 
#include <stdio.h>

/*
 * main.c
 */

void configure( unsigned, unsigned char, unsigned char );
void configure1( unsigned, unsigned char, unsigned char );
unsigned char read();
void uart_send_char( unsigned char );
void uart_send( char * );

//char buff[1024];
//int count = 0;
//int flag = 0;

int main(void) {

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

    /* Configure Pin Muxing P3.5 RXD and P3.4 TXD */
    P3SEL |= 0x30;

    P3DIR |= 0x40;
    P3OUT &= 0xBF; //set rts to low to start recieve

    // configure pin muxing pin 54 P5.7 rxd, pin 53 P5.6 txd uca1
    P5SEL |= 0xc0;


    configure( 104, 1, 0 ); //configures uca0
    configure1( 104, 1, 0 ); //configure uca1

    //UCA0IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
    //UCA0IE |= UCTXIE;
    UCA0IE |= UCRXIE;
    UCA1IE |= UCRXIE;
    __enable_interrupt();

    //UCA0IE |= UCTXIE;
    //UCA1IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
    //UCA1IE |= UCRXIE;

    //char command[] = "AT\r";
    //int command[] = { 65, 84, 13, 0 };

    /*char command0[] = "AT\r\n\0";
    char command1[] = "AT+CFUN=1\r\n\0";
    char command2[] = "AT+IFC=2,2\r\n\0";
    uart_send(&command0);
    __delay_cycles(600000);
    uart_send(&command1);
    __delay_cycles(600000);
    uart_send(&command2);
    __delay_cycles(600000);
    printf( "Command sent successfully\n");
    char gpsON[] = "AT$GPSP=1\r\n\0";
    char gpsSTAT[] = "AT$GPSP?\r\n\0";
    uart_send(&gpsON);
    __delay_cycles(600000);
    uart_send(&gpsSTAT);*/
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled


    //while (count < 8);
    //while( !flag );
    //while ( count < 8 );
    //printf ( "count is: %d\n", count);
    //while (count > 0){
    	//printf("%c", buff[8-count]);
    	//count--;
    //}

    //printf( "Done\n");


    //while( 1 );

}

void configure( unsigned divider, unsigned char fM, unsigned char sM ){
    /* Place UCA0 in Reset to be configured */
    UCA0CTL1 = UCSWRST;

    /* Configure */
    UCA0CTL1 = UCSSEL1;
    UCA0BR0 = divider & 0xFF;
    //printf( "br0: %x\n", UCA0BR0);
    UCA0BR1 = divider >> 8;
    //printf( "br1: %x\n", UCA0BR1);
    UCA0MCTL = ( (fM & 0xF) << 4 |(sM & 0x7) << 1);
    //printf( "mctl: %x\n", UCA0MCTL);

    /* Take UCA0 out of reset */
    UCA0CTL1 &= ~UCSWRST;
}


void configure1( unsigned divider, unsigned char fM, unsigned char sM ){
    /* Place UCA1 in Reset to be configured */
    UCA1CTL1 = UCSWRST;

    /* Configure */
    UCA1CTL1 = UCSSEL1;
    UCA1BR0 = divider & 0xFF;
    UCA1BR1 = divider >> 8;
    UCA1MCTL = ( (fM & 0xF) << 4 |(sM & 0x7) << 1);

    /* Take UCA1 out of reset */
    UCA1CTL1 &= ~UCSWRST;
}


void uart_send( char * command ){
    while( *command ){
        while (!(UCA0IFG & UCTXIFG));
        printf( "transmitting %c\n", *command );
        UCA0TXBUF = *command;
      /*  UCA0IFG &= ~UCTXIFG;
        if( !(UCA0IFG & UCTXIFG) ){
            printf ("interrupt flag is disabled\n");
        }*/
        command++;
    }
}

void uart_send_char( unsigned char command ){
	while(!(UCA0IFG & UCTXIFG));
	printf("sending char %c\n", command );
	UCA0TXBUF = command;
}

/*  Echo back RXed character, confirm TX buffer is ready first */
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

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    //while (!(UCA0IFG & UCTXIFG));

    //printf( "char: %c\n", (char)UCA0RXBUF );
    //UCA0IFG |= UCTXIFG;

    //printf("interrupt from usca0\n");
    //unsigned char tx_char;
	P3OUT |= 0x40; //set pin 41 --> rts to high to prevent gsm from sending extra shit cause im busy;
	//P3OUT &= 0xBF;
	char temp;
    switch(__even_in_range(UCA0IV,4)){
    	case 0:
    		printf( "case 0 no interrupt\n");
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
