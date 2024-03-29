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
#define echo_levo1 PORTAbits.RA11

#define trig_napred1 LATBbits.LATB3
#define echo_napred11 PORTDbits.RD9



_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal 10MHz
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

unsigned int analogni, echo;
unsigned int flaglevo1, flagnapred1;
unsigned int brojanjeInt, brojanjeInt2;

unsigned int broj1, broj2;

unsigned int brojac;

unsigned int brojac_ms, stoperica, ms, sekund;
unsigned int brojac_ms3, stoperica3, ms3, sekund3;

unsigned int brojac3;

static unsigned int broj = 0;

unsigned int x, n;
unsigned int stanje, taster;
double  vremeGlobal, vremeGlobal2;
unsigned char tempRX, tempRX2;
int br1 = 0;
int rec[5];
char rec2[5];


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

void initUART2(void)
{
    U2BRG=0x0015;//ovim odredjujemo baudrate

    //U2MODEbits.ALTIO=1;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne

    IEC1bits.U2RXIE=1;//omogucavamo rx1 interupt

    U2STA&=0xfffc;

    U2MODEbits.UARTEN=1;//ukljucujemo ovaj modul

    U2STAbits.UTXEN=1;//ukljucujemo predaju
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

void WriteUART2(unsigned int data)
{
	while(!U2STAbits.TRMT);

    if(U2MODEbits.PDSEL == 3)
        U2TXREG = data;
    else
        U2TXREG = data & 0xFF;
}


void RS232_putst(register const char*str)
{
    while(*str)
    {
        WriteUART1(*str++);
       // WriteUART1(10);
    }
}

void RS232_putst2(const char *s)
{
    while(*s)
    {
        WriteUART2(*s++);
        //WriteUART2(10);
    }
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

void __attribute__((__interrupt__)) _U2RXInterrupt(void) 
{
    IFS1bits.U2RXIF = 0;
    tempRX2 = U2RXREG;
    
    if(tempRX2 != 0)
    {  
       rec2[n] = tempRX2;
       if(n < 5)
           n++;
       else n=0;
    }

} 

void __attribute__((__interrupt__, no_auto_psv)) _ADCInterrupt(void) 
{   
	analogni=ADCBUF0;
    //echo = ADCBUF1;
	
    IFS0bits.ADIF = 0;
} 

/*void __attribute__ ((__interrupt__, no_auto_psv)) _T2Interrupt(void) // svakih 1ms
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

*/void __attribute__ ((__interrupt__, no_auto_psv)) _T3Interrupt(void) // svakih 1ms
{
	//TMR3 = 0;
    
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


void __attribute__ ((__interrupt__, no_auto_psv)) _INT0Interrupt(void)
{
    
    brojanjeInt++;
    if(brojanjeInt % 2 == 0)//rastuca ivica
    {
        flaglevo1 = 1;
        T3CONbits.TON=1;
        INTCON2bits.INT0EP = 1;
    }
    
    else if(brojanjeInt % 2 == 1)//opadajuca ivica
    {
        vremeGlobal=TMR3;
        flaglevo1 = 0;
        TMR3=0;
        T3CONbits.TON=0;
        INTCON2bits.INT0EP = 0;
    }  
    IFS0bits.INT0IF = 0;
}

void __attribute__ ((__interrupt__, no_auto_psv)) _INT2Interrupt(void)
{
    
    brojanjeInt2++;
    if(brojanjeInt2 % 2 == 0)//rastuca ivica
    {
        flagnapred1 = 1;
        T3CONbits.TON=1;
        INTCON2bits.INT2EP = 1;
    }
    
    else if(brojanjeInt2 % 2 == 1)//opadajuca ivica
    {
        vremeGlobal2=TMR3;
        flagnapred1 = 0;
        TMR3=0;
        T3CONbits.TON=0;
        INTCON2bits.INT2EP = 0;
    }
    IFS1bits.INT2IF = 0;
}



void __attribute__((__interrupt__)) _T2Interrupt(void) // pwm
{
    TMR2 = 0;
    IFS0bits.T2IF = 0;
}

void __attribute__((__interrupt__)) _T1Interrupt(void)  // 10us
{
    TMR1 = 0;
    br1++;    
    IFS0bits.T1IF = 0;
}

/*void __attribute__((__interrupt__)) _T3Interrupt(void)  // 5ms
{
    TMR3 = 0;
    brojac3++;
    IFS0bits.T3IF = 0;
}*/


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
* Ime funkcije      : WriteUART1dec2string--WriteUART2dec2string                 		   *
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

void WriteUART2dec2string(unsigned int data)
{
	unsigned char temp;
    temp=data/10000;
	WriteUART2(temp+'0');
	data=data-temp*10000;
	temp=data/1000;
	WriteUART2(temp+'0');
	data=data-temp*1000;
	temp=data/100;
	WriteUART2(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART2(temp+'0');
	data=data-temp*10;
	WriteUART2(data+'0');
}


void ispisiAnalogni(unsigned int analogni) // ispisuje na terminal vrednost AD konverzije sa Analognog senzora
{
    RS232_putst("  Analogni: ");
    WriteUART1dec2string(analogni);
	for(broj1=0;broj1<1000;broj1++);
}

void ispisiDistancu(unsigned int analogni) //ispis HC senzora
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
    LATFbits.LATF0=0; //za smer DESNOG motora
    LATFbits.LATF1=1; // motori gone unazad
        
    LATBbits.LATB10=0; //za smer motora
    LATBbits.LATB11=1; //motori gone unazad
    
    PWM1(400);
    PWM2(400);
    //dodati ispis na uart 
    //RS232_putst2("Idem pravo");
}

void skreniDesno()
{
    LATFbits.LATF0=1; //za smer motora
    LATFbits.LATF1=0; // motor goni unapred
                
    LATBbits.LATB10=0; //za smer motora
    LATBbits.LATB11=1; //motori gone unazad0
    
    PWM1(400);
    PWM2(400);
    //dodati ispis na uart 
    //RS232_putst2("Skrecem desno");
}

void skreniLevo()
{
    LATFbits.LATF0=0; //za smer motora
    LATFbits.LATF1=1; // motor goni unapred
                
    LATBbits.LATB10=1; //za smer motora
    LATBbits.LATB11=0; //motori gone unazad0
    
    PWM1(400);
    PWM2(400);
    //dodati ispis na uart 
    //RS232_putst2("Skrecem levo");
}



void stani()
{
    LATFbits.LATF0=1; //za smer motora
    LATFbits.LATF1=0; // motori gone unazad
                
    LATBbits.LATB10=0; //za smer motora
    LATBbits.LATB11=1; //motori gone unazad
    
    PWM1(0);
    PWM2(0);
    //dodati ispis na uart 
    //RS232_putst2("Stao sam");
}

 //SENZORI

double OcitajLevo1()
{
    
    double vreme1=0;
    double distanca1=0;
    //brojac_ms3=0;
    //sekund=0;
    //RS232_putst("Pre svega");
    trig_levo1 = 0;  //GENERISANJE TRIG SIGNALA
    delay_us(1);
    trig_levo1 = 1;
    delay_us(1);
    trig_levo1 = 0;
    //RS232_putst("Pre petlje");
    
    vreme1=vremeGlobal;
    
    //vreme1 = brojac_ms3 + sekund*1000;
     //WriteUART1dec2string(vreme1);
     //WriteUART1(13);
     vreme1=vreme1/10;
     distanca1 = vreme1 * 0.034 / 2;
     //RS232_putst("OcitajLevo  ");
     //WriteUART1dec2string(distanca1);
     //WriteUART1(13);
    
    
     
    delay_for();           
    T3CONbits.TON = 0; 
    
    return distanca1;
}

double OcitajPravo1()
{
    
    double vreme2=0;
    double distanca2=0;
    //brojac_ms3=0;
    //sekund=0;
    //RS232_putst("Pre svega");
    trig_napred1 = 0;  //GENERISANJE TRIG SIGNALA
    delay_us(1);
    trig_napred1 = 1;
    delay_us(1);
    trig_napred1 = 0;
    //RS232_putst("Pre petlje");
    
    vreme2=vremeGlobal2;
    
    //vreme1 = brojac_ms3 + sekund*1000;
     //WriteUART1dec2string(vreme1);
     //WriteUART1(13);
     vreme2=vreme2/10;
     distanca2 = vreme2 * 0.034 / 2;
     //RS232_putst("OcitajPravo   ");
     //WriteUART1dec2string(distanca2);
     //WriteUART1(13);
    
    
     
    delay_for();           
    T3CONbits.TON = 0; 
    
    return distanca2;
}


int main(int argc, char** argv) {
        
        		
        for(broj1=0;broj1<60000;broj1++); 
        
        
        //INTCON1bits.GIE = 1; // dozvola globalnog interapta
        IEC0bits.INT0IE = 1; //dozvola za interupt na INT0
        IFS0bits.INT0IF = 0; //clear external interrupt 0 flag
        
        IEC1bits.INT2IE = 1; //dozvola za interupt na INT0
        IFS1bits.INT2IF = 0; //clear external interrupt 0 flag
        
        //IPC9bits.INT2IP=5; //interupt priority
        INTCON2bits.INT2EP=0;//triger on rising edge
        
        
        
        LATFbits.LATF0=0; //za smer DESNOG motora
        LATFbits.LATF1=1; // motori gone unazad
        
        LATBbits.LATB10=0; //za smer motora
        LATBbits.LATB11=1; //motori gone unazad
        
        
        ConfigureADCPins(); //podesi pinove za AD konverziju
        initUART1(); //inicijalizacija UART-a
 		ADCinit(); //inicijalizacija AD konvertora
        
		ADCON1bits.ADON=1;//pocetak Ad konverzije 
        
        flaglevo1 = 0;
        flagnapred1 = 0;
        
        Init_T1();
        Init_T2(); //inicijalizuj tajmer 2
        initUART2();
        Init_T3(); //inicijalizuj tajmer 3
        
	while(1)
	{ 
      /*  for(broj1=0; broj<5; broj++)
        {
            WriteUART2(rec2[broj]);
        }
        
        RS232_putst2("DA");
        WriteUART2(13);*/
        
       
       
       
        
        if(broj == 0)
        {
            analogni = 0;
            vremeGlobal = 0;
            vremeGlobal2=0;
            
            LATFbits.LATF0=0; //za smer DESNOG motora
            LATFbits.LATF1=0; // motori gone unazad
        
            LATBbits.LATB10=0; //za smer motora
            LATBbits.LATB11=0; //motori gone unazad
            
            broj++;
        }
        
         
        
        //RS232_putst2("Cekam POCETAK");
        
        //if (rec2[0]=='S' && rec2[1]=='T' && rec2[2]=='A' && rec2[3]=='R' && rec2[4]=='T')
        //{
        
        //OcitajLevo1();
        //pravo();
        
        //OcitajPravo1();
        //WriteUART1(13);
        //OcitajLevo1();
        //WriteUART1(13);
         
        if (OcitajPravo1()<15 && OcitajPravo1()>5) // >1000
        {
            stani();
            //RS232_putst2("STAO");
            //WriteUART2(83);
            
            for(broj2=0; broj2 < 4; broj2++)
                for(broj1=0; broj1<40000; broj1++)
                {
                    skreniLevo();
                    //RS232_putst2("LEVO");
                    //WriteUART2(68);
                }
            stani();
            //RS232_putst2("STAO");
            //WriteUART2(83);
        }        
        
        else 
        {    
            if (OcitajLevo1()<15 && OcitajLevo1()>5)
            {
                pravo();
                //RS232_putst2("PRAVO");
                //WriteUART2(80);
            }
            else if (OcitajLevo1()>15)  //>1000 //ocitaj levo je u stvari ocitaj desno
            {
                stani();
                //RS232_putst2("STAO");
                //WriteUART2(83);
                for(broj2=0; broj2 < 4; broj2++)
                     for(broj1=0; broj1<45000; broj1++)
                     {
                        skreniDesno();
                        //RS232_putst2("DESNO");
                        //WriteUART2(68);//L 80 P 68 D 83 S
                     }
                
                
                 
            }
        }
        
       // }//OD start if-a
    }//od whilea
    return (EXIT_SUCCESS);
}