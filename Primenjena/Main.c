#include <stdio.h>
#include "driverGLCD.h"
#include <stdlib.h>
#include "adc.h"
#include "Tajmeri.h"
#include "timer2.h"
#include "timer3.h"
#include <stdbool.h> 
#include "outcompare.h"

#define trig_levo1 LATBbits.LATB2
#define echo_levo1 PORTBbits.RB3



_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal 10MHz
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

unsigned int analogni;

unsigned int broj1;

unsigned int brojac_ms, stoperica, ms, sekund;
unsigned int brojac_ms3, stoperica3, ms3, sekund3;

unsigned int x, n;
unsigned int stanje, taster;

unsigned char zakljucan = 0;
unsigned char tempRX;
int br1 = 0;
int rec[5];


/***************************************************************************
* Ime funkcije      : initUART1                                            *
* Opis              : inicjalizuje RS232 komunikaciju s 28800 bauda        * 
* Parameteri        : Nema                                                 *
* Povratna vrednost : Nema                                                 *
***************************************************************************/
void initUART1(void)
{
    //OVO JE KOPIRANO IZ Touch screen.X
    U1BRG=0x0015; //ovim odredjujemo baudrate
    U1MODEbits.ALTIO=1; //biramo koje pinove koristimo za komunikaciju osnovne ili alternativne, koristimo alternativne

    IEC0bits.U1RXIE = 1;
    U1STA&=0xfffc;
    U1MODEbits.UARTEN=1;
    U1STAbits.UTXEN=1;
}

void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void) 
{
    IFS0bits.U1RXIF = 0;
    tempRX=U1RXREG;
    
    if(tempRX != 0)
    {  
       rec[n] = tempRX;
       if(n < 5)
           n++;
       else n=0;
    }
} 

void __attribute__((__interrupt__, no_auto_psv)) _ADCInterrupt(void) 
{   
	analogni=ADCBUF0;
	
    IFS0bits.ADIF = 0;
} 

void __attribute__ ((__interrupt__, no_auto_psv)) _T2Interrupt(void) // svakih 1ms
{
	TMR2 =0;
     ms=1;//fleg za milisekundu ili prekid;potrebno ga je samo resetovati u funkciji

	brojac_ms++;//brojac milisekundi
    stoperica++;//brojac za funkciju Delay_ms

    if (brojac_ms==1000)//sek
        {
          brojac_ms=0;
          sekund=1;//fleg za sekundu
		 } 
	IFS0bits.T2IF = 0;    
}

void __attribute__ ((__interrupt__, no_auto_psv)) _T3Interrupt(void) // svakih 1ms
{
	TMR3 = 0;
    ms3 = 1; //fleg za milisekundu ili prekid ; potrebno ga je samo resetovati u funkciji

	brojac_ms3++; //brojac milisekundi
    stoperica3++; //brojac za funkciju Delay_ms3

    if (brojac_ms3 == 1000) //sek
        {
            brojac_ms3 = 0;
            sekund3 = 1; //fleg za sekundu
		} 
    
	IFS0bits.T3IF = 0;    
}

void Delay_ms (int vreme)//funkcija za kasnjenje u milisekundama(nije milisekundama menjali smo)
{
    stoperica = 0;
	while(stoperica < vreme);
}

void Delay_ms3 (int vreme)//funkcija za kasnjenje u milisekundama (nije milisekundama menjali smo)
{
	stoperica3 = 0;
	while(stoperica3 < vreme);
}

/*********************************************************************
* Ime funkcije      : WriteUART1                            		 *
* Opis              : Funkcija upisuje podatke u registar U1TXREG,   *
*                     za slanje podataka    						 *
* Parameteri        : unsigned int data-podatak koji zelimo poslati  *
* Povratna vrednost : Nema                                           *
*********************************************************************/
void WriteUART1(unsigned int data)
{
	while (U1STAbits.TRMT==0);
    if(U1MODEbits.PDSEL == 4) //3
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}

void RS232_putst(register const char*str)
{
    while((*str)!=0)
    {
        WriteUART1(*str);
        str++;
    }
}

void delay_us (unsigned int mikrosek)  //za senzore 10us
{
    br1 = 0;
    while(br1  < mikrosek);
}

void delay_for(void)  //za senzore 20ms
{
    int i, j;
    for (i = 0; i < 200; i++)
        for (j = 0; j < 100; j++);
}

/***********************************************************************
* Ime funkcije      : WriteUART1dec2string                     		   *
* Opis              : Funkcija salje 4-cifrene brojeve (cifru po cifru)*
* Parameteri        : unsigned int data-podatak koji zelimo poslati    *
* Povratna vrednost : Nema                                             *
************************************************************************/
void WriteUART1dec2string(unsigned int data)
{
	unsigned char temp;

	temp=data/1000;
	WriteUART1(temp+'0');
	data=data-temp*1000;
	temp=data/100;
	WriteUART1(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}

void ispisiAnalogni(unsigned int analogni) // ispisuje na terminal vrednost AD konverzije sa Analognog senzora
{
    RS232_putst("  Analogni: ");
    WriteUART1dec2string(analogni);
	for(broj1=0;broj1<1000;broj1++);
}

void ispisiDistancu(unsigned int analogni) // ispisuje na terminal vrednost AD konverzije sa Analognog senzora
{
    RS232_putst("  Distanca: ");
    WriteUART1dec2string(analogni);
	for(broj1=0;broj1<1000;broj1++);
}



void PWM1(int dutyC)
{
    PR2 = 500;//odredjuje frekvenciju po formuli
    OC1RS = dutyC;//postavimo pwm
    OC1R = 1000;//inicijalni pwm pri paljenju samo
    OC1CON  = OC_IDLE_CON & OC_TIMER2_SRC & OC_PWM_FAULT_PIN_DISABLE& T2_PS_1_256;//konfiguracija pwma
                   
    T2CONbits.TON = 1;//ukljucujemo timer koji koristi

 
}

void PWM2(int dutyC)
{
    PR2 = 500;//odredjuje frekvenciju po formuli
    OC2RS = dutyC;//postavimo pwm
    OC2R = 1000;//inicijalni pwm pri paljenju samo
    OC2CON = OC_IDLE_CON & OC_TIMER2_SRC & OC_PWM_FAULT_PIN_DISABLE& T2_PS_1_256;//konfiguracija pwma
             
    T2CONbits.TON = 1;//ukljucujemo timer koji koristi

}



/*
 * 
 */

void pravo()
{
    PWM1(400);
    PWM2(400);
}

void skreniDesno()
{
    PWM1(0);
    PWM2(400);
}

void skreniLevo()
{
    PWM1(400);
    PWM2(0);
}

void stani()
{
    PWM1(0);
    PWM2(0);
}
 //SENZORI
int OcitajLevo1()
{
    int vreme1;
    int distanca1;
    
    trig_levo1 = 0;  //GENERISANJE TRIG SIGNALA
    delay_us(1);
    trig_levo1 = 1;
    delay_us(1);
    trig_levo1 = 0;
    while (echo_levo1 != 0);
    T3CONbits.TON = 1;
    TMR3 = 0;            
    while (echo_levo1 == 1 && TMR3 < 40000);
    vreme1 = TMR3 / 10 ;
    distanca1 = vreme1 * 0.034 / 2;
    delay_for();           
    T3CONbits.TON = 0; 
    
    return distanca1;
}

int main(int argc, char** argv) {
        
        		
        for(broj1=0;broj1<60000;broj1++); 
        
        LATFbits.LATF0=0; //za smer motora
        LATFbits.LATF1=1; // motori gone unazad
        
        LATBbits.LATB10=0; //za smer motora
        LATBbits.LATB11=1;//motori gone unazad
        
        
        ConfigureADCPins(); //podesi pinove za AD konverziju
        initUART1(); //inicijalizacija UART-a
 		ADCinit(); //inicijalizacija AD konvertora
        
		ADCON1bits.ADON=1;//pocetak Ad konverzije 
        
        Init_T2(); //inicijalizuj tajmer 2
        Init_T3(); //inicijalizuj tajmer 3
        
	while(1)
	{ 
        //OcitajLevo1();
        ispisiAnalogni(analogni);
        
        ispisiDistancu(OcitajLevo1());
        WriteUART1(13);
        for(broj1=0; broj1<10000; broj1++);
        /*if(analogni>700)
           // skreniLevo();
        else
            pravo();
        */
        
        
        //LATDbits.LATD0=1;
        //LATDbits.LATD1=1;
        
        
       
        
    }//od whilea

    return (EXIT_SUCCESS);
}