/*-------------------------------------------------------------------------------------------
 ********************************************************************************************
 *-------------------------------------------------------------------------------------------
 *
 *				DATA LOGGER DE VARIABLES AMBIENTALES INTERNAS
 *							CIMEC CONICET - UTN FRP
 *								     2016
 *
 *						Polo, Franco		fjpolo@frp.utn.edu.ar
 *						Burgos, Sergio		sergioburgos@frp.utn.edu.ar
 *						Bre, Facundo		facubre@cimec.santafe-conicet.gov.ar
 *
 *	datalogger.c
 *
 *	Descripción:
 *
 *  Implementación de las funciones a utilizar en el datalogger
 *
 *  - Periféricos I2C:
 *  	a) HR y Tbs		HIH9131		0b0100111		0x27
 *  	b) Ev			TSL2563		0b0101001		0x29
 *  	c) Va			ADS			0b1001000		0x48
 *  	d) Tg			LM92		0b1001011		0x51
 *  	e) RTC			DS1703		0b1101000		0x68
 *
 *  - Periféricos OneWire@PD6
 *  	a) Ts01			MAX31850	ROM_Addr		0x3B184D8803DC4C8C
 *  	b) Ts02			MAX31850	ROM_Addr		0x3B0D4D8803DC4C3C
 *  	c) Ts03			MAX31850	ROM_Addr		0x3B4D4D8803DC4C49
 *  	d) Ts04			MAX31850	ROM_Addr		0x3B234D8803DC4C99
 *  	e) Ts05			MAX31850	ROM_Addr		0x3B374D8803DC4C1E
 *  	f) Ts06			MAX31850	ROM_Addr
 *
 *  - IHM
 *  	a) RESET		!RST
 *  	b) SW_SD		PC6
 *  	c) SW_ON		PC5
 *  	d) SW_1			PC7
 *  	e) WAKE			PF2
 *  	f) LEDON		PE0
 *  	g) LED1			PE1
 *  	h) LED2			PE2
 *
 *
 *--------------------------------------------------------------------------------------------
 *********************************************************************************************
 *-------------------------------------------------------------------------------------------*/

/*********************************************************************************************
 * INCLUDES
 ********************************************************************************************/
// inc
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
// driverlib
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pwm.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/hibernate.h"
// datalogger
#include "datalogger/datalogger.h"
#include "datalogger/delay.h"

/*****************************************************************************************************
 * Defines
 ****************************************************************************************************/
// SWITCHES
#define SW_PORT 	GPIO_PORTC_BASE
#define SW_ON 		GPIO_PIN_5
#define SW_SD 		GPIO_PIN_6
#define SW_1 		GPIO_PIN_7
// LEDS
#define LED_PORT 	GPIO_PORTE_BASE
#define LED_ON 		GPIO_PIN_0
#define LED_1 		GPIO_PIN_1
#define LED_2 		GPIO_PIN_2
// Timer0
#define TOGGLE_FREQUENCY 1

/*********************************************************************************************
 * Functions
 * ******************************************************************************************/
extern void Hibernate_IRQHandler (void);

/*********************************************************************************************
 * Global variables
 * ******************************************************************************************/
extern unsigned long int _MSDELAY_BASE;
extern unsigned long int _USDELAY_BASE;
unsigned long ulPeriod;
extern unsigned long ulNVData[3];

/*********************************************************************************************
 * Initialize
 * ******************************************************************************************/
void Initialize(void){
	InitClock();
	InitGPIO();
	//InitTimer0();
	InitGPIOInt();
	//InitHibernation();
}

/*********************************************************************************************
 * InitClock
 * ******************************************************************************************/
void InitClock(void){
	//
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	// Delay values
	_MSDELAY_BASE = (SysCtlClockGet()/3)/1000;
	_USDELAY_BASE = _MSDELAY_BASE/1000;

}
/*********************************************************************************************
 * InitGPIO
 * ******************************************************************************************/
void InitGPIO(void){
	//
	// IHM
	//
	//Salidas
	//Habilito el puerto C
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlDelay(10);
	//PC5,PC6PC7 como entradas
	GPIOPinTypeGPIOInput(SW_PORT, SW_ON|SW_SD|SW_1);
	//Enable pull-up resistor
	GPIOPadConfigSet(SW_PORT,SW_ON,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(SW_PORT,SW_SD,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(SW_PORT,SW_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
	//
	//Entradas
	//Habilito el puerto E
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlDelay(10);
	//PE0,PE1 y PE2 salidas
	GPIOPinTypeGPIOOutput(LED_PORT, LED_ON|LED_1|LED_2);
}

/*********************************************************************************************
 * InitTimer0
 * ******************************************************************************************/
void InitTimer0(void){
	// Timer0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE,TIMER_CFG_32_BIT_PER);
	// Period
	ulPeriod=(SysCtlClockGet()/TOGGLE_FREQUENCY)/2;
	TimerLoadSet(TIMER0_BASE,TIMER_A,ulPeriod-1);
	// Enable interrupts
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
	IntMasterEnable();
}

/*********************************************************************************************
 * InitGPIOInt
 * ******************************************************************************************/
void InitGPIOInt(){
	//
	// Interrupt using GPIO
	//
	GPIOIntTypeSet(SW_PORT, SW_1, GPIO_RISING_EDGE );
	GPIOPinIntEnable(SW_PORT,SW_1);
	IntMasterEnable();
}

/*********************************************************************************************
 * InitHibernation
 * ******************************************************************************************/
void InitHibernation(void){
	// Enable Hibernate module
	SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
	// Clock source
	HibernateEnableExpClk(SysCtlClockGet());
}

/*********************************************************************************************
 * InitHibernation
 * ******************************************************************************************/
void DataLoggingON(unsigned long *Status){
	// Hibernate clock
	HibernateEnableExpClk(SysCtlClockGet());
	//Retencion de estado de los pines
	HibernateGPIORetentionEnable();
	//HibernateClockConfig(HIBERNATE_OSC_LOWDRIVE);
	SysCtlDelay(SysCtlClockGet()/10);
	//HibernateRTCTrimSet(0x7FFF);
	HibernateRTCEnable();
	//RTC
	HibernateRTCSet(0);
	HibernateRTCMatch0Set(5);
	//HibernateRTCMatch0Set(HibernateRTCGet() + 30);
	//Condiciones de wake
	HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);
	// Recupero info
	HibernateDataSet(ulNVData, 2);
	//Interrupciones
	HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_PIN_WAKE);
	// Clear any pending status.
	*Status = HibernateIntStatus(0);
	HibernateIntClear(*Status);
	//
	HibernateIntClear(HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_LOW_BAT | HIBERNATE_INT_RTC_MATCH_0);
	HibernateIntRegister(Hibernate_IRQHandler);
	//
	//Hibernamos
	//
	HibernateRequest();
}
/*********************************************************************************************
 * InitHibernation
 * ******************************************************************************************/
void DataLoggingOFF(void){
	// Hibernation module disabled
	HibernateDisable();
}
