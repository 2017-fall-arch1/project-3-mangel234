#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6

char  playerOne_Score = 0;
char playerTwo_Score = 0;
char scoreReferee[3];

AbRect rect10 = {abRectGetBounds, abRectCheck, {2,12}}; //Paddle rectangles definition!
AbRect line = {abRectGetBounds, abRectCheck, {1, 50}};//Paddle rectangles definition!

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 7, screenHeight/2 - 12}
};

// ball
Layer layer3 = {
  (AbShape *)&circle3,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &layer3
};

//Pink Paddle
Layer layer1 = {
  (AbShape *)&rect10,
  {screenWidth/2 - 55, screenHeight/2 -55}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PINK,
  &fieldLayer,
};

//green Paddle
Layer layer0 = {	
  (AbShape *)&rect10,
  {(screenWidth/2)+55, (screenHeight/2)+55}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &layer1,
};

/** Moving Layer
*  Linked list of layer references
*  Velocity represents one iteration of change (direction & magnitude)
*/
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {1,5}, 0 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {0,5}, &ml3 }; 
MovLayer ml0 = { &layer0, {0,5}, &ml1 }; 

//Just draws anything aorund a moving layer 
void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  
  
  and_sr(~8);			/**< disable interrupts (GIE off) */ //Turn off all interrupts 
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */ //Turn on all interupts


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds); 

    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
    bounds.botRight.axes[0], bounds.botRight.axes[1]);
  //For every pixel in the area..
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
  Vec2 pixelPos = {col, row};
  u_int color = bgColor;
  Layer *probeLayer; //We probe to get the colors..
  for (probeLayer = layers; probeLayer; 
      probeLayer = probeLayer->next) { /* probe all layers, in order */
    if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
      color = probeLayer->color;
      break; 
    } /* if probe check */
  } // for checking all layers at col, row
  lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  


//Region fence = {{10,10}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
*  
*  \param ml The moving shape to be advanced
*  \param fence The region which will serve as a boundary for ml
*/
void mlAdvance(MovLayer *whitePaddle, MovLayer *redPaddle, MovLayer *ml, Region *fence)
{
    
  scoreReferee[1] = '-';
  Vec2 newPos;
  Vec2 posBall; //not used
  Vec2 posRed; //notUsed
  Vec2 posWhite; //notused
  
  u_char axis;
  u_char axisRed; //not used
  u_char axisWhite; //not used
  
  Region shapeBoundary; //This is the fence
  Region ballBoundary;
  Region redBoundary;
  Region whiteBoundary;
  
  ballBoundary.topLeft.axes[0] = fence -> topLeft.axes[0] + 7;
  ballBoundary.topLeft.axes[1] = fence -> topLeft.axes[1];
  ballBoundary.botRight.axes[0] = fence -> botRight.axes[0] - 7;
  ballBoundary.botRight.axes[1] = fence -> botRight.axes[1];
  
  for (; ml; ml = ml->next) 
  {
      buzzer_set_period(0);
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity); 
    vec2Add(&posRed, &redPaddle->layer->posNext, &redPaddle->velocity); //not used
    vec2Add(&posWhite, &whitePaddle->layer->posNext, &whitePaddle->velocity); //not used
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    abShapeGetBounds(redPaddle->layer->abShape, &posRed, &redBoundary);//not used
    abShapeGetBounds(whitePaddle->layer->abShape, &posWhite, &whiteBoundary); //not used
    
        for (axis = 0; axis < 2; axis ++)
        {
          if((shapeBoundary.topLeft.axes[axis] < ballBoundary.topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > ballBoundary.botRight.axes[axis]))
          { 
            if( (shapeBoundary.topLeft.axes[1] > redBoundary.topLeft.axes[1]) && (shapeBoundary.botRight.axes[1] < redBoundary.botRight.axes[1]) && (shapeBoundary.topLeft.axes[0] > (screenWidth/2)))
            {
              int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
              buzzer_set_period(3000);
              newPos.axes[0] += (2*velocity);
             
              break;
            }
            if( (shapeBoundary.topLeft.axes[1] > whiteBoundary.topLeft.axes[1]) && (shapeBoundary.botRight.axes[1] < whiteBoundary.botRight.axes[1]) && (shapeBoundary.topLeft.axes[0] < (screenWidth/2)) )
            {
                int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
                buzzer_set_period(500);
                newPos.axes[0] += (2*velocity);
               
                break;
            }
          }
          
          if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])) {
          
              int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            buzzer_set_period(500);
            newPos.axes[axis] += (2*velocity);
          } /**< for axis */
         
         
         if(shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]){
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            ml->velocity.axes[0] = 2;
            ml->layer->posNext = newPos;
            playerOne_Score++;
            int redrawScreen = 1;
            break;
          }
          else if (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0]) {
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            ml->velocity.axes[0] = -2;
            ml->layer->posNext = newPos;
            playerTwo_Score++;
            int redrawScreen = 1;
            break;
          }
        
        }/**< for axis */
         int redrawScreen =1;
        ml->layer->posNext = newPos;
      
        if(playerOne_Score > 9 || playerTwo_Score > 9){
            playerOne_Score = 0;
            playerTwo_Score = 0;

            }
            scoreReferee[2] = '0' + playerOne_Score;
            scoreReferee[0] = '0' + playerTwo_Score;
  
  }/**< for ml */
 
    int redrawScreen = 1;
     drawString5x7(37,150, scoreReferee, COLOR_PINK, COLOR_BLACK);
   
 //drawString5x7(50,50,"damn", COLOR_PINK, COLOR_BLACK);
}

//Allows the paddles to move in the direction depending of buttton press
void p1(u_int button)
{
    if( !(button & (1 << 0)))
    {
      ml1.velocity.axes[1] = -5;
    }
    else if(!( button & (1<<1)))
    {
      ml1.velocity.axes[1] = 5; 
    }
    else
    {
      ml1.velocity.axes[1] = 0;
    }
}

void p2(u_int button)
{
    if( !(button & (1 << 2)))
    {
      ml0.velocity.axes[1] = -5;
    }
    else if(!( button & (1<<3)))
    {
      ml0.velocity.axes[1] = 5; 
    }
    else
    {
      ml0.velocity.axes[1] = 0;
    }
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region fieldPaddleRed;
Region fieldPaddleWhite;
Region fieldBall;


/** Initializes everything, enables interrupts and green LED, 
*  and handles the rendering for the screen
*/
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  buzzer_init();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);
  layerGetBounds(&layer1, &fieldPaddleRed);
  layerGetBounds(&layer0, &fieldPaddleWhite);
  layerGetBounds(&layer3, &fieldBall);
  //Display adds on game
    //drawString5x7(37,150, scoreReferee, COLOR_PURPLE, COLOR_BLACK);
    drawString5x7(1,0, "Sponsored by HINOJOSA", COLOR_YELLOW, COLOR_BLACK);
    drawString5x7(0,20, "G", COLOR_YELLOW, COLOR_BLACK);
    drawString5x7(0,30, "F", COLOR_PURPLE, COLOR_BLACK);
    drawString5x7(0,40, "U", COLOR_RED, COLOR_BLACK);
    drawString5x7(0,50, "E", COLOR_YELLOW, COLOR_BLACK);
    drawString5x7(0,60, "L", COLOR_STEEL_BLUE, COLOR_BLACK);
    drawString5x7(0,90, "D", COLOR_RED, COLOR_BLACK);
    drawString5x7(0,100, "O", COLOR_RED, COLOR_BLACK);
    drawString5x7(0,110, "C", COLOR_RED, COLOR_BLACK);
    drawString5x7(0,120, "!", COLOR_RED, COLOR_BLACK);
    drawString5x7(0,70, "..", COLOR_STEEL_BLUE, COLOR_BLACK);
    drawString5x7(0,150, "Score:", COLOR_PURPLE, COLOR_BLACK);
    drawString5x7(120,20, "R", COLOR_YELLOW, COLOR_BLACK);
    drawString5x7(120,30, "O", COLOR_PURPLE, COLOR_BLACK);
    drawString5x7(120,40, "B", COLOR_RED, COLOR_BLACK);
    drawString5x7(120,50, "E", COLOR_YELLOW, COLOR_BLACK);
    drawString5x7(120,60, "R", COLOR_STEEL_BLUE, COLOR_BLACK);
    
    drawString5x7(120,70, "T", COLOR_YELLOW, COLOR_BLACK); 
    drawString5x7(115,80, "<3", COLOR_PURPLE, COLOR_BLACK);
    //drawString5x7(20,50, "COLLISIONLESS ", COLOR_YELLOW, COLOR_BLACK);
   //drawString5x7(50,60, "PONG", COLOR_YELLOW, COLOR_BLACK);
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  u_int button;


  for(;;) { 
    button = p2sw_read();
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    
    p1(button);
    p2(button);
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
    mlAdvance(&ml1, &ml0, &ml0, &fieldFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
