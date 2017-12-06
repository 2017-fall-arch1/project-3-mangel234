 
 
/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
//
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6

int playerOne_Score = 0;
int playerTwo_Score = 0;
char scoreReferee[3];


AbRectOutline paddle = {abRectOutlineGetBounds, abRectOutlineCheck, {15, 2}};



AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 12, screenHeight/2 - 12}
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_STEEL_BLUE,
  0
};


Layer layer2 = {		/**< Layer with user paddle botttom*/
  (AbShape *)&paddle,
  
  {screenWidth/2 , screenHeight/2 + 63}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_STEEL_BLUE,
  &fieldLayer,
};

Layer layer1 = {		/**< Layer with paddle top paddle*/
  (AbShape *)&paddle,
  {screenWidth/2, screenHeight/2 - 63}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PINK,
  &layer2,
};

Layer layer0 = {		/**< Layer with the ball */
  (AbShape *)&circle4,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_STEEL_BLUE,
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
//MovLayer ml3 = { &layer3, {1,1}, 0 }; /**< not all layers move */
MovLayer ml2 = { &layer2, {2,0}, 0 }; //bottom paddle
MovLayer ml1 = { &layer1, {2,0}, &ml2}; //top paddle
MovLayer ml0 = { &layer0, {2,1}, &ml1}; //ball


void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;
  /**
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
    
    drawString5x7(120,70, "T", COLOR_YELLOW, COLOR_BLACK); */
    //drawString5x7(115,80, "<3", COLOR_PURPLE, COLOR_BLACK);
    
  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
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



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, MovLayer *p1, MovLayer *p2, Region *fence)
{
    scoreReferee[1] = '-';
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  //just added
  Region paddle;
    Vec2 newPosP1;
	u_char axisP1;
	Region p1Boundary;
    
    Vec2 newPosP2;
	u_char axisP2;
	Region p2Boundary;
    
    paddle.topLeft.axes[0] = fence->topLeft.axes[0] + 7;
    paddle.topLeft.axes[1] = fence->topLeft.axes[1];
    paddle.botRight.axes[0] = fence->botRight.axes[0] - 7;
    paddle.botRight.axes[1] = fence->botRight.axes[1];
  
  for (; ml; ml = ml->next) {
     // buzzer_set_period(0);
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
        vec2Add(&newPosP1, &p1->layer->posNext, &p1->velocity);
		abShapeGetBounds(p1->layer->abShape, &newPosP1, &p1Boundary);
        
        vec2Add(&newPosP2, &p2->layer->posNext, &p2->velocity);
		abShapeGetBounds(p2->layer->abShape, &newPosP2, &p2Boundary);
        
    for (axis = 0; axis < 2; axis ++) {
        
        if((shapeBoundary.topLeft.axes[axis] < paddle.topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > paddle.botRight.axes[axis])){
          //  drawString5x7(60,60, "first if", COLOR_STEEL_BLUE, COLOR_BLACK);
                if(shapeBoundary.topLeft.axes[0] > p2Boundary.topLeft.axes[0] && shapeBoundary.botRight.axes[0] < p2Boundary.botRight.axes[0] && shapeBoundary.topLeft.axes[1] > (screenWidth/2)){
                 //    drawString5x7(50,50, "second if", COLOR_STEEL_BLUE, COLOR_BLACK);
                     int velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
                   // buzzer_set_period(5000);
                    newPos.axes[0] += (2*velocity);
                    break;
                }
                if(shapeBoundary.topLeft.axes[0] > p1Boundary.topLeft.axes[0] && shapeBoundary.botRight.axes[0] < p1Boundary.botRight.axes[0] && shapeBoundary.topLeft.axes[1] < (screenWidth/2)){
                  //   drawString5x7(70,70, "third if", COLOR_STEEL_BLUE, COLOR_BLACK);
                     int velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
                   // buzzer_set_period(4000);
                    newPos.axes[0] += (2*velocity);
                    break;
                }
            }
        
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
     //buzzer_set_period(500);
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    
    if(shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]){
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] =screenHeight/2;
            ml->velocity.axes[0] = 2;
            ml->layer->posNext = newPos;
            playerOne_Score++;
            int redrawScreen =1;
            break;
            
     }
     
     else if(shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]){
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            ml->velocity.axes[0] = -2;
            ml->layer->posNext = newPos;
            playerTwo_Score++;
            int redrawScreen =1;
            break;
            
     }
     
    } /**< for axis */

     int redrawScreen =1;
     ml->layer->posNext =newPos;
  
  if(playerOne_Score > 9 || playerTwo_Score > 9){
    playerOne_Score = 0;
    playerTwo_Score = 0;

  }
    scoreReferee[0] = '0' + playerOne_Score;
    scoreReferee[2] = '0' + playerTwo_Score;
  }
  int redrawScreen =1;
  drawString5x7(37,150, scoreReferee, COLOR_PURPLE, COLOR_BLACK);
 
}


void p1_UP_DOWN(u_int sw) {
  if(!(sw & (1<<0))) {
    ml2.velocity.axes[0] = -5;
  }
  else if(!(sw & (1<<1))) {
    ml2.velocity.axes[0] = 5;
  }
  else {
    ml2.velocity.axes[0] = 0;
  }
}

void p2_UP_DOWN(u_int sw) {
  if(!(sw & (1<<2))) {
    ml1.velocity.axes[0] = -5;
  }
  else if(!(sw & (1<<3))) {
    ml1.velocity.axes[0] = 5;
  }
  else {
    ml1.velocity.axes[0] = 0;
  }
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region fence;


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  //buzzer_init();
  lcd_init();
  shapeInit();
    p2sw_init(15);


  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

 
  layerGetBounds(&fieldLayer, &fieldFence);
  drawString5x7(40,150, scoreReferee, COLOR_PURPLE, COLOR_BLACK);
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
   
  enableWDTInterrupts();      /**< enable periodic interrupt */
  // drawString5x7(20,50, "COLLISIONLESS ", COLOR_YELLOW, COLOR_BLACK);
   //drawString5x7(50,60, "PONG", COLOR_YELLOW, COLOR_BLACK);
  or_sr(0x8);	              /**< GIE (enable interrupts) */

u_int sw;

  for(;;) { 
      sw = p2sw_read(); //added----------
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    p1_UP_DOWN(sw);
    p2_UP_DOWN(sw);
    
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
     movLayerDraw(&ml1, &layer0);
    movLayerDraw(&ml2, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  
  if (count == 15) {
    mlAdvance(&ml0, &ml1, &ml2, &fieldFence);
    
    
    if (p2sw_read())
      redrawScreen = 1;
    
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
  //drawString5x7(50 ,50, "PAUSE", COLOR_YELLOW, COLOR_BLACK);
}
