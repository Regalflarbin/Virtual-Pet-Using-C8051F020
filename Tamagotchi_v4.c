//Code written by Stuart Tresslar and James Gowdy
//Code partially derived from the book, tweaking values to use the potentiometer instead of the temp sensor
/*
Changelog:
v3: 
-Added LED Matrix control code, as well as data for Matrix layouts, and mood/hunger affecting display.
-Should play 12 frames of idle animation while all needs are met. Should sit down when sad,
display hamburger when hungry, and display ZZ while asleep, though light sensor code is pending
*/


#include <compiler_defs.h>
#include <c8051f020_defs.h>

#define SYSCLK 22118400

unsigned int ADC0_reading, ADC0_change, ADC0_old, counter, counter2, start = 1, timing = 0;
unsigned int happiness = 500, food = 400, water = 300;

/*
//Idle animation frames (Stand, Raise, Wave; two frames for each. All six for idle);
char Stand1[8] = {0x06,0x3C,0xC3,0x52,0x4A,0xD3,0x3C,0x06};
char Stand2[8] = {0x06,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x06};
char Raise1[8] = {0x70,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x70};
char Raise2[8] = {0xF0,0x3C,0xC3,0x52,0x4A,0xD3,0x3C,0xF0};
char Wave1[8]  = {0xF0,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x07};
char Wave2[8]  = {0x07,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0xF0};

//char ThirstyLED[8]  = {0x00,0x1E,0x3F,0xFF,0xFF,0x3F,0x1E,0x00}; //Kind of looks like a drop of water?
char AsleepLED[8] = {0x90,0xB0,0xD0,0x90,0x09,0x0B,0x0D,0x09}; //Two Zs, I guess?
char HungryLED[8] = {0x5A,0xDB,0xDB,0xDB,0xDB,0xDB,0xDB,0x5A}; //This is the closest I could get to a hamburger. Sorry -_-
char SadLED[8] = {0x03,0x0E,0x35,0x13,0x15,0x31,0x0E,0x03}; //Wasn't sure how to make him sad. I guess he sits down.
*/

char toDisplay[8];
int j = 0; //used for for loops in animation updates
int animationTimer = 0;
int idleFrame = 0;
int idle = 75;

//Booleans to track mood/things that need to display on LED matrix
int hungry = 0;
//int thirsty = 0;
int asleep = 0; //Boolean is not toggled on or off anywhere yet. Will depend on lighte sensor. I guess just turn it on if Sensor is dark.
int sad = 0;

void Init_Clock(void);
void Init_ADC0(void);
void Init_Timer3(unsigned int counts);
void Init_Port(void);
void Timer3_ISR(void);

void main(void)
{
	EA = 0;

	WDTCN = 0xDE;
	WDTCN = 0xAD;

	Init_Clock();
	Init_Port();
	Init_ADC0();
	
	P1 = 0xFF;
	P2 = 0xFF;

	Init_Timer3(SYSCLK/12/10);

	EA = 1;
	//Set to Stand1 Initially
	toDisplay[] = {0x06,0x3C,0xC3,0x52,0x4A,0xD3,0x3C,0x06};
	

	while(1){
		if(P2 == 0xFF){
			P2 = 0x7F;
			P1 = toDisplay[0];
			}
		else if(P2 == 0x7F){
			P2 = 0xBF;
			P1 = toDisplay[1];
			}
		else if(P2 == 0xBF){
			P2 = 0xDF;
			P1 = toDisplay[2];
			}
		else if(P2 == 0xDF){
			P2 = 0xEF;
			P1 = toDisplay[3];
			}
		else if(P2 == 0xEF){
			P2 = 0xF7;
			P1 = toDisplay[4];
			}
		else if(P2 == 0xF7){
			P2 = 0xFB;
			P1 = toDisplay[5];
			}
		else if(P2 == 0xFB){
			P2 = 0xFD;
			P1 = toDisplay[6];
			}
		else if(P2 == 0xFD){
			P2 = 0xFE;
			P1 = toDisplay[7];
			}
		else
			P2 = 0xFF;
	}
}


void Init_Clock(void)
{
	OSCXCN = 0x67;

	while (!(OSCXCN & 0x80));

	OSCICN = 0x88;
}

void Init_Timer3(unsigned int counts)
{
	TMR3CN = 0x00;

	TMR3RL = -counts;
	TMR3 = 0xffff;
	EIE2 |= 0x01;
	TMR3CN |= 0x04;
}

void Init_ADC0(void)
{
	REF0CN = 0x07;
	ADC0CF = 0x80;
	AMX0SL = 0x02;
	ADC0CN = 0x84;
}

void Init_Port(void)
{
	XBR0 = 0x00;
	XBR1 = 0x00;
	XBR2 = 0x40;

	P0MDOUT = 0x00;
	P1MDOUT = 0xFF;
	P2MDOUT = 0xFF;
	P3MDOUT = 0x00;

	P74OUT = 0x08;

	P5 |= 0x0F;
	P4 = 0xFF;
}

void Timer3_ISR(void) interrupt 14
{
	TMR3CN &= ~(0x80);

	while ((ADC0CN & 0x20) == 0);
	ADC0_reading = ADC0;
	ADC0CN &= 0xDF;
	

	if(++animationTimer % 25 == 0){
		animationTimer = 0;
		
		if(asleep > 0)//Set to AsleepLED
			toDisplay = {0x90,0xB0,0xD0,0x90,0x09,0x0B,0x0D,0x09};
		else if(sad > 0)
			toDisplay = {0x03,0x0E,0x35,0x13,0x15,0x31,0x0E,0x03};
		else if(hungry > 0)
			toDisplay = {0x5A,0xDB,0xDB,0xDB,0xDB,0xDB,0xDB,0x5A};
		else{
			idleFrame++;

			if(idleFrame == 1 || idleFrame == 3){ //Stand1
				toDisplay = {0x06,0x3C,0xC3,0x52,0x4A,0xD3,0x3C,0x06};
			}
			else if(idleFrame == 2 || idleFrame == 4){//Stand2
				toDisplay = {0x06,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x06};
			}
			else if(idleFrame == 5 || idleFrame == 7){//Raise1
				toDisplay = {0x70,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x70};
			}
			else if(idleFrame == 6 || idleFrame == 8){//Raise2
				toDisplay = {0xF0,0x3C,0xC3,0x52,0x4A,0xD3,0x3C,0xF0};
			}
			else if(idleFrame == 9 || idleFrame == 11){//Wave1
				toDisplay = {0xF0,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0x07};
			}
			else if(idleFrame == 10){//Wave2
				toDisplay = {0x07,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0xF0};
			}
			else{//Wave2
				toDisplay = {0x07,0x3C,0xD3,0x4A,0x52,0xC3,0x3C,0xF0};
				idleFrame = 0;		
			}

		}
	}


	if (start)
	{
		start = 0;
		ADC0_old = ADC0_reading;
	}

	if (timing > 1)
	{
		//Happiness/Playing System
		if (ADC0_reading > ADC0_old)
			ADC0_change = ADC0_reading - ADC0_old;
		else if (ADC0_reading < ADC0_old)
			ADC0_change = ADC0_old - ADC0_reading;
		else
			ADC0_change = 0;

		if(ADC0_change > 1000)
		{
			//counter2++;
			if (happiness > 1)
				happiness += 50
				happiness -= 50;
		}
		else if (ADC0_change > 750)
		{
			//counter++;
			if (happiness < 450)
				happiness += 25;
		}

		ADC0_old = ADC0_reading;

		if(happiness < 100)
		{
			P5 = 0x0F;
		}
		else if (happiness < 200)
		{
			P5 = 0x1F;
		}
		else if (happiness < 300)
		{
			P5 = 0x3F;
		}
		else if (happiness < 400)
		{
			P5 = 0x7F;
		}
		else
			P5 = 0xFF;

		if (happiness > 1)	
			happiness -= 1;

		if(happiness > 500)
			happiness = 500;

		if(happiness < 100)
			sad = 1;
		else
			sad = 0;

		//Food System
		if (!(P5 & 0x01)) //P5.0 being pressed
		{
			counter++;
			food += 2;
		}

		if (food >= 1)	
			food -= 1;

		if(food > 400)
			food = 400;

		if(food < 100)
			hungry = 1;
		else
			hungry = 0;

		/*
		//Water System
		if (!(P5 & 0x02))
			water += 2;
		
		if (water >= 1)	
			water -= 1;

		if(water > 300)
			water = 300;

		if(water < 100)
			thirsty = 1;
		else
			thirsty = 0;
		*/

		timing = 0;
	}

	timing++;
}