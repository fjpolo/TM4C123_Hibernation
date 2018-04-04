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
 *	main.c
 *
 *	Descripción:
 *
 *  Desarrollo del firmware de la placa base del data logger, constando de:
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
//

/*********************************************************************************************
 * Global variables
 * ******************************************************************************************/
//
int SWRead;
//
//Datos a mantener durante la hibernacion
//0: sensor_flag
//1: hibernate_flag
unsigned long ulNVData[3] = { 1, 0, 0};
// Estado del modulo hibernacion
unsigned long ulStatus;
unsigned long ulPeriod;

/*********************************************************************************************
 * GPIOPortC_IRQHandler
 * ******************************************************************************************/
void GPIOPortC_IRQHandler(void){
	// Clear interrupt source
	GPIOPinIntClear(SW_PORT, SW_1);
	//Flag de que estoy en la interrupcion por GPIO
	GPIOPinWrite(LED_PORT,LED_2, LED_2);//
	SysCtlDelay(8000000);
	GPIOPinWrite(LED_PORT,LED_2, 0);
	SysCtlDelay(4000000);
	// Flag
	ulNVData[1]=1;
}

/*****************************************************************************************************
 * Hibernate_IRQHandler
 ****************************************************************************************************/
void Hibernate_IRQHandler (void){
	// Clear interrupt sources
	HibernateIntClear( HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_LOW_BAT | HIBERNATE_INT_RTC_MATCH_0 );
	//Controlo mi bandera para sensar
	if(ulNVData[0]>=5){
		//Reset flag
		ulNVData[0]=1;
		//Flag de que mido cada 5 mediciones
		//LED
		GPIOPinWrite(LED_PORT, LED_1|LED_2, 0x06);
		SysCtlDelay(32000000);
		GPIOPinWrite(LED_PORT, LED_1|LED_2, 0x00);
	}
	else {
		//Aumento mi flag
		ulNVData[0]++;
		//Flag de que mido todas las interrupciones
		GPIOPinWrite(LED_PORT, LED_1, LED_1);
		SysCtlDelay(32000000);
		GPIOPinWrite(LED_PORT, LED_1, 0);
	}
	//Guardo mis variables importantes
	HibernateDataSet(ulNVData,2);
}
/*********************************************************************************************
 * Main loop
 *
 * Uso del modulo de hibernación. Se despierta despues de X tiempo o con SW_ON.
 * Agregada la interrupción.
 * Agregando las funciones en datalogger.c
 *
 ********************************************************************************************/
int main(void){
	//
	Initialize();
	InitHibernation();
	//
	IntEnable(INT_GPIOC);
	//
	if(HibernateIsActive())
	{
		//Leo el estado y determino la causa de despertarse
		ulStatus = HibernateIntStatus(false);
		//Controlo la razon de haberse despertado
		if(ulStatus & HIBERNATE_INT_RTC_MATCH_0)
		{
			//Se despierta de una hibernación
			//Flag de que estoy en despertando por RTC
			/*GPIOPinWrite(LED_PORT, LED_ON|LED_1|LED_2, 0x07);//
			SysCtlDelay(8000000);
			GPIOPinWrite(LED_PORT, LED_ON|LED_1|LED_2, 0x00);
			SysCtlDelay(4000000);*/
			//Recupero mis variables importantes
			HibernateDataGet(ulNVData, 2);
		}
		if (ulStatus & HIBERNATE_INT_PIN_WAKE){
			HibernateIntClear(HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_LOW_BAT | HIBERNATE_INT_RTC_MATCH_0);
			GPIOPinWrite(LED_PORT,LED_ON, LED_ON);//Red
			SysCtlDelay(8000000);
			GPIOPinWrite(LED_PORT,LED_ON, 0);
			SysCtlDelay(4000000);
			ulNVData[1]=0;
		}
		//En caso contrario, es un reset o power-on
	}
	//
	while(1){
		// Check hibernate_flag
		if(ulNVData[1] == 1) DataLoggingON(&ulStatus);
		else DataLoggingOFF();
	}
}
