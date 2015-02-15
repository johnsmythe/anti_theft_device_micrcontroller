#include <msp430.h> 
#include <stdio.h>
#include "uart.h"

/*
 * main.c
 */

void configureMSP( void );

//char buff[1024];
//int count = 0;
//int flag = 0;

int main(void) {

	configureMSP();
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

    //while( 1 );

}

//bootstrap, turn on interrupts from UCA0, UCA1
void configureMSP(){
	 WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

	    /* Configure Pin Muxing P3.5 RXD and P3.4 TXD */
	    P3SEL |= 0x30;
	    P3DIR |= 0x40;
	    P3OUT &= 0xBF; //set RTS to low to start recieve from GSM

	    // configure pin muxing pin 54 P5.7 rxd, pin 53 P5.6 txd uca1
	    P5SEL |= 0xc0;


	    configureUCA0(); //configures uca0 baudrate and enables interrupt
	    configureUCA1();

	    //UCA0IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
	    //UCA0IE |= UCTXIE;
	    __enable_interrupt();
}
