//Timer 2 logic is from the book in Chapter 8.11, starting at pg171
//I2C logic is from https://www.8051projects.net/wiki/I2C_Implementation_on_8051
//TCS34725 logic is from the Adafruit_TCS34725.h and Adafruit_TCS34725.cpp libraries that Adafruit hosts on Github
//Adafruit_TCS34725 library: https://github.com/adafruit/Adafruit_TCS34725
//Code from library used as a reference, as it was built using Arduino libraries and not compatible with the 8051
//'TCS34725' has been replaced with 'RGB' to make the code easier to read for any viewer
//I2C, TCS34725, and LSM303 implementation written by Stuart Tresslar
 


#include <compiler_defs.h>
#include <c8051f020_defs.h>
#include <stdlib.h>

//Needed for Timing
unsigned char count;
unsigned char ackcount = 0;
unsigned char ackcounter = 0;

//Values needed for the TCS34725 RGB Sensor
#define RGB_ADDRESS			(0x29)
#define RGB_COMMAND_BIT		(0x80)
#define RGB_ENABLE			(0x00)
#define RGB_ENABLE_PON 		(0x01)
#define RGB_ENABLE_AEN		(0x02)
#define RGB_ATIME 			(0x01)
#define RGB_CONTROL			(0x0F)
#define RGB_CDATAL			(0x14)
#define RGB_RDATAL			(0x16)
#define RGB_GDATAL			(0x18)
#define RGB_BDATAL			(0x1A)
#define RGB_ID				(0x12)

unsigned char _RGBInit;
unsigned int green = 0, red = 0, blue = 0, clear = 0;

//Values needed for the LSM303 Accelerometer
#define LSM_ADDRESS						(0x32 >> 1)
#define LSM_ID							(0b11010100)
#define LSM_REGISTER_ACCEL_OUT_X_L_A 	(0x28)
#define LSM_REGISTER_ACCEL_CTRL_REG1_A	(0x20)

unsigned int accelx, accely, accelz;

//Port definitions for the I2C connection
sbit SDA = P1^0;
sbit SCL = P1^1;

//MsDelay code from: http://blowtech.blogspot.com/2013/08/different-ways-to-generate-delays-in.html
void MsDelay(unsigned int itime)
{
	unsigned int i,j;
	for (i = 0; i < itime; i++)
	{
		for (j = 0; j < 1275; j++) {}
	}
}

void Init_Port(void)
{
	XBR0 = 0x00;
	XBR1 = 0x00;
	XBR2 = 0x40;

	P0MDOUT = 0x00;
	P1MDOUT = 0x00;
	P2MDOUT = 0x00;
	P3MDOUT = 0x00;

	P74OUT = 0x08;

	P5 |= 0x0F;
	P4 = 0xFF;
}

void Init_Timer2(unsigned int counts)
{
	CKCON = 0x00;
	T2CON = 0x00;

	RCAP2 = counts;

	T2 = 0xFFFF;

	IE |= 0x20;
	T2CON |= 0x04;
}

void I2CInit()
{
	SDA = 1;
	SCL = 1;
}

void I2CStart()
{
	SDA = 0;
	SCL = 0;
}

void I2CRestart()
{
	SDA = 1;
	SCL = 1;
	SDA = 0;
	SCL = 0;
}

void I2CStop()
{
	SCL = 0;
	SDA = 0;
	SCL = 1;
	SDA = 1;
}

void I2CAck()
{
	SDA = 0;
	SCL = 1;
	SCL = 0;
	SDA = 1;
}

void I2CNak()
{
	SDA = 1;
	SCL = 1;
	SCL = 0;
	SDA = 1;
}

unsigned char I2CSend(unsigned char Data)
{
	unsigned char i, ack_bit;
	for (i = 0; i < 8; i++) 
	{
		if ((Data & 0x80) == 0)
			SDA = 0;
		else
			SDA = 1;
		SCL = 1;
		SCL = 0;
		Data <<= 1;
	}
	//SDA = 1;
	SCL = 1;
	ack_bit = SDA;
	SCL = 0;
	return ack_bit;
}

unsigned char I2CRead()
{
	unsigned char i, Data = 0;
	for (i = 0; i < 8; i++)
	{
		SCL = 1;
		if (SDA)
			Data |= 1;
		if (i<7)
			Data <<= 1;
		SCL = 0;
	}
	return Data;
}

void RGB_Write8(char reg, int value)
{
	char ack;
	I2CInit();
	I2CStart();
	//Sends the address to establish connection
	ack = I2CSend(RGB_ADDRESS);
	if (!ack)
		ackcount++;
	ack = I2CSend(RGB_COMMAND_BIT | reg);
	ack = I2CSend(value & 0xFF);
	I2CStop();
}

unsigned char RGB_Read8(char reg)
{
	unsigned char x, ack;
	I2CInit();
	I2CStart();
	ack = I2CSend(RGB_ADDRESS);
	x = I2CRead();

	I2CSend(RGB_COMMAND_BIT | reg);

	x = I2CRead();
	I2CNak();
	I2CStop();
	return x;
}

unsigned int RGB_Read16(char reg)
{
	unsigned int x, t, ack;
	I2CInit();
	I2CStart();
	ack = I2CSend(RGB_ADDRESS | 1);
	I2CSend(RGB_COMMAND_BIT | reg | 1);
	I2CAck();
	x = I2CRead();
	I2CAck();
	t = I2CRead();
	I2CNak();
	I2CStop();
	x <<= 8;
	x |= t;
	return x;
}

void RGB_Enable(void)
{
	RGB_Write8(RGB_ENABLE, RGB_ENABLE_PON);
	MsDelay(3);
	RGB_Write8(RGB_ENABLE, RGB_ENABLE_PON | RGB_ENABLE_AEN);
}

void RGB_Disable(void)
{
	char reg = 0;
	reg = RGB_Read8(RGB_ENABLE);
	RGB_Write8(RGB_ENABLE, reg & ~(RGB_ENABLE_PON | RGB_ENABLE_AEN));
}

unsigned char RGB_Begin(void)
{
	unsigned char x;
	I2CInit();
	I2CStart();

	x = RGB_Read8(RGB_ID);
	if (x != 0x44)
	{
		count ++;
		if (count > 50)
			count = 0;
		return 0;
	}
	_RGBInit = 1;

	RGB_Write8(RGB_ATIME, 0xFF);

	RGB_Write8(RGB_CONTROL, 0x00);

	RGB_Enable();

	I2CStop();

	return 1;
}

void RGB_Raw()
{
	if (!_RGBInit)
		RGB_Begin();
	clear = RGB_Read16(RGB_CDATAL);
	red = RGB_Read16(RGB_RDATAL);
	green = RGB_Read16(RGB_GDATAL);
	blue = RGB_Read16(RGB_BDATAL);
}

char LSM_Begin()
{
	char ack0 = 1;
	I2CInit();
	I2CStart();
	ack0 = I2CSend(LSM_ADDRESS);
	if (!ack0)
		ackcount++;
	ack0 = I2CSend(LSM_REGISTER_ACCEL_CTRL_REG1_A);
	ack0 = I2CSend(0x27);
	I2CStop();
	return 1;
}

void LSM_Read(void)
{
	char ack1 = 1, xlo, xhi, ylo, yhi, zlo, zhi;
	LSM_Begin();

	I2CInit();
	I2CStart();
	ack1 = I2CSend(LSM_ADDRESS);
	if (!ack1)
		ackcounter++;
	ack1 = I2CSend(LSM_REGISTER_ACCEL_OUT_X_L_A | 0x80);
	I2CStop();
	I2CInit();
	I2CStart();
	I2CSend(LSM_ADDRESS | 1);
	I2CAck();
	xlo = I2CRead();
	I2CAck();
	xhi = I2CRead();
	I2CAck();
	ylo = I2CRead();
	I2CAck();
	yhi = I2CRead();
	I2CAck();
	zlo = I2CRead();
	I2CAck();
	zhi = I2CRead();
	I2CNak();
	I2CStop();

	accelx = ((xhi << 8) | xlo);
	accely = ((yhi << 8) | ylo);
	accelz = ((zhi << 8) | zhi);
}

void main(void)
{
	EA = 0;

	WDTCN = 0xDE;
	WDTCN = 0xAD;

	Init_Port();

	Init_Timer2(1);

	P5 = P5 & 0xFF;

	EA = 1;

	//RGB_Enable();

	ackcount = 0;
	ackcounter = 0;
	accely = accelx = accelz = 0;
	
	while(1);
}

//This is to update values for every timer cycle
void Timer2_ISR(void) interrupt 5
{
	T2CON &= ~(0x80);

	RGB_Raw();
	//LSM_Read();
	
	P5 = P5 ^ 0xF0;


}







