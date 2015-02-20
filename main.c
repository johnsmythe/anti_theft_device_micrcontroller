#include <msp430.h> 
#include <stdio.h>
#include "uart.h"

/*
 * main.c
 */
void configureTimer( void );
void configureMSP( void );
void configureGSM( void );

int counter = 0;

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

    while( 1 ){
    	//main event loop, check for text messages;
    	enable_UCA0_interrupt();
    	uart_send_string("AT+CMGL=\"REC UNREAD\"\r");
    	//printf("string buffer: %s", gsmBuf );
    	disable_UCA0_interrupt();
    	//printf( "sleeping for 5 secs maybe before going back to LPM\n");
    	//__delay_cycles(5000000);
    	//printf( "going to LPM0\n");
    	TA0CTL ^= (TASSEL_1 + MC_1 );	//reenable timer interrupt
    	__bis_SR_register(LPM0_bits);
    };

}

void configureGSM(){
	uart_send_string("ATE0\r");
	uart_send_string("AT+CMGF=1\r");
	printf( "gsm configure success!\n");
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
	    //configureUCA1(); //this is from laptop

	    __enable_interrupt(); //enable the interrupt for the uart ports so we can configure gsm
	    configureGSM();
	    disable_UCA0_interrupt();
	    configureTimer();

	    //UCA0IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
	    //UCA0IE |= UCTXIE;
	    //__enable_interrupt();
}

void configureTimer(){
	printf("configuring timer\n");
	TA0CCR0 = 24000; // 12kHz count limit generates one second interrupts
	TA0CCTL0 = 0x10; //enable counter interrupts
	TA0CTL = TASSEL_1 + MC_1; //select timer A0 with 12khz, counting UP

}

#pragma vector=TIMER0_A0_VECTOR
   __interrupt void Timer0_A0 (void) {		// Timer0 A0 interrupt service routine

	//printf( "timer interrupt, counter is %d\n", counter++ );
	counter++;
	if( counter == 10 ){

		//TA0CTL &= ~MC0; //disable interrupts
		TA0CTL ^= (TASSEL_1 + MC_1 );
		printf( "stop interrupts\n" );

		counter = 0;
		//__delay_cycles(5000000);
		//printf( "reenable interrupts\n" );
		//TA0CTL ^= (TASSEL_1 + MC_1 );
		//__disable_interrupt();
		printf( "exiting LPM0\n ");
		__bic_SR_register_on_exit(LPM0_bits);
	}

}