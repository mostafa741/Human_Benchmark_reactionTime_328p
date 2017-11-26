/*
* Human_Benchmark_reactionTime_328p.c
*
* Created: 24-Jan-17 7:58:26 AM
* Author : Mostafa
*/

#define desiredNumberOfTries 5
/*
#define STATE_0 0
#define STATE_1 1
#define STATE_2 2
#define STATE_3 3*/
#define LEDDDR DDRC
#define LEDPORT PORTC
#define LEDPIN PINC0
#define SPEAKER_PIN PINC2
#define BUTTONDDR DDRC
#define BUTTONPORT PORTC
#define BUTTONPIN PINC1
#define BUTTONPINR PINC
#define CONFIDENCE 2000


#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include "hd44780.h"

static void setup();
static void setupTimer();
static void startTimer();
static void stopTimer();
static void resetTimer();
static void welcomeMessage();
static void resetLED();
//static void setupTimerForRandomNumber();
static void setupPWM();

typedef enum STATES{STATE_0,STATE_1,STATE_2,STATE_3} STATES;

volatile uint16_t randomTime_ms=0;
volatile uint16_t timeElapsed_ms=0;
volatile STATES currentState=STATE_0;
volatile uint8_t playTone=0;
char buffer[16];


int main(void)
{
	uint16_t tries[desiredNumberOfTries]={0};
	uint8_t currentTries=0;
	/*uint16_t releaseconfidenceLevel=0;
	uint8_t pushedFlag=0;*/
	uint16_t*const bestTimeAddress=(uint16_t*)0x02;
	uint16_t*const bestAvgTimeAddress=(uint16_t*)0x00;
	uint16_t bestAvgTime=(eeprom_read_word(bestAvgTimeAddress)<1000)?eeprom_read_word(bestAvgTimeAddress):1000;
	uint16_t bestTime=(eeprom_read_word(bestTimeAddress)<1000)?eeprom_read_word(bestTimeAddress):1000;

	uint8_t pushedFlag=0,releaseFlag=1;
	uint16_t releaseConfidence=0,pushConfidence=0;


	lcd_init();
	welcomeMessage();
	setup();
	while (1)
	{
		/*if ((BUTTONPINR&_BV(BUTTONPIN)))
		{
		releaseconfidenceLevel++;
		if (releaseconfidenceLevel>200)
		{
		pushedFlag=0;
		releaseconfidenceLevel=0;
		}
		}
		else
		{
		releaseconfidenceLevel=0;
		}*/

		if (bit_is_clear(BUTTONPINR,BUTTONPIN))
		{
			pushConfidence++;
			releaseConfidence = 0;
			if (pushConfidence >(currentState==STATE_2?0: CONFIDENCE))
			{
				pushConfidence = 0;
				pushedFlag = 1;
			}
		}

		if (bit_is_set(BUTTONPINR,BUTTONPIN))
		{
			releaseConfidence++;
			pushConfidence = 0;
			if (releaseConfidence > CONFIDENCE)
			{
				releaseConfidence = 0;
				pushedFlag = 0;
				releaseFlag = 1;
			}
		}

		//if(!pushedFlag&&!(BUTTONPINR&_BV(BUTTONPIN)))

		if (pushedFlag&&releaseFlag)
		{
			//pushedFlag=1;
			releaseFlag=0;
			releaseConfidence = 0;
			pushConfidence = 0;
			switch (currentState)
			{
				//////////////////////////////////////////////////////////////////////////////////////////
				//State 0
				case STATE_0:
				stopTimer();
				lcd_clrscr();
				resetLED();
				lcd_puts("Wait for it");
				currentState=STATE_1;
				srand(TCNT2);
				randomTime_ms=rand()%1000+1000;
				//randomTime_ms=2;
				timeElapsed_ms=0;
				startTimer();
				break;
				//////////////////////////////////////////////////////////////////////////////////////////
				//State 1
				case STATE_1:
				stopTimer();
				lcd_clrscr();
				lcd_puts("CHEATER!");
				currentState=STATE_0;
				currentTries=0;
				resetLED();
				break;
				//////////////////////////////////////////////////////////////////////////////////////////
				//State 2
				case STATE_2:
				stopTimer();
				playTone=0;
				resetLED();
				lcd_clrscr();
				lcd_puts("Time = ");
				sprintf(buffer,"%u",timeElapsed_ms);
				lcd_puts(buffer);
				lcd_goto(0x40);
				sprintf(buffer,"%u",bestTime);
				lcd_puts(buffer);
				tries[currentTries++]=timeElapsed_ms;
				if (timeElapsed_ms<bestTime)
				{
					bestTime=timeElapsed_ms;
					eeprom_update_word(bestTimeAddress,timeElapsed_ms);
				}
				currentState=(currentTries==desiredNumberOfTries)?STATE_3:STATE_0;
				break;
				//////////////////////////////////////////////////////////////////////////////////////////
				//State 3
				case STATE_3:
				lcd_clrscr();
				uint16_t average=0;
				for (unsigned char i=0;i<desiredNumberOfTries;i++)
				{
					average+=tries[i];
				}
				average=average/desiredNumberOfTries;
				lcd_puts("Avg T = ");
				sprintf(buffer,"%u",average);
				lcd_puts(buffer);
				lcd_goto(0x40);
				sprintf(buffer,"%u",bestAvgTime);
				lcd_puts(buffer);
				if (average<bestAvgTime)
				{
					bestAvgTime=average;
					eeprom_update_word(bestAvgTimeAddress,average);
				}
				currentState=STATE_0;
				currentTries=0;
				break;
				//////////////////////////////////////////////////////////////////////////////////////////
				default:
				break;
			}
		}
	}
}

static void setup(){
	/*SET LED PIN AS OUTPUT*/
	LEDDDR|=_BV(LEDPIN);
	/*PULLUP FOR BUTTON*/
	BUTTONPORT|=_BV(BUTTONPIN);
	/*SET SPEAKER PIN AS OUTPUT*/
	DDRC|=_BV(SPEAKER_PIN);
	setupTimer();
	setupPWM();
	/*ENABLE GLOBAL INTERRUPT*/
	sei();
}

static void setupTimer(){
	TIMSK0|=_BV(OCIE0A);
	TCCR0A|=_BV(WGM01);
	OCR0A=249;
	TIMSK1|=_BV(OCIE1A);
	OCR1A=124;
	TCCR1B|=_BV(CS10)|_BV(CS11)|_BV(WGM12);
}


static void startTimer(){
	//start timer (set prescaler to 64)
	TCNT0=0;
	TCCR0B|=_BV(CS00)|_BV(CS01);
}

static void stopTimer(){
	//stop timer (set prescaler to 0)
	TCCR0B&=~(_BV(CS00)|_BV(CS01));
}
static void resetTimer(){
	stopTimer();
	startTimer();
}

ISR(TIMER0_COMPA_vect){
	//Will be executed after 1ms
	timeElapsed_ms++;

	if (timeElapsed_ms==randomTime_ms)
	{
		playTone=1;
		LEDPORT|=_BV(LEDPIN);
		currentState=STATE_2;
		timeElapsed_ms=0;
		resetTimer();
	}
	else if (currentState==STATE_2&&timeElapsed_ms>=1000)
	{
		stopTimer();
		lcd_clrscr();
		lcd_puts("Too slow :/");
		currentState=STATE_0;
		playTone=0;
		resetLED();
	}
}

ISR(TIMER1_COMPA_vect)
{
	if(playTone)
	{
		PORTC^=_BV(SPEAKER_PIN);
	}
	else
	PORTC&=~_BV(SPEAKER_PIN);

}

static void resetLED(){
	LEDPORT&=~_BV(LEDPIN);
}

static void welcomeMessage(){
	lcd_puts("Ready");
}

static void setupPWM(){
	DDRD|=_BV(PIND3);
	OCR2B=56;
	TCCR2A|=_BV(WGM20)|_BV(WGM21)|(0<<COM2B0)|_BV(COM2B1);
	TCCR2B|=_BV(CS20)|(0<<CS21)|(0<<CS22);
}



