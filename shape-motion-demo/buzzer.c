#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"

static signed int note = 0;
static unsigned int period = 1000;//starting note
static signed int rate = 200;	//incremement amount increase or decrease

#define MIN_PERIOD 1000
#define MAX_PERIOD 4000

#define e 1674
#define d 1879
#define c 2109
#define g 1408


void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

    //buzzer_set_period(1000);	/* start buzzing!!! */
}
/*
 * This method iterates through a switch
 * statement that increments by one and 
 * reached each case to play a song.
 * ("Mary had a little lamb").
 */
void buzzer_advance_frequency() 
{

   /* period += rate;
    if ((rate > 0 && (period > MAX_PERIOD)) || (rate < 0 && (period < MIN_PERIOD))) {
    rate = -rate;
    period += (rate << 1);
    }
    buzzer_set_period(period);
*/
}//end buzzer

void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 5;		/* one half cycle */
}



    
    
  

