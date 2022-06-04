/*
 * File:  P_maestro.c
 * Autor: Eduardo Rubin
 *
 * Creado el 24 de mayo de 2022, 10:20
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000           // Frecuencia del oscilador
#define BOTON      PORTBbits.RB0     // Asignamos un alias a RB0
/*------------------------------------------------------------------------------
 * VARIABLES
 ------------------------------------------------------------------------------*/

int  POT3R, POT4R, POT5R, POT6R, FLAG_POT = 0, POT_temp = 0, POT_temp2;
char mode[2] = {0x01, 0x02};    //Para cambiar de modo
int  m = 0;
unsigned short map(uint8_t val, uint8_t in_min, uint8_t in_max, unsigned short out_min, unsigned short out_max);

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    
    if (PIR1bits.SSPIF){                     // Revisar si fue interrupcion de SPI
 
    if(PORTE == 0x01){    
        if (FLAG_POT == 0){
        POT3R  = SSPBUF;                      // SSPBUF a valor
        PORTD  = POT3R;
        CCPR1L = (POT3R>>3)+ 7;              // Corriento de ADRESH de 3 bit (0-32) -- 7 (bits mas sig. controlen el mov. total del servo)
        
        FLAG_POT = 1;}

        else if (FLAG_POT == 1){
        POT4R  = SSPBUF;                      // SSPBUF a valor
        PORTD  = POT4R;
        CCPR2L = (POT4R>>3)+ 7;              // Corriento de ADRESH de 3 bit (0-32) -- 7 (bits mas sig. controlen el mov. total del servo)
            
        FLAG_POT = 0;}}
    
    else if(PORTE == 0x02){
    
        POT_temp = SSPBUF;
        //PORTD = SSPBUF;
        POT_temp2 = POT_temp & 0xC0;
        
        if(POT_temp2 == 0x0){
        
        POT5R  = POT_temp & 0b00111111;                      // SSPBUF a valor
        //PORTD  = POT5R;
        CCPR1L = map(POT5R, 0, 63, 7, 39); 
        }
        else if(POT_temp2 == 0x40){
        
        POT6R  = POT_temp & 0b00111111;                      // SSPBUF a valor
        //PORTD  = POT6R;
        CCPR2L = map(POT6R, 0, 63, 7, 39);  
        }}
    
    PIR1bits.SSPIF = 0;
    }
    
    else if(INTCONbits.RBIF){            // Fue interrupción del PORTB
        if(!BOTON){                 // Verificamos si fue RB0 quien generó la interrupción
            m++;
            if(m>1){m=0;}
            PORTE = mode[m];}       // Incremento del PORTC (INCF PORTC)

        INTCONbits.RBIF = 0;    // Limpiamos bandera de interrupción
    }
    
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){}
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    
    // Configuración de entradas y salidas
    ANSEL  = 0;         // Emtradas para los Potenciometros
    ANSELH = 0;                 // Las demas entradas AN no se utilizaran

    TRISA = 0x20;               // Puerto A del maestro conectados a POTS
    PORTA = 0;                  // Limpiar el resto de bits

    TRISB = 0xFF;               // Puerto B como entradas para los botones

    TRISD = 0;                  // Puerto D como salidas para mostrar alguna variable
    PORTD = 0;                  // Limpiar el puerto

    TRISE = 0;                  // Puerto E como salidas para mostrar el modo
    PORTE = 1;

    //Configuarcion interrupciones
    INTCONbits.PEIE  = 1;           // Interrupciones perifericas
    INTCONbits.GIE   = 1;           // Interrupciones globales

    //Configuracion oscilador
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno

    //Comunicacion SPI de esclavo
    TRISC = 0b00011000; // -> SDI y SCK entradas, SD0 como salida
    PORTC = 0;

    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0100;   // -> SPI Esclavo, SS hablitado
    SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj

    PIR1bits.SSPIF = 0;         // Limpiamos bandera de SPI
    PIE1bits.SSPIE = 1;         // Habilitamos int. de SPI

    //Interrupcion del puerto B
    OPTION_REGbits.nRBPU = 0;       // Habilitamos resistencias de pull-up del PORTB

    WPUBbits.WPUB0  = 1;            // Resistencia de pull-up de RB0

    INTCONbits.RBIE = 1;            // Habilitamos interrupciones del PORTB
    IOCBbits.IOCB0  = 1;            // Habilitamos interrupción por cambio de estado para RB0
    INTCONbits.RBIF = 0;            // Limpiamos bandera de interrupción

    
    //Configuracion del PWM
    TRISCbits.TRISC1   = 1;         // RC1 como entrada
    TRISCbits.TRISC2   = 1;         // RC2 como entrada

    PR2 = 62;                       // Configurar el periodo
    CCP1CONbits.P1M    = 0;
    CCP1CONbits.CCP1M  = 0b1100;    // Habilitar CCP1 en modo PWM
    CCP2CONbits.CCP2M  = 0b1100;    // Habilitar CCP2 en modo PWM

    CCPR1L = 0x0f;                  // ciclo de trabajo
    CCP1CONbits.DC1B   = 0;
    PIR1bits.TMR2IF    = 0;         // apagar la bandera de interrupcion
    T2CONbits.T2CKPS   = 0b11;      // prescaler de 1:16
    T2CONbits.TMR2ON   = 1;         // prescaler de 1:16

    while(PIR1bits.TMR2IF == 0);    // esperar un ciclo del TMR2
    PIR1bits.TMR2IF    = 0;

    TRISCbits.TRISC2   = 0;         // Salida del PWM2
    TRISCbits.TRISC1   = 0;         // Salida del PWM1
    return;
}

unsigned short map(uint8_t x, uint8_t x0, uint8_t x1, unsigned short y0, unsigned short y1){
 return 
         (unsigned short)(y0+((float)(y1-y0)/(x1-x0))*(x-x0));
}