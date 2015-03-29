#include <msp430.h> 
#include <stdio.h>
#include "uart.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>	//for exit code

#define NUM_TRIES 15
#define BUF_LEN 20
#define TEXT_BUF_LEN 100
#define SLEEP_CYCLE 7000000
#define MARGIN 0.0001
#define PRECISION 4
#define RESET_PERIOD 500000
#define GSM_FAILURE 3

//TODO: maybe get the number from the arm message instead of hard coding it?, in a more
//sophisticated system, you would have an onboard chip that stores the number and you just read it

char mikesnumber[] = "+16472006075";
//char mikesnumber[] = "+17059550333";

//these are the armed coordinates
float longitude;
float latitude;

void configureMSP( void );

int configureGSM_timeout( int tries );
int configureGPS_timeout( int tries );

int parseGPS( const char * gpgllMessage, float * lotude, float * latude );
void processGPS( void );
void processTEXT( void );

void send_text_with_timeout( const char * message, const char * number, int timeout );
void setInitialGPSstate( void );
int checksum( const char * string, char sum );
void hard_reset(void);
void gsm_send_cmd( char * cmd, int mode );



int notify = 1; //flag for sending armed message at most once

int main(void) {
	configureMSP();
    //UCA0IE |= UCTXIE;
    //UCA1IE |= UCRXIE + UCTXIE; //enable Rx & Tx interrupt
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled

    while( 1 ){
		//main event loop, check for text messages;
		printf("sleep for 5 seconds\n");
		__delay_cycles(SLEEP_CYCLE);
    	if( notify ){
			char textmsg[TEXT_BUF_LEN];
			setInitialGPSstate();
			send_text_with_timeout( "Armed at ", mikesnumber, NUM_TRIES );
			snprintf( textmsg, TEXT_BUF_LEN, "%.*f %.*f", PRECISION, latitude, PRECISION, longitude );
			send_text_with_timeout( textmsg, mikesnumber, NUM_TRIES );
		    gsm_send_cmd("AT+CMGD=1,4\r", 1);	//clear the text messages
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

int configureGSM_timeout(int tries){
	return
		uart_send_string_with_timeout("ATE0\r",1, tries) &&
		uart_send_string_with_timeout("AT+CMGF=1\r",1, tries) &&	//set message format else cant read message
		uart_send_string_with_timeout( "AT+CMGD=1,4\r",1, tries) && //delete all messages
		//set the notification of text messages directly to terminal
		uart_send_string_with_timeout( "AT+CNMI=2,2,0,0,0\r",1, tries);
}

int configureGPS_timeout(int tries){
	//set the prefix and the id, doesnt matter im gonna parse them out anyways
	return
		uart_send_string_with_timeout("AT#GPSPREFIX=\"&&\"\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSURAIS=\"ID1111\"\r",1, tries) &&

	//disable everything except for GPGLL messages
	//contains the longitude, latitude, date which we need
		uart_send_string_with_timeout("AT#GPSGPRMC=0\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSGPGGA=0\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSGPGLL=1\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSGPGSA=0\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSGPGSV=0\r",1, tries) &&
		uart_send_string_with_timeout("AT#GPSGPVTG=0\r",1, tries) &&

	//disable auto gps message sending for now
		uart_send_string_with_timeout("AT#GPSINTERVALLOC=0\r",1, tries);
}

void hard_reset(){
	int death_counter = 0;
	while( death_counter < GSM_FAILURE ){
		P3OUT &= 0x7F; //set the pin to low;
		__delay_cycles(RESET_PERIOD);
		P3OUT |= 0x80; //set the pin to high;
		__delay_cycles( SLEEP_CYCLE );
		if( configureGSM_timeout( NUM_TRIES ) && configureGPS_timeout( NUM_TRIES ) ){
			return;
		}
		death_counter++;
	}
	//unblink LED
	P1OUT &= 0x7F;
	printf("cant reset, going to die\n");
	exit( EXIT_FAILURE );
}


int parseGPS( const char * gpgllMessage, float * latude, float * lotude ){
	char gpsMessage[TEXT_BUF_LEN];
	char lat_dir, long_dir, valid;
	int lat_degrees, long_degrees, sum;
	float lat_min, long_min;
	//im expecting a very specific message format, if im not given:
	//<prefix><userid>$GPGLL,4916.45,N,12311.12,W,225444,A,*<checksum> i will fail
	if(sscanf(gpgllMessage, "%*[^$]$%99[^*]*%x%*s", gpsMessage, &sum) == 2){
		if(checksum(gpsMessage, sum)){
			printf( "gpsmessage: '%s'\n", gpsMessage);
			if(sscanf( gpsMessage, "GPGLL,%2d%8f,%c,%3d%8f,%c,%*[^,],%c%*s", &lat_degrees, &lat_min,
				&lat_dir, &long_degrees, &long_min, &long_dir, &valid ) == 7 ){
					if( valid == 'A'){
						*latude = ( lat_dir == 'N' ) ? ((float)lat_degrees + lat_min/60) : -((float)lat_degrees + lat_min/60);
						*lotude = ( long_dir == 'W' ) ? -((float)long_degrees + long_min/60) : ((float)long_degrees + long_min/60);
						return 1;
					}
			}
		}
	}
	return 0;
}

void processGPS(){
	char alert[TEXT_BUF_LEN];
	float current_latitude, current_longitude;
	char count = 0;
	while(count < NUM_TRIES){
		gsm_send_cmd("AT#GPSGETMESSAGE\r",1);
		if(parseGPS(gsmBuf, &current_latitude, &current_longitude)){
			printf( "current_longitude: %.*f, current_latitude %.*f\n", PRECISION,  current_longitude, PRECISION, current_latitude );
			//printf( "latitude diff: %.*f, longitude diff %.*f\n", PRECISION, (float)abs(current_longitude - longitude),
			//		PRECISION, (float)abs(current_latitude - latitude ));
			float longitude_margin = fabs(current_longitude - longitude );
			float latitude_margin = fabs(current_latitude - latitude );
			if(longitude_margin >= MARGIN || latitude_margin >= MARGIN){
				send_text_with_timeout( "theft detected, coordinates are:", mikesnumber, NUM_TRIES );
				snprintf(alert, TEXT_BUF_LEN, "%.*f %.*f",PRECISION, current_latitude, PRECISION, current_longitude);
				printf("sending alert '%s'\n", alert );
				send_text_with_timeout( alert, mikesnumber, NUM_TRIES );
				latitude = current_latitude;
				longitude = current_longitude;
			}
			else{
				snprintf(alert, TEXT_BUF_LEN, "no movement, lat_margin %.*f, long_margin %.*f, %.*f, %.*f",PRECISION, latitude_margin, PRECISION, longitude_margin,
						PRECISION, current_latitude, PRECISION, current_longitude );
				send_text_with_timeout( alert, mikesnumber, NUM_TRIES );
			}
			return;
		}
		count++;
	}
	send_text_with_timeout("gps signal lost, last known location:", mikesnumber, NUM_TRIES);
	snprintf( alert, TEXT_BUF_LEN, "%.*f %.*f", PRECISION, latitude,PRECISION, longitude );
	send_text_with_timeout( alert, mikesnumber, NUM_TRIES );
}

void setInitialGPSstate(){
	int tries = 0;
	while(1){
		while( tries < NUM_TRIES ){
			gsm_send_cmd("AT#GPSGETMESSAGE\r", 1);
			if(parseGPS(gsmBuf, &latitude, &longitude )){
				break;
			}
			tries++;
		}
		if( tries < NUM_TRIES ){
			break;
		}
		else{
			send_text_with_timeout( "cant acquire signal for arming, going to sleep", mikesnumber, NUM_TRIES );
			clear_buf();
			enable_UCA0_interrupt();
			tries = 0;
			__bis_SR_register(LPM0_bits + GIE );
		}
	}
	printf("initial state set\n");
}

//bootstrap, turn on interrupts from UCA0(gsm), UCA3(laptop term)
void configureMSP(){
	 WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

	    /* Configure Pin Muxing P3.5 RXD and P3.4 TXD */
	    P3SEL |= 0xB0;
	    P3DIR |= 0x40 | 0x80;
	    P3OUT &= 0xBF; //set RTS to low to start recieve from GSM

	    P3OUT |= 0x80; //set the reset high 3.7

	    // configure pin muxing pin 54 P5.7 rxd, pin 53 P5.6 txd uca1
	    //P5SEL |= 0xc0;
	    P10SEL |= 0x30;

		//LED
		//P1SEL |= 0x80;
		P1DIR |= 0x80;
		P1OUT |= 0x80;	//set the LED high; we are alive


	    configureUCA0(); //configures uca0 baudrate and enables interrupt
	    //configureUCA3(); //this is from laptop

	    __enable_interrupt(); //enable the interrupt for the uart ports so we can configure gsm
		int counter = 0;
		while( counter < GSM_FAILURE){
			if(configureGSM_timeout(NUM_TRIES) && configureGPS_timeout(NUM_TRIES)){
				break;
			}
			hard_reset();
			counter++;
		}
		if( counter >= GSM_FAILURE ){
			//extinguish LED, i am dead
			P1OUT &= 0x7F;
			exit(EXIT_FAILURE);
		}
		printf("gsm and gps set!\n");
	    enable_UCA0_interrupt();
}

void processTEXT(){
	//gsm_send_cmd("AT+CMGL=\"ALL UNREAD\"\r", 1);
	gsm_send_cmd("AT+CMGL=\"ALL\"\r", 1);
	if(strstr( gsmBuf, "Disarm" )){
		gsm_send_cmd("AT+CMGD=1,4\r", 1);
		printf("disarming\n");
		send_text_with_timeout("Disarmed, going to sleep", mikesnumber, NUM_TRIES);
		enable_UCA0_interrupt();
		printf("going in to sleep mode\n");
		notify=1;
		__bis_SR_register(LPM0_bits);
		printf("rearming\n");
		//i know if i come out of here i will go into the delay for 10 seconds, and i am rearmed
	}
	gsm_send_cmd("AT+CMGD=1,4\r", 1);
}

//wrapper for the uart_send_string
void gsm_send_cmd( char * command, int mode ){
	int death_counter = 0;
	while( death_counter < GSM_FAILURE ){
		if(uart_send_string_with_timeout( command, mode, NUM_TRIES )){
			return;
		}
		hard_reset();
		death_counter++;
	}
	//disable_LED
	P1OUT &= 0x7F;
	printf("im dying\n");
	exit(EXIT_FAILURE);
}

void send_text_with_timeout( const char * message, const char * number, int timeout ){
	char sendbuf[TEXT_BUF_LEN];
	snprintf( sendbuf, TEXT_BUF_LEN, "AT+CMGS=\"%s\"\r%s%c", number, message, 26 );
	//char 26 is control-z
	printf("sending message %s\n", sendbuf);
	int death_counter = 0;
	while( death_counter < GSM_FAILURE ){
		if( uart_send_string_with_timeout( sendbuf, 0, timeout )){
			return;
		}
		hard_reset();
		death_counter++;
	}
	//unblink LED
	P1OUT &= 0x7F;
	printf("cant send text, dying\n");
	exit( EXIT_FAILURE );
}
