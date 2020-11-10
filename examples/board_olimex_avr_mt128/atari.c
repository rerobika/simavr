#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

// BUILD: gcc -o atari atari.c -lm

// BLOCK
typedef enum
{
  VOID,
  BAT,
  BLOCK,
  BALL,
  TRAP,
} block_t;

// DEFINES
#define N 4  // MODIFIABLE
#define M 16 // MODIFIABLE
#define LAST_COL (M - 1)
#define LAST_ROW (N - 1)
#define PI 3.141592653589793
#define RASTER_ROUND_LIMIT 0.5

#define ARRAY_SIZE(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define ADD_BALL(_x, _y)                  \
  {                                       \
    .pos = {.x = _y, .y = _x}, .angle = 0 \
  }
#define CLEAR_SCREEN()      \
  do                        \
  {                         \
    printf("\033[H\033[J"); \
  }                         \
  while (0)

// MAP
typedef struct
{
  int x;
  int y;
} map_coordinate_t;

// MODIFIABLE
static block_t map[N][M] = {
    {TRAP, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {BAT, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {TRAP, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
    {TRAP, VOID, VOID, VOID, VOID, VOID, VOID, VOID, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK, BLOCK},
};

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
static ball_t ball_map[] = {
    ADD_BALL(1, 4),
    ADD_BALL(3, 5),
};

// MODIFIABLE
static int block_count = 4 * 8;

// DEGREE
static int
random_degree(void)
{
  return rand() % 360;
}

double deg_to_rad(int deg)
{
  return (double)(deg * (PI / 180));
}

// BLOCK
static char
block_to_string(block_t type)
{
  switch (type)
  {
    case BAT:
      return '|';
    case BALL:
      return '*';
    case BLOCK:
      return 'O';
    case TRAP:
      return 'X';
    default:
      break;
  }
  return ' ';
}

static void
print_map(void)
{
  for (int i = 0; i < M + 2; i++)
  {
    printf("#");
  }

  printf("\n");

  for (int i = 0; i < N; i++)
  {
    printf("#");

    for (int j = 0; j < M; j++)
    {
      printf("%c", block_to_string(map[i][j]));
    }

    printf("#\n");
  }

  for (int i = 0; i < M + 2; i++)
  {
    printf("#");
  }

  printf("\n");
}

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

static void
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
      printf("Game over!\n");
      exit(0);
      break;
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
        printf("You win!\n");
        exit(0);
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
}

static void
move_ball(ball_t *ball_p)
{
  ball_pos_t original_ball_pos = ball_p->pos;

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

static void
move_balls()
{
  for (int i = 0; i < ARRAY_SIZE(ball_map); i++)
  {
    ball_t *ball_p = &ball_map[i];
    ball_pos_t original_ball_pos = ball_p->pos;
    move_ball(ball_p);
    update_map(ball_p, &original_ball_pos);
  }
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

int main(int argc,
         char *argv[])
{
  srand(time(0));
  start_balls();

  while (true)
  {
    move_balls();
    print_map();
    CLEAR_SCREEN();
    usleep(1000 * 250);
  }
}
