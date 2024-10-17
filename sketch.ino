/*
 *  Дахен какое-то говно написала
 */
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Random16.h>


const byte MAX_UINT8 = 255;

const byte STICK_R_HORZ = A0;
const byte STICK_R_VERT = A1;
const byte STICK_L_HORZ = A2;
const byte STICK_L_VERT = A3;

const byte LCD_DC = 23;
const byte LCD_CS = 27;

const uint16_t WHITE   = ILI9341_WHITE; 
const uint16_t BLACK   = ILI9341_BLACK;
const uint16_t RED     = ILI9341_RED;
const uint16_t ORANGE  = ILI9341_ORANGE;
const uint16_t YELLOW  = ILI9341_YELLOW;
const uint16_t GREEN   = ILI9341_GREEN;
const uint16_t CYAN    = ILI9341_CYAN;
const uint16_t BLUE    = ILI9341_BLUE;
const uint16_t MAGENTA = ILI9341_MAGENTA;

const uint16_t BG_COLOR = BLACK;

const int16_t SCREEN_HEIGHT = 320;
const int16_t SCREEN_WIDTH  = 240;

const int16_t FIELD_HEIGHT = 300;
const int16_t FIELD_WIDTH  = 150;
const int16_t FIELD_Y      = (SCREEN_HEIGHT-FIELD_HEIGHT)/2;
const int16_t FIELD_X      = FIELD_Y;

const int16_t BLOCK_SIZE = FIELD_WIDTH/10;

const int16_t SCORE_X = FIELD_X*2 + FIELD_WIDTH;
const int16_t SCORE_Y = FIELD_Y;

const int16_t START_X = FIELD_X+(FIELD_WIDTH/2)-BLOCK_SIZE;
const int16_t START_Y = FIELD_Y;

enum {
  ROTATION_0,
  ROTATION_90,
  ROTATION_180,
  ROTATION_270
} Rotation;

enum { HARD, SOFT } DropType;
enum { RIGHT, LEFT } MoveDir;
enum { CW, CCW } RotateDir;
enum { ROTATE_180, SWAP } Rotate180Swap;

enum {
  I_PIECE, J_PIECE, L_PIECE, O_PIECE,
  S_PIECE, Z_PIECE, T_PIECE
} Pieces;


uint16_t g_score = 0,
         g_pieceX, g_piecePX = START_X, 
         g_pieceY, g_piecePY = START_Y, 
         g_pieceBC, g_pieceLC, g_pieceRC;
uint8_t  g_level = 0, g_holdPiece = 0, 
         g_piece, 
         g_pieceR, g_piecePR = 0;
bool     g_moved = false;

uint8_t g_grid[20][10];
uint8_t g_bag[7];
uint8_t g_bagIndex = MAX_UINT8;
Random16 g_rnd;

auto g_lcd = Adafruit_ILI9341(LCD_CS, LCD_DC);


void
drop(const int dropSpeed) 
{
}


void 
swap_rotate_180(const int swapRotate180) 
{
  uint8_t tmp;

  if (swapRotate180 == SWAP) {
    tmp = g_piece;
    g_piece = g_holdPiece;
    g_holdPiece = tmp;
    return;
  }

  g_piecePR = g_pieceR; 
  g_pieceR += 2;   
  g_moved = true;
}

void
move_piece(const int moveDir) 
{
  int16_t x = g_pieceX;

  if (moveDir == LEFT) {
    if (x == FIELD_X+g_pieceLC) return;
    x -= BLOCK_SIZE;
  } else {
    if (x == FIELD_X+FIELD_WIDTH-g_pieceRC) return;
    x += BLOCK_SIZE;
  }

  g_piecePX = g_pieceX;
  g_pieceX = x;
  g_moved = true;
}

void
rotate_piece(const int rotateDir) 
{
  g_piecePR = g_pieceR;
  g_pieceR += rotateDir == CW ? 1 : -1;
  g_moved = true;
}


void (* jumpTable[4])(const int) = { drop, move_piece, swap_rotate_180, rotate_piece };

inline void
handle_input(void)
{
  static uint8_t input = 0;
  
  if ( ! input) {
    input |= (((analogRead(STICK_R_HORZ)+1) >> 9) + 2);
    input |= (((analogRead(STICK_R_VERT)+1) >> 9) + 2) << 2;
    input |= (((analogRead(STICK_L_HORZ)+1) >> 9) + 2) << 4;
    input |= (((analogRead(STICK_L_VERT)+1) >> 9) + 2) << 6;
    
    input = ~input;

    if ( ! input) return;
  }

  for (int i = 3; input != 0; input >>= 2, --i)
    if (input & 0b01) jumpTable[i](!!(input & 0b10));
}


bool
move_current_piece_down(void)
{

  g_moved = true;
  return true;
}


void
draw_i(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t rotation) 
{
  int16_t width  = BLOCK_SIZE,
          height = BLOCK_SIZE,
          dx = 0, dy = 0;

  switch (rotation % 4) {
    case ROTATION_180: {
      dy = BLOCK_SIZE;
      g_pieceBC = BLOCK_SIZE;
    }
    case ROTATION_0: {
      g_pieceBC += BLOCK_SIZE;
      height <<= 2;
      break;
    }
    case ROTATION_270:
      dx -= BLOCK_SIZE;
    case ROTATION_90: {
      dx -= BLOCK_SIZE;
      dy = BLOCK_SIZE*2;
      g_pieceBC = BLOCK_SIZE*2;
      width <<= 2;
    }
  }

  g_lcd.fillRect(x+dx, y+dy, width, height, color);
}


void 
draw_j(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation)
{
  int16_t width = BLOCK_SIZE, 
          height = BLOCK_SIZE,
          dx_block = 0, 
          dy_block = 0, 
          dx_bar = 0, 
          dy_bar = BLOCK_SIZE;

  switch (rotation % 4) {
    case ROTATION_180: {
      dx_block = BLOCK_SIZE*2;
      dy_block = BLOCK_SIZE*2;
    }
    case ROTATION_0: {
      width += BLOCK_SIZE*2;
      break;
    }
    case ROTATION_270: {
      dy_block += BLOCK_SIZE*2;
      dx_block -= BLOCK_SIZE*2;
    }
    case ROTATION_90: {
      height += BLOCK_SIZE+BLOCK_SIZE;
      dx_bar = dy_bar;
      dy_bar = 0;
      dx_block += BLOCK_SIZE*2;
    }
  }

  g_lcd.fillRect(x+dx_block, y+dy_block, BLOCK_SIZE, BLOCK_SIZE, color);
  g_lcd.fillRect(x+dx_bar, y+dy_bar, width, height, color);
}


void 
draw_l(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation)
{
  int16_t width = BLOCK_SIZE, 
          height = BLOCK_SIZE,
          dx_block = BLOCK_SIZE*2, 
          dy_block = 0, 
          dx_bar = 0, 
          dy_bar = BLOCK_SIZE;

  switch (rotation % 4) {
    case ROTATION_270: {
      dx_block = 0;
      dy_block = BLOCK_SIZE*2;
    }
    case ROTATION_90: {
      width += BLOCK_SIZE*2;
      break;
    }
    case ROTATION_180: {
      dy_block += BLOCK_SIZE*2;
      dx_block += BLOCK_SIZE*2;
    }
    case ROTATION_0: {
      height += BLOCK_SIZE+BLOCK_SIZE;
      dx_bar = dy_bar;
      dy_bar = 0;
      dx_block -= BLOCK_SIZE*2;
    }
  }

  g_lcd.fillRect(x+dx_block, y+dy_block, BLOCK_SIZE, BLOCK_SIZE, color);
  g_lcd.fillRect(x+dx_bar, y+dy_bar, width, height, color);
}


void 
draw_o(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation) {
  g_lcd.fillRect(x, y, BLOCK_SIZE*2, BLOCK_SIZE*2, YELLOW);
}


void 
draw_s(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation)
{
  int16_t width  = BLOCK_SIZE,
          height = BLOCK_SIZE,
          dx_1 = BLOCK_SIZE, dy_1 = 0,
          dx_2 = 0, dy_2 = BLOCK_SIZE;

  switch (rotation % 4) {
    case ROTATION_180:
      dy_1 = BLOCK_SIZE;
    case ROTATION_0: {
      dy_2 = dy_1+BLOCK_SIZE;
      width += BLOCK_SIZE;
      break;
    }
    case ROTATION_270:
      dx_1 -= BLOCK_SIZE;
    case ROTATION_90: {
      dx_2 = dx_1+BLOCK_SIZE;
      height += BLOCK_SIZE;
    }
  }

  g_lcd.fillRect(x+dx_1, y+dy_1, width, height, color);
  g_lcd.fillRect(x+dx_2, y+dy_2, width, height, color);
}


void 
draw_z(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation)
{
  int16_t width  = BLOCK_SIZE,
          height = BLOCK_SIZE,
          dx_1 = 0, dy_1 = 0,
          dx_2 = BLOCK_SIZE, dy_2 = BLOCK_SIZE;

  switch (rotation % 4) {
    case ROTATION_180:
      dy_1 = BLOCK_SIZE;
    case ROTATION_0: {
      dy_2 = dy_1+BLOCK_SIZE;
      width += BLOCK_SIZE;
      break;
    }
    case ROTATION_270:
      dx_2 -= BLOCK_SIZE;
    case ROTATION_90: {
      dx_1 = dx_2+BLOCK_SIZE;
      height += BLOCK_SIZE;
    }
  }

  g_lcd.fillRect(x+dx_1, y+dy_1, width, height, color);
  g_lcd.fillRect(x+dx_2, y+dy_2, width, height, color);
}


void 
draw_t(const uint16_t x, 
       const uint16_t y, 
       const uint16_t color,
       const uint8_t  rotation)
{
  int16_t width = BLOCK_SIZE, 
          height = BLOCK_SIZE,
          dx_block = BLOCK_SIZE, 
          dy_block = 0, 
          dx_bar = 0, 
          dy_bar = BLOCK_SIZE;

  switch (rotation % 4) {
    case ROTATION_180:
      dy_block += BLOCK_SIZE*2;
    case ROTATION_0: {
      width += BLOCK_SIZE*2;
      break;
    }
    case ROTATION_270: {
      dx_block = -BLOCK_SIZE;
    }
    case ROTATION_90: {
      dx_block += BLOCK_SIZE;
      dy_block += BLOCK_SIZE;
      dy_bar = 0;
      dx_bar = BLOCK_SIZE;
      height += BLOCK_SIZE*2;
    }
  }  

  g_lcd.fillRect(x+dx_block, y+dy_block, BLOCK_SIZE, BLOCK_SIZE, color);
  g_lcd.fillRect(x+dx_bar, y+dy_bar, width, height, color);
}


void
get_next_piece(void)
{
  g_piece  = g_bag[++g_bagIndex];
  g_pieceX = START_X;
  g_pieceY = START_Y;
  g_pieceR = 0;
}


inline void
draw_ui(void)
{
  // Пишет очки
  g_lcd.setTextSize(1);
  g_lcd.setCursor(SCORE_X, SCORE_Y);
  g_lcd.print(g_score);
}


uint16_t colorTable[7] = { CYAN, BLUE, ORANGE, YELLOW, GREEN, RED, MAGENTA };

void (* draw_piece[7])(const uint16_t, const uint16_t, const uint16_t, const uint8_t) = {
  draw_i, draw_j, draw_l, draw_o,
  draw_s, draw_z, draw_t
};

inline void
draw_blocks(void)
{
  void (* drawp)(const uint16_t, const uint16_t, const uint16_t, const uint8_t);

  drawp = draw_piece[g_piece];

  if (g_moved) {
    drawp(g_piecePX, g_piecePY, BLACK, g_piecePR);
    g_moved = false;
  }
  drawp(g_pieceX, g_pieceY, colorTable[g_piece], g_pieceR);
}


inline void
update_lcd(void)
{
  draw_ui();
  draw_blocks();
}


inline void
process_game(void)
{
  uint8_t i;

  if (g_bagIndex >= 7) {
    for (i = 6; i >= 0; --i)
      g_bag[i] = g_rnd.get(7);
    g_bagIndex = MAX_UINT8;
  }

  if ( ! move_current_piece_down())
    get_next_piece();
}


void 
setup() 
{
  int8_t i;

  g_rnd.setSeed(analogRead(A15));

  for (i = 3; i >= 0; --i)
    pinMode(A0+i, INPUT);

  Serial.begin(115200);

  SPI.beginTransaction(
    SPISettings(14000000, MSBFIRST, SPI_MODE0)
  );

  for (i = 6; i >= 0; --i)
    g_bag[i] = g_rnd.get(7);
  get_next_piece();

  g_lcd.begin();
  g_lcd.drawRect(FIELD_X-2, FIELD_Y-2, FIELD_WIDTH+4, FIELD_HEIGHT+4, WHITE);
}


void 
loop() 
{
  handle_input();
  update_lcd();
  process_game();

  delay(100);
}