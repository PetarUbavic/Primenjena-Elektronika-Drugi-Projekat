#include <stdio.h>
#include "driverGLCD.h"
#include <stdlib.h>
#include "adc.h"
#include "Tajmeri.h"
#include "timer2.h"
#include "timer3.h"
#include <stdbool.h> 



_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal 10MHz
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

const unsigned int AD_Xmin = 220;
const unsigned int AD_Xmax = 3642;
const unsigned int AD_Ymin = 520;
const unsigned int AD_Ymax = 3450;

unsigned int sirovi0,sirovi1;
unsigned int temp0,temp1;

unsigned int X, Y, x_vrednost, y_vrednost;

unsigned int pir, mq3, foto, enpir, enfoto, enmq3;
unsigned int broj1;

unsigned int brojac_ms, stoperica, ms, sekund;
unsigned int brojac_ms3, stoperica3, ms3, sekund3;

int korak = pocetna/5;
int vreme_on = pocetna;
int vreme_off = pocetna;

unsigned int x, n;
unsigned int stanje, taster;

unsigned char zakljucan = 0;
unsigned char tempRX;

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
    U1MODEbits.ALTIO=0; //biramo koje pinove koristimo za komunikaciju osnovne ili alternativne, koristimo alternativne

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
	sirovi0=ADCBUF2;
	sirovi1=ADCBUF3;

	temp0=sirovi0; //sirovi0 i sirovi1 su vrednosti sa TouchScreen-a
	temp1=sirovi1; //cuvaju se u temp0 i temp1 kako bi se dalje njima manipulisalo
       
	pir=ADCBUF0;
	mq3=ADCBUF1;
    foto=ADCBUF4;									

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

void ispisiPir(unsigned int pir) // ispisuje na terminal vrednost AD konverzije sa PIR-a
{
    RS232_putst("  PIR: ");
    WriteUART1dec2string(pir);
	for(broj1=0;broj1<1000;broj1++);
}

void ispisiMq3(unsigned int mq3) // ispisuje na terminal vrednost AD konverzije sa MQ3
{
    RS232_putst("  MQ3: ");
	WriteUART1dec2string(mq3);
    for(broj1=0;broj1<1000;broj1++);
}

void ispisiFoto(unsigned int foto) // ispisuje na terminal vrednost AD konverzije sa fotootoprnika
{
    RS232_putst("  FOTO: ");
    WriteUART1dec2string(foto);
	for(broj1=0;broj1<1000;broj1++);
}


/*
 * 
 */


int main(int argc, char** argv) {
        
        		
        for(broj1=0;broj1<60000;broj1++); 
        
        ConfigureADCPins(); //podesi pinove za AD konverziju
        initUART1(); //inicijalizacija UART-a
 		ADCinit(); //inicijalizacija AD konvertora
        
		ADCON1bits.ADON=1;//pocetak Ad konverzije 
        
        Init_T2(); //inicijalizuj tajmer 2
        Init_T3(); //inicijalizuj tajmer 3
        
	while(1)
	{ 
        
      
        
        
    }//od whilea

    return (EXIT_SUCCESS);
}