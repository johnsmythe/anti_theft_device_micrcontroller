#include <msp430.h> 
#include <stdio.h>
#include "uart.h"
#include <string.h>

#define NUM_TRIES 15
#define BUF_LEN 20
#define TEXT_BUF_LEN 100
#define SLEEP_CYCLE 5000000

//TODO: maybe get the number from the arm message instead of hard coding it?, in a more
//sophisticated system, you would have an onboard chip that stores the number and you just read it

//char mikesnumber[] = "+16472006075";
char mikesnumber[] = "+17059550333";

//these are the armed coordinates
char longitude[BUF_LEN];
char latitude[BUF_LEN];

void configureMSP( void );
void configureGSM( void );
void configureGPS( void );
int parseGPS( const char * gpgllMessage, char * lotude, char * latude );

void processGPS( void );
void processTEXT( void );
void sendText( const char * message, const char * number );
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
		printf("sleep for 5 seconds\n");
		__delay_cycles(SLEEP_CYCLE);
    	if( notify ){
			char textmsg[TEXT_BUF_LEN];
			setInitialGPSstate();
			sendText( "Armed at ", mikesnumber );
			//snprintf(textmsg, TEXT_BUF_LEN, "armed at: latitude %s, longitude %s", latitude,
					//longitude);
			snprintf( textmsg, TEXT_BUF_LEN, "%s %s", latitude, longitude );
			//printf("sending text: '%s'\n", textmsg);
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
	char gpsMessage[TEXT_BUF_LEN];
	char lat_dir, long_dir, valid;
	int lat_degrees, long_degrees, sum;
	float lat_min, long_min;
	//im expecting a very specific message format, if im not given:
	//<prefix><userid>$GPGLL,4916.45,N,12311.12,W,225444,A,*<checksum> i will fail
	if(sscanf(gpgllMessage, "%*[^$]$%99[^*]*%x%*s", gpsMessage, &sum) == 2){
		if(checksum(gpsMessage, sum)){
			if(sscanf( gpsMessage, "GPGLL,%2d%8f,%c,%3d%8f,%c,%*[^,],%c%*s", &lat_degrees, &lat_min,
				&lat_dir, &long_degrees, &long_min, &long_dir, &valid ) == 7 ){
					if( valid == 'A'){
						float _latitude = ( lat_dir == 'N' ) ? ((float)lat_degrees + lat_min/60) : -((float)lat_degrees + lat_min/60);
						float _longitude = ( long_dir == 'W' ) ? -((float)long_degrees + long_min/60) : ((float)long_degrees + long_min/60);
						snprintf( latude, BUF_LEN, "%6.2f", _latitude );
						snprintf( lotude, BUF_LEN, "%7.2f", _longitude );
						return 1;
					}
			}
		}
	}
	return 0;
}

void processGPS(){
	char alert[TEXT_BUF_LEN];
	char current_longitude[BUF_LEN];
	char current_latitude[BUF_LEN];
	char count = 0;
	while(count < NUM_TRIES){
		uart_send_string("AT#GPSGETMESSAGE\r",1);
		if(parseGPS(gsmBuf, current_latitude, current_longitude)){
			break;
		}
		count++;
	}

	if( count < 15 ){
		if(strcmp( current_latitude, latitude ) || strcmp( current_longitude, longitude )){
			sendText( "theft detected coordinates are:", mikesnumber );
			snprintf(alert, TEXT_BUF_LEN, "%s %s", current_latitude,
						current_longitude);
			printf("sending alert '%s'\n", alert );
			sendText(alert, mikesnumber);
			strcpy(latitude, current_latitude);
			strcpy(longitude, current_longitude);
		}
	}
	else{
		sendText("gps signal lost, last known location:", mikesnumber);
		snprintf( alert, TEXT_BUF_LEN, "%s %s", latitude, longitude );
		sendText( alert, mikesnumber );
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

void sendText( const char * message, const char * number ){
	char sendbuf[TEXT_BUF_LEN];
	snprintf( sendbuf, 100, "AT+CMGS=\"%s\"\r%s%c", number, message, 26 );
	//char 26 is control-z
	printf("sending message %s\n", sendbuf);
	uart_send_string(sendbuf, 0);
}
