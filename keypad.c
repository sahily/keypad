/*
 * keypad.c
 *
 * Created: 4/8/2019 7:45:46 PM
 * Author : Sahil S. Mahajan
 * Description: Keypad scanner. Displays value of key on 7seg
 *
 */

#include <REG51F.H>

void init(void);
void configureTimer0(void);
void enableInterrupt(void);
void resetTimer0(void);
void displayKeyValue(void);

sbit buzz = P1^5;
sbit sl1 = P2^0;
sbit sl2 = P2^1;
sbit sl3 = P2^2;
sbit sl4 = P2^3;
sbit krl1 = P2^4;
sbit krl2 = P2^5;
sbit krl3 = P2^6;
sbit krl4 = P2^7;

code unsigned int numberList[23] = {
                                     0xFC,    //0         
                                     0x60,    //1
                                     0xDA,    //2 
                                     0xF2,    //3
                                     0x66,    //4
                                     0xB6,    //5
                                     0xBE,    //6
                                     0xE0,    //7
                                     0xFE,    //8
                                     0xF6,    //9
                                     0x00,    //'9'+1 (clear)
                                     0xFF,    //3b
                                     0xFF,    //3c
                                     0xFF,    //3d
                                     0xFF,    //3e
                                     0xFF,    //3f
                                     0xFF,    //40
                                     0xEE,    //A
                                     0x3E,    //B
                                     0x9C,    //C
                                     0x7A,    //D
                                     0x9E,    //E
                                     0x8E,    //F
                                   };

unsigned char asciiTab[16] = {"0123456789ABCDEF"};

unsigned int numbersToDisplay[4] = {'0','0','0','0'};

unsigned char inputColumn[4] = {1,1,1,1};   /* used for scanner function. When value is low, */ 
                                            /* key on the column is selected                 */

unsigned int digitSelector[4][4] = {
                                      0,1,1,1,   //select 4th digit on 7seg display
                                      1,0,1,1,   //select 3rd digit on 7seg display
                                      1,1,0,1,   //select 2nd digit on 7seg display
                                      1,1,1,0    //select 1st digit on 7seg display
                                   };
                                   
unsigned int scanNumber;        //used with digitSelctor variable in display function to select digit
                                //  Also used for tracking keypad values
                                 
unsigned int x;                 //used in scanner function to determine whether key is pushed
bit keyPushConfirmed;           //set when it is determined key has been pushed
bit keyReleaseConfirmed;        //set when it is determined key has been released
unsigned int pushDetectedCount; //used to determine whether key is pushed after a push was detected
unsigned int pushReleasedCount; //used to determine whether key is released after a push was confirmed
unsigned char keyCode;          //the value of the key that was detected




void main(void)
{
    init();
    configureTimer0();
    enableInterrupt();

    while(1)
    {
        while(keyPushConfirmed == 0);
        displayKeyValue();
        while(keyReleaseConfirmed == 0);

        keyPushConfirmed = 0;       //reset values
        keyReleaseConfirmed = 0;
    }
}


/* -----------------
 * Function: init
 * -----------------
 *
 * Initializes variables, clears first three 7seg displays, configures timer mode, 
 *    and sets input pins for keypad
 *
 */

void init(void)
{
    scanNumber = 0;
    keyPushConfirmed = 0;
    keyReleaseConfirmed = 0;
    pushDetectedCount = 33;
    pushReleasedCount = 32;

    numbersToDisplay[3] = '9'+1;
    numbersToDisplay[2] = '9'+1;
    numbersToDisplay[1] = '9'+1;        //'9'+1 is 0x00 (clear)

    TMOD = 0x01;    //use Timer 0 in 16-bit Timer operating mode
    
    krl1 = inputColumn[0];
    krl2 = inputColumn[1];
    krl3 = inputColumn[2];
    krl4 = inputColumn[3];
}
 
                                   
                                   
/* -----------------
 * Function: configureTimer0
 * -----------------
 *
 * sets up Timer 0 and enables timer interrupt
 *
 */

void configureTimer0(void)
{
    resetTimer0();          //load timer0 start value
    TR0 = 1;                //start Timer 0
    ET0 = 1;                //enable Timer 0 overflow interrupt
}


/* -----------------
 * Function: enableInterrupt
 * -----------------
 *
 * enables all interrupts that has their individual interrupt bit enabled
 *
 */

void enableInterrupt(void)
{
    EA = 1;
}


/* -----------------
 * Function: resetTimer0
 * -----------------
 *
 * loads Timer 0's 16 bit count register with start value
 *
 */

void resetTimer0(void)
{
    TH0 = 0xFC;
    TL0 = 0x66;         //set to 1 millisecond  
}  


/* -----------------
 * Function: display
 * -----------------
 *
 * selects one digit on the 7-segment display at a time and displays a number on the selected digit
 *
 */

void display(void)
{

    P0 = 0x00;

    sl1 = digitSelector[scanNumber/4][0];           /* Digit is selected when its pin is reset.         */
    sl2 = digitSelector[scanNumber/4][1];           /* Only one of these is selected in each iteration  */
    sl3 = digitSelector[scanNumber/4][2];           /* The keypad is also connected to these pins.      */
    sl4 = digitSelector[scanNumber/4][3];           /* Each sl is connected to a keypad row. When row   */
                                                    /* is selected with 0, and key on the row is        */
                                                    /* pushed, the pin connected to the key's column    */
                                                    /* will be 0.                                       */  

    P0 = numberList[numbersToDisplay[scanNumber/4]-'0'];
}


/* -----------------
 * Function: scanner
 * -----------------
 *
 * scans keypad for key pushes and releases
 *
 */

void scanner(void)
{
    inputColumn[0] = krl1;
    inputColumn[1] = krl2;
    inputColumn[2] = krl3;
    inputColumn[3] = krl4;      /* take input values of pins and store them in inputColumn. */
                                /* inputColumn will be 0 when keypad row is selected        */
                                /* through digitSelector and key on that row is pushed.     */
     
    
    x = inputColumn[scanNumber%4];      /* We are interested in the column of the key that was pushed. */ 
                                        /* Hence we take the modulus.                                  */

    if(keyPushConfirmed == 1)
    {
        if(x == 0)                      //key push still detected
        {
            pushReleasedCount = 32; 
        } else {                        //else key release was detected
            pushReleasedCount--;
            if(pushReleasedCount == 0)  /* If key release still detected after 32 iterations (2 rounds on keypad), */
                                        /* key release is confirmed.                                               */
            {
                keyReleaseConfirmed = 1;
                pushReleasedCount = 32; 
            }
        }
    } else {
        if(pushDetectedCount == 33)
        {
            if(x == 0)                          //x is 0 if key push is detected, 1 otherwise
            {
                pushDetectedCount--;
                keyCode = scanNumber;           /* save key value of detected push. If push is confirmed this  */
                                                /* value will be displayed.                                    */
            }
        } else {
            pushDetectedCount--;
            if(pushDetectedCount == 0)
            {
                if(x == 0)                      /* If key push still detected after 32 iterations (2 rounds on keypad), */
                                                /* key push is confirmed.                                               */
                {
                    keyPushConfirmed = 1;
                    pushDetectedCount = 33;
                } else {                                                  
                    pushDetectedCount = 33;     //Detected push was not confirmed. Forget value
                }
            }
        }
    }
}

/* -----------------
 * Function: buzzer
 * -----------------
 *
 * sounds buzzer when key is pushed
 *
 */

void buzzer(void)
{
    if(keyPushConfirmed == 1)
    {
        buzz = ~buzz;
    }
}


/* -----------------
 * Function: displayKeyValue
 * -----------------
 *
 * updates value to be displayed
 *
 */

void displayKeyValue(void)
{
    numbersToDisplay[0] = asciiTab[keyCode];
}


/* --------------------------------------
 * Interrupt Service Routine: Timer 0 Interrupt
 * --------------------------------------
 *
 * Runs each time timer 0 interrupt is generated 
 *
 * resets Timer 0's count register to start value, displays keypad value, scans keypad, 
 *    and sounds buzzer when key is pushed
 * 
 *
 */

void isr_t0() interrupt 1
{
    resetTimer0();
    display();
    scanner();
    buzzer();

    scanNumber++;
    if(scanNumber == 16)
    {
        scanNumber = 0;
    }
}
      
