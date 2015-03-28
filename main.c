#include <msp430.h> 
#include <stdio.h>
#include "uart.h"
#include <assert.h>
#include <string.h>

//TODO: maybe get the number from the arm message instead of hard coding it?, in a more
//sophisticated system, you would have an onboard chip that stores the number and you just read it

char mikesnumber[] = "+16472006075";

//these are the armed coordinates
char longitude[20];
char latitude[20];

void configureMSP( void );
void configureGSM( void );
void configureGPS( void );
int parseGPS( const char * gpgllMessage, char * lotude, char * latude );

void processGPS( void );
void processTEXT( void );
void setInitialGPSstate( void );
int checksum( const char * string, char sum );
int notify = 1; //flag for sending armed message at most once

int main(void) {
	configureMSP();
    //UCA0IE |= UCTXIE;
    //UCA1IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled

    while( 1 ){
		//main event loop, check for text messages;
		printf("sleep for 10 seconds\n");
		__delay_cycles(10000000);
    	if( notify ){
			char textmsg[100];
			setInitialGPSstate();
			snprintf(textmsg, 100, "armed at: latitude %s, longitude %s", latitude,
					longitude);
			printf("sending text: '%s'\n", textmsg);
    		sendText(textmsg, mikesnumber);
    		uart_send_string("AT+CMGD=1,4\r", 1);	//clear the text messages
    		notify=0;
    	}
		else{
			processGPS();
			processTEXT();
		}
    }
}

int checksum( const char * string, char sum ){
    int counter;
    if( string ){
        char check = 0;
        for( counter = 0;string[counter] != '\0'; counter++ ){
            check ^= string[counter];
        }
        if( check == sum ){
			printf("passed checksum\n");
            return 1;
        }
    }
	printf("failed checksum\n");
    return 0;
}

void configureGSM(){
	uart_send_string("ATE0\r",1);
	uart_send_string("AT+CMGF=1\r",1);
	uart_send_string( "AT+CMGD=1,4\r",1); //delete all messages
	//set the notification of text messages directly to terminal
	uart_send_string( "AT+CNMI=2,2,0,0,0\r",1);
	printf( "gsm configure success!\n");
}

void configureGPS(){
	//set the prefix and the id, doesnt matter im gonna parse them out anyways
	uart_send_string("AT#GPSPREFIX=\"&&\"\r",1);
	uart_send_string("AT#GPSURAIS=\"ID1111\"\r",1);

	//disable everything except for GPGLL messages
	//contains the longitude, latitude, date which we need
	uart_send_string("AT#GPSGPRMC=0\r",1);
	uart_send_string("AT#GPSGPGGA=0\r",1);
	uart_send_string("AT#GPSGPGLL=1\r",1);
	uart_send_string("AT#GPSGPGSA=0\r",1);
	uart_send_string("AT#GPSGPGSV=0\r",1);
	uart_send_string("AT#GPSGPVTG=0\r",1);

	//disable auto gps message sending for now
	uart_send_string("AT#GPSINTERVALLOC=0\r",1);
	printf("gps configure success!\n");
}

int parseGPS( const char * gpgllMessage, char * latude, char * lotude ){
	char gpsMessage[100];
	char lat_dir[2];
	char long_dir[2];
	char valid;
	int sum;
	//im expecting a very specific message format, if im not given:
	//<prefix><userid>$GPGLL,4916.45,N,12311.12,W,225444,A,*<checksum> i will fail
	//if(sscanf(gpgllMessage, "%*[^$]$%99[^*]*%x%*s", gpsMessage, &sum) == 2){
	if(sscanf(gpgllMessage, "%*[^$]$%99[^*]*%x%*s", gpsMessage, &sum) == 2){
		if(checksum(gpsMessage, sum)){
			if(sscanf( gpsMessage, "GPGLL,%18[^,],%1s,%18[^,],%1s,%*[^,],%c%*s", latude,
				lat_dir, lotude, long_dir, &valid ) == 5 ){
				if( valid == 'A'){
					strcat(latude, lat_dir);
					strcat(lotude, long_dir);
					return 1;
				}
			}
		}
	}
	return 0;
}

void processGPS(){
	char alert[100];
	char current_longitude[20];
	char current_latitude[20];
	while(1){
		uart_send_string("AT#GPSGETMESSAGE\r",1);
		if(parseGPS(gsmBuf, current_latitude, current_longitude)){
			break;
		}
		//sendText( "gps signal lost", mikesnumber );
	}
	if(strcmp( current_latitude, latitude ) || strcmp( current_longitude, longitude )){
		snprintf(alert, 100, "alert: latitude %s, longitude %s", current_latitude,
					current_longitude);
		printf("sending alert '%s'\n", alert );
    	sendText(alert, mikesnumber);
	}
}

void setInitialGPSstate(){
	while(1){
		uart_send_string("AT#GPSGETMESSAGE\r", 1);
		if(parseGPS(gsmBuf, latitude, longitude )){
			break;
		}
	}
	printf("initial state set\n");
}

//bootstrap, turn on interrupts from UCA0(gsm), UCA3(laptop term)
void configureMSP(){
	 WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

	    /* Configure Pin Muxing P3.5 RXD and P3.4 TXD */
	    P3SEL |= 0xB0;
	    P3DIR |= 0x40;
	    P3OUT &= 0xBF; //set RTS to low to start recieve from GSM
	    //P3OUT |= 0x80; //set the reset high 3.7

	    // configure pin muxing pin 54 P5.7 rxd, pin 53 P5.6 txd uca1
	    //P5SEL |= 0xc0;
	    P10SEL |= 0x30;


	    configureUCA0(); //configures uca0 baudrate and enables interrupt
	    //configureUCA3(); //this is from laptop

	    __enable_interrupt(); //enable the interrupt for the uart ports so we can configure gsm
	    configureGSM();
		configureGPS();
	    enable_UCA0_interrupt();
}

void processTEXT(){
	uart_send_string("AT+CMGL=\"REC UNREAD\"\r", 1);
	if(strstr( gsmBuf, "Disarm" )){
		uart_send_string("AT+CMGD=1,4\r", 1);
		clear_buf();
		printf("disarming\n");
		sendText("Disarmed, going to sleep", mikesnumber);
		enable_UCA0_interrupt();
		printf("going in to sleep mode\n");
		notify=1;
		__bis_SR_register(LPM0_bits);
		//i know if i come out of here i will go into the delay for 10 seconds, and i am rearmed
	}
}
