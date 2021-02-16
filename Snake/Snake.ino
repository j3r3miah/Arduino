#include <Wire.h>
#include <LiquidCrystal_I2C.h>

byte vertical[] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte horizontal[] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte topRight[] = {
  0b00000,
  0b00000,
  0b00000,
  0b11100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte topLeft[] = {
  0b00000,
  0b00000,
  0b00000,
  0b00111,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte bottomRight[] = {
  0b00100,
  0b00100,
  0b00100,
  0b11100,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte bottomLeft[] = {
  0b00100,
  0b00100,
  0b00100,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte heart[] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte alien[] = {
  0b11111,
  0b10101,
  0b11111,
  0b11111,
  0b01110,
  0b01010,
  0b11011,
  0b00000
};

byte board[20][4] = {0};

LiquidCrystal_I2C lcd(0x27, 20, 4);

#define HORIZONTAL 0
#define VERTICAL 1
#define TOP_RIGHT 2
#define TOP_LEFT 3
#define BOTTOM_RIGHT 4
#define BOTTOM_LEFT 5
#define HEART 6
#define ALIEN 7

#define CLEAR 32 // space char

class Point {
public:
  int8_t x;
  int8_t y;
  byte sprite;

  Point() : x(0), y(0), sprite(CLEAR) {}
  Point(int8_t x, int8_t y, byte sprite) : x(x), y(y), sprite(sprite) {}
  Point(const Point& p) : x(p.x), y(p.y), sprite(p.sprite) {}

  void place(byte board[20][4]) {
    board[x][y] = sprite;
  }
};

Point o(0, 0, HORIZONTAL);

Point ring[16];
uint8_t ring_cur = 0;
uint8_t ring_len = 6;
bool ring_full = false;

void ring_add(Point p) {
  ring[ring_cur++] = p;
  if (ring_cur >= ring_len) {
    ring_cur = 0;
    ring_full = true;
  }
}

Point ring_getLast() {
  // uint8_t last = ring_cur++;
  // if (last > sizeof(ring)) last = 0;
  return ring[ring_cur];
}

void setup()
{
  lcd.init();
  lcd.backlight();

  lcd.createChar(HORIZONTAL, horizontal);
  lcd.createChar(VERTICAL, vertical);
  lcd.createChar(TOP_RIGHT, topRight);
  lcd.createChar(TOP_LEFT, topLeft);
  lcd.createChar(BOTTOM_RIGHT, bottomRight);
  lcd.createChar(BOTTOM_LEFT, bottomLeft);
  lcd.createChar(HEART, heart);
  lcd.createChar(ALIEN, alien);

  resetBoard();
  o.place(board);
  paint();

  Serial.begin(9600);
}

void resetBoard() {
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 20; i++) {
      board[i][j] = CLEAR;
    }
  }
}

void paint() {
  // TODO make this faster by not writing anything that's unchanged
  for (int j = 0; j < 4; j++) {
    lcd.setCursor(0, j);
    for (int i = 0; i < 20; i++) {
      lcd.write(board[i][j]);
    }
  }
}

void loop()
{
  Point last(o);

  // clear sprite from last position
  // board[o.x][o.y] = CLEAR;

  // move to next position
  updatePosition();

  // choose sprite
  if (o.x == 0 && o.y == 0) o.sprite = TOP_LEFT;
  else if (o.x == 19 && o.y == 0) o.sprite = TOP_RIGHT;
  else if (o.x == 19 && o.y == 3) o.sprite = BOTTOM_RIGHT;
  else if (o.x == 0 && o.y == 3) o.sprite = BOTTOM_LEFT;
  else if (o.x == 0 || o.x == 19) o.sprite = VERTICAL;
  else o.sprite = HORIZONTAL;

  // add new sprite to new position
  o.place(board);

  // if ring is full, delete oldest sprite
  ring_add(last);
  if (ring_full) {
    Point del = ring_getLast();
    del.sprite = CLEAR;
    del.place(board);
  }

  paint();

  // TODO wow, writing to the LCD is slow enough to delay paints
  // delay(10);
}

int dir = 1;
void updatePosition() {
  o.x += dir;
  if (o.x > 19) {
    o.x = 19;
    o.y += dir;
    if (o.y > 3) {
      o.y = 3;
      dir = -1;
      o.x += dir;
    }
  }
  if (o.x < 0) {
    o.x = 0;
    o.y += dir;
    if (o.y < 0) {
      o.y = 0;
      dir = 1;
      o.x += dir;
    }
  }
}

void demo() {
  lcd.setCursor(8, 0);
  lcd.write(TOP_LEFT);
  lcd.write(HORIZONTAL);
  lcd.write(HORIZONTAL);
  lcd.write(TOP_RIGHT);

  lcd.setCursor(8, 1);
  lcd.write(VERTICAL);
  lcd.setCursor(11, 1);
  lcd.write(VERTICAL);

  lcd.setCursor(8, 2);
  lcd.write(BOTTOM_LEFT);
  lcd.write(HORIZONTAL);
  lcd.write(HORIZONTAL);
  lcd.write(BOTTOM_RIGHT);

  lcd.setCursor(3, 1);
  lcd.write(HEART);

  lcd.setCursor(16, 1);
  lcd.write(ALIEN);
}
