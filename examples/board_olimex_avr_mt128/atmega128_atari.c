/**
 * Atari breakout game
 * by Robert Fancsik
 */

#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

#define	__AVR_ATmega128__	1
#include <avr/io.h>
#include <math.h>


// GENERAL INIT - USED BY ALMOST EVERYTHING ----------------------------------

static void port_init() {
	PORTA = 0b00011111;	DDRA = 0b01000000; // buttons & led
	PORTB = 0b00000000;	DDRB = 0b00000000;
	PORTC = 0b00000000;	DDRC = 0b11110111; // lcd
	PORTD = 0b11000000;	DDRD = 0b00001000;
	PORTE = 0b00100000;	DDRE = 0b00110000; // buzzer
	PORTF = 0b00000000;	DDRF = 0b00000000;
	PORTG = 0b00000000;	DDRG = 0b00000000;
}

// TIMER-BASED RANDOM NUMBER GENERATOR ---------------------------------------

static void rnd_init() {
	TCCR0 |= (1  << CS00);	// Timer 0 no prescaling (@FCPU)
	TCNT0 = 0; 				// init counter
}

// generate a value between 0 and max
static int rnd_gen(int max) {
	return TCNT0 % max;
}

// BUTTON HANDLING -----------------------------------------------------------

#define BUTTON_NONE		0
#define BUTTON_UP		  1
#define BUTTON_LEFT		2
#define BUTTON_CENTER	3
#define BUTTON_RIGHT	4
#define BUTTON_DOWN		5
static int button_accept = 1;

static int button_pressed() {
	// up
	if ((!(PINA & 0b00000001)) & button_accept)
	{					   // check state of button 1 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_UP;
	}

	// left
	if ((!(PINA & 0b00000010)) & button_accept)
	{					   // check state of button 2 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_LEFT;
	}

	// center
	if ((!(PINA & 0b00000100)) & button_accept)
	{					   // check state of button 3 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_CENTER;
	}

	// right
	if ((!(PINA & 0b00001000)) & button_accept)
	{					   // check state of button 5 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_RIGHT;
	}

	// down
	if ((!(PINA & 0b00010000)) & button_accept)
	{					   // check state of button 5 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_DOWN;
	}

	return BUTTON_NONE;
}

static void button_unlock() {
	//check state of all buttons
	if (
		((PINA & 0b00000001)
		|(PINA & 0b00000010)
		|(PINA & 0b00000100)
		|(PINA & 0b00001000)
		|(PINA & 0b00010000)) == 31)
	button_accept = 1; //if all buttons are released button_accept gets value 1
}

// LCD HELPERS ---------------------------------------------------------------

#define		CLR_DISP	    0x00000001
#define		DISP_ON		    0x0000000C
#define		DISP_OFF	    0x00000008
#define		CUR_HOME      0x00000002
#define		CUR_OFF 	    0x0000000C
#define   CUR_ON_UNDER  0x0000000E
#define   CUR_ON_BLINK  0x0000000F
#define   CUR_LEFT      0x00000010
#define   CUR_RIGHT     0x00000014
#define   CG_RAM_ADDR		0x00000040
#define		DD_RAM_ADDR	  0x00000080
#define		DD_RAM_ADDR2	0x000000C0

//#define		ENTRY_INC	    0x00000007	//LCD increment
//#define		ENTRY_DEC	    0x00000005	//LCD decrement
//#define		SH_LCD_LEFT	  0x00000010	//LCD shift left
//#define		SH_LCD_RIGHT	0x00000014	//LCD shift right
//#define		MV_LCD_LEFT	  0x00000018	//LCD move left
//#define		MV_LCD_RIGHT	0x0000001C	//LCD move right

static void lcd_delay(unsigned int b) {
	volatile unsigned int a = b;
	while (a)
		a--;
}

static void lcd_pulse() {
	PORTC = PORTC | 0b00000100;	//set E to high
	lcd_delay(1400); 			//delay ~110ms
	PORTC = PORTC & 0b11111011;	//set E to low
}

static void lcd_send(int command, unsigned char a) {
	unsigned char data;

	data = 0b00001111 | a;					//get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	if (command)
		PORTC = PORTC & 0b11111110;			//set RS port to 0 -> display set to command mode
	else
		PORTC = PORTC | 0b00000001;			//set RS port to 1 -> display set to data mode
	lcd_pulse();							//pulse to set D4-D7 bits

	data = a<<4;							//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//set D4-D7
	if (command)
		PORTC = PORTC & 0b11111110;			//set RS port to 0 -> display set to command mode
	else
		PORTC = PORTC | 0b00000001;			//set RS port to 1 -> display set to data mode
	lcd_pulse();							//pulse to set d4-d7 bits
}

static void lcd_send_command(unsigned char a) {
	lcd_send(1, a);
}

static void lcd_send_data(unsigned char a) {
	lcd_send(0, a);
}

static void lcd_init() {
	//LCD initialization
	//step by step (from Gosho) - from DATASHEET

	PORTC = PORTC & 0b11111110;

	lcd_delay(10000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00100000;				//set D4 to 0, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)

	lcd_send_command(0x28); // function set: 4 bits interface, 2 display lines, 5x8 font
	lcd_send_command(DISP_OFF); // display off, cursor off, blinking off
	lcd_send_command(CLR_DISP); // clear display
	lcd_send_command(0x06); // entry mode set: cursor increments, display does not shift

	lcd_send_command(DISP_ON);		// Turn ON Display
	lcd_send_command(CLR_DISP);		// Clear Display
}

static void lcd_send_text(char *str) {
	while (*str)
		lcd_send_data(*str++);
}

static void lcd_send_line1(char *str) {
	lcd_send_command(DD_RAM_ADDR);
	lcd_send_text(str);
}

static void lcd_send_line2(char *str) {
	lcd_send_command(DD_RAM_ADDR2);
	lcd_send_text(str);
}

// GAME ----------------------------------------------------------------------
// BLOCK
typedef enum
{
  VOID,
  BAT,
  BLOCK,
  BALL,
  TRAP,
	__BLOCK_COUNT
} block_t;

// DEFINES
#define N 4  // MODIFIABLE
#define M 16 // MODIFIABLE
#define BALL_COUNT 2 // MODIFIABLE
#define LAST_COL (M - 1)
#define LAST_ROW (N - 1)
#define PI 3.141592653589793
#define RASTER_ROUND_LIMIT 0.5

#define ARRAY_SIZE(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define ADD_BALL(_x, _y)                  \
  {                                       \
    .pos = {.x = _y, .y = _x}, .angle = 0 \
  }

// GAME_STATUS
typedef enum
{
	GAME_STATUS_CONTINUE,
	GAME_STATUS_GAME_OVER,
	GAME_STATUS_WIN,
} game_status_t;

// MAP
typedef struct
{
  int x;
  int y;
} map_coordinate_t;

// BALL
typedef struct
{
  double x;
  double y;
} ball_pos_t;

typedef struct
{
  ball_pos_t pos;
  int angle;
} ball_t;

// MODIFIABLE
static const int default_block_count = 4 * 8;

static const block_t default_map[N][M] = {
    {TRAP, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {BAT,  VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {BAT,  VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {TRAP, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
};

static const ball_t default_ball_map[BALL_COUNT] = {
    ADD_BALL(1, 4),
    ADD_BALL(3, 5),
};

static int block_count;
static block_t map[N][M];
static ball_t ball_map[BALL_COUNT];

// DEGREE
static int
random_degree(void)
{
  return rnd_gen(360);
}

double deg_to_rad(int deg)
{
  return (double)(deg * (PI / 180));
}

// BLOCK
static map_coordinate_t
ball_pos_to_coordinate(ball_pos_t *ball_p)
{
  map_coordinate_t coord;

  coord.x = (int)ball_p->x;
  coord.y = (int)ball_p->y;

  if (ball_p->x - coord.x >= RASTER_ROUND_LIMIT)
  {
    coord.x++;
  }

  if (ball_p->y - coord.y >= RASTER_ROUND_LIMIT)
  {
    coord.y++;
  }

  return coord;
}

// BALL
static void
adjust_angle_corner(ball_t *ball_p)
{
  ball_p->angle = (ball_p->angle + 180) % 360;
}

static void
adjust_angle_y(ball_t *ball_p)
{
  ball_p->angle = 360 - ball_p->angle;
}

static void
adjust_angle_x(ball_t *ball_p)
{
  if (ball_p->angle > 180) // 180 - 360
  {
    ball_p->angle = 270 + (270 - ball_p->angle);
  }
  else
  {
    ball_p->angle = 180 - ball_p->angle;
  }
}

static game_status_t
update_map(ball_t *ball_p, ball_pos_t *original_ball_cord_p)
{
  map_coordinate_t original_ball_cord = ball_pos_to_coordinate(original_ball_cord_p);
  map_coordinate_t ball_cord = ball_pos_to_coordinate(&ball_p->pos);

  switch (map[ball_cord.y][ball_cord.x])
  {
    case BAT:
    {
      ball_p->pos.x++;
      ball_cord.x++;
      adjust_angle_x(ball_p);
      break;
    }
    case TRAP:
    {
      return GAME_STATUS_GAME_OVER;
    }
    case BLOCK:
    {
      if (fabs(original_ball_cord_p->x - ball_p->pos.x) < RASTER_ROUND_LIMIT)
      {
        if (fabs(original_ball_cord_p->y - ball_p->pos.y) < RASTER_ROUND_LIMIT)
        {
          adjust_angle_corner(ball_p);
        }
        else
        {
          adjust_angle_y(ball_p);
        }
      }
      else
      {
        if (fabs(original_ball_cord_p->y - ball_p->pos.y) < RASTER_ROUND_LIMIT)
        {
          adjust_angle_corner(ball_p);
        }
        else
        {
          adjust_angle_x(ball_p);
        }
      }

      if (--block_count == 0)
      {
        return GAME_STATUS_WIN;
      }
      break;
    }
    default:
    {
      break;
    }
  }

  map[original_ball_cord.y][original_ball_cord.x] = VOID;
  map[ball_cord.y][ball_cord.x] = BALL;
	return GAME_STATUS_CONTINUE;
}

static void
move_ball(ball_t *ball_p)
{
  ball_p->pos.x += cos(deg_to_rad(ball_p->angle));
  ball_p->pos.y += sin(deg_to_rad(ball_p->angle));

  if (ball_p->pos.x <= 0)
  {
    ball_p->pos.x = 0;
    adjust_angle_x(ball_p);
  }
  else if (ball_p->pos.x >= LAST_COL)
  {
    ball_p->pos.x = LAST_COL;
    adjust_angle_x(ball_p);
  }

  if (ball_p->pos.y <= 0)
  {
    ball_p->pos.y = 0;
    adjust_angle_y(ball_p);
  }
  else if (ball_p->pos.y >= LAST_ROW)
  {
    ball_p->pos.y = LAST_ROW;
    adjust_angle_y(ball_p);
  }
}

static game_status_t
move_balls()
{
  for (int i = 0; i < ARRAY_SIZE(ball_map); i++)
  {
    ball_t *ball_p = &ball_map[i];
    ball_pos_t original_ball_pos = ball_p->pos;
    move_ball(ball_p);
    game_status_t status = update_map(ball_p, &original_ball_pos);
		if (status != GAME_STATUS_CONTINUE)
		{
			return status;
		}
  }

	return GAME_STATUS_CONTINUE;
}

static void
start_balls(void)
{
  for (int i = 0; i < ARRAY_SIZE(ball_map); i++)
  {
    ball_map[i].angle = random_degree();
  }
}

static void
move_bat_up(void)
{
  if (map[0][0] == BAT)
  {
    return;
  }

  for (int i = 1; i < N; i++)
  {
    map[i - 1][0] = map[i][0];
  }

  map[LAST_ROW][0] = TRAP;
}

static void
move_bat_down(void)
{
  if (map[LAST_ROW][0] == BAT)
  {
    return;
  }

  for (int i = LAST_ROW; i > 0; i--)
  {
    map[i][0] = map[i - 1][0];
  }

  map[0][0] = TRAP;
}

// GRAPHICS ------------------------------------------------------------------

typedef enum {
	CHAR_MAP_SOLID_TOP,
	CHAR_MAP_SOLID_BOTTOM,

	CHAR_MAP_BALL_TOP,
	CHAR_MAP_BALL_BOTTOM,

	CHAR_MAP_SOLID_BALL,
	CHAR_MAP_BALL_SOLID,

	CHAR_MAP_SOLID_TRAP,
	CHAR_MAP_TRAP_SOLID,
	__CHAR_MAP_SIZE,

	CHAR_MAP_EMPTY = ' ',
	CHAR_MAP_SOLID_FULL = '#',
	CHAR_MAP_BALL_BALL = '8',
} char_mapping_t;


static unsigned char CHARMAP[__CHAR_MAP_SIZE][8] = {
	{ 0b11111, 0b11111, 0b11111, 0b11111, 0, 0, 0, 0 },	// CHAR_MAP_SOLID_TOP
	{ 0, 0, 0, 0, 0b11111, 0b11111, 0b11111, 0b11111 },	// CHAR_MAP_SOLID_BOTTOM

	{ 0b01110, 0b11111, 0b11111, 0b01110, 0, 0, 0, 0 },	// CHAR_MAP_BALL_TOP
	{ 0, 0, 0, 0, 0b01110, 0b11111, 0b11111, 0b01110 },	// CHAR_MAP_BALL_BOTTOM

	{ 0b11111, 0b11111, 0b11111, 0b11111, 0b01110, 0b11111, 0b11111, 0b01110 },	// CHAR_MAP_SOLID_BALL
	{ 0b01110, 0b11111, 0b11111, 0b01110, 0b11111, 0b11111, 0b11111, 0b11111 },	// CHAR_MAP_BALL_SOLID

	{ 0b11111, 0b11111, 0b11111, 0b11111, 0b10001, 0b01010, 0b01010, 0b10001 },	// CHAR_MAP_SOLID_TRAP
	{ 0b10001, 0b01010, 0b01010, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111 },	// CHAR_MAP_TRAP_SOLID
};

static void chars_init() {
	for (int c = 0; c < __CHAR_MAP_SIZE; ++c) {
		lcd_send_command(CG_RAM_ADDR + c*8);
		for (int r = 0; r < 8; ++r)
			lcd_send_data(CHARMAP[c][r]);
	}
}

static char char_mapping[__BLOCK_COUNT][__BLOCK_COUNT] =
{
	// VOID - VOID,  VOID - BAT,            VOID - BLOCK,          VOID - BALL,          VOID - TRAP
	{CHAR_MAP_EMPTY, CHAR_MAP_SOLID_BOTTOM, CHAR_MAP_SOLID_BOTTOM, CHAR_MAP_BALL_BOTTOM, CHAR_MAP_SOLID_BOTTOM},
	// BAT - VOID,       BAT - BAT,           BAT - BLOCK,                     BAT - BALL,                     BAT - TRAP
	{CHAR_MAP_SOLID_TOP, CHAR_MAP_SOLID_FULL, CHAR_MAP_EMPTY /* impossible */, CHAR_MAP_EMPTY /* game-over */, CHAR_MAP_SOLID_TOP },
	// BLOCK - VOID,     BLOCK - BAT,                     BLOCK - BLOCK,       BLOCK - BALL,        BLOCK - TRAP
	{CHAR_MAP_SOLID_TOP, CHAR_MAP_EMPTY /* impossible */, CHAR_MAP_SOLID_FULL, CHAR_MAP_SOLID_BALL, CHAR_MAP_EMPTY /* impossible */ },
	// BALL - VOID,     BALL - BAT,                          BALL - BLOCK,        BALL - BALL,        BALL - TRAP
	{CHAR_MAP_BALL_TOP, CHAR_MAP_SOLID_FULL /* game over */, CHAR_MAP_BALL_SOLID, CHAR_MAP_BALL_BALL, CHAR_MAP_BALL_SOLID /* game over */ },
	// TRAP - VOID,  TRAP - BAT,            TRAP - BLOCK,                    TRAP - BALL,                    TRAP - TRAP
	{CHAR_MAP_EMPTY, CHAR_MAP_SOLID_BOTTOM, CHAR_MAP_EMPTY /* impossible */, CHAR_MAP_EMPTY /* game over */, CHAR_MAP_EMPTY },
};

static void
write_char(int row_idx, int column_index, int write_cmd)
{
	block_t top_type = map[row_idx][column_index];
	block_t bottom_type = map[row_idx + 1][column_index];
	lcd_send_data(char_mapping[top_type][bottom_type]);
}

static void screen_update() {
	lcd_send_command(DD_RAM_ADDR);		//set to Line 1
	for (int j = 0; j < M; j++)
	{
		write_char(0, j, DD_RAM_ADDR);
	}

	lcd_send_command(DD_RAM_ADDR2);		//set to Line 1
	for (int j = 0; j < M; j++)
	{
		write_char(2, j, DD_RAM_ADDR2);
	}
}

static void reset_map()
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < M; j++)
		{
			map[i][j] = default_map[i][j];
		}
	}

	for (int i = 0; i < BALL_COUNT; i++)
	{
		ball_map[i] = default_ball_map[i];
	}

	block_count = default_block_count;
}

// THE GAME ==================================================================

int main() {
	port_init();
	lcd_init();
	chars_init();
	rnd_init();

	// "Splash screen"
	lcd_send_line1("    Atari");
	lcd_send_line2("  by rerobika  ");

	// loop of the whole program, always restarts game
	while (1) {

		while (button_pressed() != BUTTON_CENTER) // wait till start signal
			button_unlock(); // keep on clearing button_accept

		reset_map();
		game_status_t status;
		start_balls();

		// loop of the game
		while (1) {
			int button = button_pressed();

			if (button == BUTTON_UP)
			{
				move_bat_up();
			}
			else if (button == BUTTON_DOWN)
			{
				move_bat_down();
			}

			status = move_balls();
			if (status != GAME_STATUS_CONTINUE)
			{
				break;
			}
			// once all movements are done, update the screen
			screen_update();

			// try to unlock the buttons (technicality but must be done)
			button_unlock();
		} // end of game-loop

		if (status == GAME_STATUS_GAME_OVER)
		{
			lcd_send_line1("    GAME OVER   ");
		}
		else
		{
			lcd_send_line1("     YOU WIN    ");
		}
		lcd_send_line2("Click to restart");
	} // end of program-loop, we never quit
}
