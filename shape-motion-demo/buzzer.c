#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"
#include "switches.h"

#define MIN_PERIOD 1000
#define MAX_PERIOD 4000

#define rest 0
static unsigned int period=1000;
static signed int rate =200;



void buzzer_init()
{
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

    //buzzer_set_period(0);	/* start buzzing!!! */
    //buzzer_advance_frequency();
}

void buzzer_advance_frequency() 
{
  period += rate;
  if ((rate > 0 && (period > MAX_PERIOD)) || 
      (rate < 0 && (period < MIN_PERIOD))) {
    rate = -rate;
    period += (rate << 1);
  }
  buzzer_set_period(period);
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}

void rest()
{
    beuzzer_set_period(0);
    
}

void bounce()
{
    for(int i=0;i<100;i++)
    {
        __delay_cycles(2000);
        buzzer_advance_frequency();
    }
    rest();
    
}
