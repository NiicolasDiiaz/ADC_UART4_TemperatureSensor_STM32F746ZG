#include <stdio.h>
#include "STM32F7xx.h"

int j=0;
int conteo=0;   //lleva el conteo de los canales de conversion del ADC
int conteo1=0;  //lleva el conteo de los datos recibidos del serial

bool bandera=0;
int dato_referencia = 30;   //temperatura de referencia 30º

char envio = 0x31;
char envio1[] = {"TEMPERATURA SENSORES = "};
char envio2[] = {"PROMEDIO TEMPERATURA SENSORES = "};
char canales[8] = {0x9,0xE,0xF,0x4,0x5,0x6,0x7,0x8};

//***********************************************************************************************************

long dato_adc1=0;
int dato_adc[3]={0x30,0x30,0x30};    ////vector para guardar los 3 datos de los sensores
int dato_promedio=0;

int dato_serial[3]={0x30,0x30,0x30};

// (3,3V/1024)*(1ºC/10mV)*V_ADC= Temperatura en ºC

int res = 32226;     // (3,3V/1024)*(100) = 3300/1024  =  3,2226  =   32226 * 10^-4
										 //3,2226 * V_ADC = Temperatura en ºC
										 //32226 * 10^-4 * V_ADC = Temperatura en ºC

char dato1 =0x30;
char dato2 =0x30;
char dato3 =0x30;
char dato4 =0x30;
char dato5 =0x30;

extern "C" {
	
	void UART4_IRQHandler(void){      //Interrupcion por dato recibido.
		if (UART4->ISR & 0x20) {  			// evaluar si la bandera RXNE esta activa		
				dato_serial[conteo1] = (UART4->RDR - 0x30);	//Quito 0x30 para obtener el dato original y lo guardo
			  conteo1++;
					if (conteo1 ==3){
						conteo1=0;
						bandera=1;
						GPIOB->ODR ^= 0x1;   //blink cada vez que recibe 3 digitos del PC  
				  }
		}		
	}

	void ADC_IRQHandler(void){         //Interrupcion por final de conversion.
		dato_adc[conteo] = ADC3->DR;	   //lee el dato
		conteo = conteo +1;
		if (conteo ==3){
			conteo=0;
		}		
	}
	
}


void send(){
	short i=0;
	for (i=0;envio!='=';i++){
	    envio = envio1[i];
	    UART4->TDR = envio;
			while ((UART4->ISR &= 0x80)==0);   //esperar hasta que TXE sea 1, lo que indica que se envio el dato
	}
	envio='1';
}

void send2(){
	short i=0;
	for (i=0;envio!='=';i++){
	    envio = envio2[i];
	    UART4->TDR = envio;
			while ((UART4->ISR &= 0x80)==0);   //esperar hasta que TXE sea 1, lo que indica que se envio el dato
	}
	envio='1';
}



void send_dato(char dt){
	UART4->TDR = dt;
	while ((UART4->ISR &= 0x80)==0);   //esperar hasta que TXE sea 1, lo que indica que se envio el dato
}


void conv_adc(){
	
	ADC3->SQR3 = canales[conteo];     //Define el orden de las conversiones //Primer CANAL
	ADC3->CR2 |= (1UL << 30);        //inicio conversión (set el bit SWSTART)
	while ((ADC3->SR &= 0x2)==1);   //esperar hasta que EOC sea 1 (termino la conversión canal 1)
	
	for(int k=0; k<200000;k++){};    //delay

	ADC3->SQR3 = canales[conteo];     //Define el orden de las conversiones //Segundo CANAL
	ADC3->CR2 |= (1UL << 30);        //inicio conversión (set el bit SWSTART)
  while ((ADC3->SR &= 0x2)==1);   //esperar hasta que EOC sea 1 (termino la conversión canal 2)
	
	for(int k=0; k<200000;k++){};    //delay
	
  ADC3->SQR3 = canales[conteo];     //Define el orden de las conversiones	//Tercer CANAL	
	ADC3->CR2 |= (1UL << 30);        //inicio conversión (set el bit SWSTART)
	while ((ADC3->SR &= 0x2)==1);   //esperar hasta que EOC sea 1 (termino la conversión canal 3)
	
	for(int k=0; k<200000;k++){};    //delay
				
}


void conv_bcd_tx(int a){
	dato_adc1 = a*res;
	dato1 = dato_adc1/10000000;
	dato2 =(dato_adc1%10000000)/1000000;
	dato3 =(dato_adc1%1000000)/100000;
	dato4 =(dato_adc1%100000)/10000;
	dato5 =(dato_adc1%10000)/1000;	
	dato1 = dato1 + 0x30;
	dato2 = dato2 + 0x30;
	dato3 = dato3 + 0x30;
	dato4 = dato4 + 0x30;	
	dato5 = dato5 + 0x30;	
	
	send_dato(dato1);
	send_dato(dato2);
	send_dato(dato3);
	send_dato('.');
	send_dato(dato4);
	send_dato(dato5);
	send_dato(' ');
	send_dato('-');
	send_dato(' ');
}



int main(void)
{
	
	RCC->AHB1ENR =0xFF; //TODOS LOS RELOGES ON -> Puertos A,B,C,D,E,F,G,H
	
  GPIOC->MODER &=  ~(3UL << 2*13); //pulsador como entrada (PC13)
	
	//**********************************************************************
	//CONFIGURACION DE LED DE LA TARJETA
	GPIOB->MODER |= 0x10004001;       //PTB0 - PTB7 - PTB14 -> OUTPUT
	GPIOB->OTYPER = 0;               //PUSH PULL -> PTB0 - PTB7 - PTB14
	GPIOB->OSPEEDR |= 0x10004001;     //MEDIUM SPEEED -> PTB0 - PTB7 - PTB14
	GPIOB->PUPDR |= 0x10004001;       //PULL-UP -> PTB0 - PTB7 - PTB14
	

//********************************************************************************************
	//CONFIGURACION UART4
	RCC->APB1ENR |= 0x80000; // Enable clock for UART4 (set el pin 19 )
													 // 8N1-> M=00, no es necesario escribir el registro
	UART4->BRR = 0x683;      // 9600 Baudios, fclk=16Mhz, por defecto el HSI
  UART4->CR1 |= 0x2C;      // Tx habilitado, Rx habilitado, Interrup de Rx habilitado, 8N1 
													 // CR2 se deja por defecto pues 1 bit de stop es 00
		
	UART4->CR1 |= 0x1;       // Habilita UART, UE=1 
	
	
	NVIC_EnableIRQ(UART4_IRQn);      //Habilita interrupción del UART4 
	
	GPIOA->MODER |=  (2UL << 2*0);   //pines PA0 en modo alterno (TX)
  GPIOC->MODER |=  (2UL << 2*11);  //pines PC11 en modo alterno (RX)
	GPIOA->AFR[0] |= 0x8;            // PA0 -> AF8=UART4 TX, AF8=1000 en AFR0
	GPIOC->AFR[1] |= 0x8000;         // PC11 -> AF8=UART4 RX, AF8=1000 en AFR11

//********************************************************************************************	
	//CONFIGURACION ADC (en este caso ADC3)
	
	RCC->APB2ENR |= 0x400;   //Enable clock for the ADC3 (set el bit 10 =ADC3EN)

	GPIOF->MODER = 0x3FFFC0;	//configurar los pines como analógicos PF3 al PF10 (analogico = 11)
	
	ADC3->CR1 |= (1UL << 5);   	 //ADC3->CR1 se activa la interrupcion EOCIE
	ADC3->CR1 |= (1UL << 24);    //ADC a 10bits
	ADC3->CR2 |= (1UL << 0);   	//ADC3->CR2 - The ADC is powered on by setting the ADON bit in the ADC_CR2 register
	ADC3->CR2 |= (1UL << 10);   //Set to 1 the bit EOCS (The EOC bit is set in the ADC_SR register:
																											//At the end of each regular channel conversion) 
														
	ADC3->SQR3 = canales[0];     //Define el orden de las conversiones	
  NVIC_EnableIRQ(ADC_IRQn);      //Habilita interrupción del ADC
	
//*********************************************************************************************


	//bucle infinito
	while(true)
		{
						
			if(GPIOC->IDR &= 0x2000){        //evalua si se oprimio el pulsador  
				conv_adc();
				dato_promedio = (dato_adc[0] + dato_adc[1] + dato_adc[2])/3;
				
				//enviar datos de los 3 sensores
				send();
				conv_bcd_tx(dato_adc[0]);	
				conv_bcd_tx(dato_adc[1]);
				conv_bcd_tx(dato_adc[2]);
				send_dato(0xD);  //(Carriage Return)
				send_dato(0xA);	 //Line Feed
				
				//envia dato promedio
				send2();
				conv_bcd_tx(dato_promedio);
				send_dato(0xD);  //(Carriage Return)
				send_dato(0xA);	 //Line Feed
				
				//EVALUA SI EL PROMEDIO ES MAYOR QUE EL DATO ENVIADO POR EL SERIAL
				if(dato_adc1 > (dato_referencia*100000)){
					GPIOB->ODR |= (1UL<<14);    //enciende LED
				}else{
					GPIOB->ODR &= ~(1UL<<14);    //apaga LED
				}
				
				for(int k=0; k<200000;k++);    //delay
			}			
			
			//convierte en decimal el valor de temperatura recibido en ASCII por el serial (minimo 3 digitos)
			if(bandera==1){
				dato_referencia = dato_serial[0]*100 + dato_serial[1]*10 + dato_serial[2];
				bandera=0;
			}
					
		}//cierra while
}//cierra main