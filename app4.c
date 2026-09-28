#include "sim.h"

#define X_SIZE (SIM_X_SIZE / 4)
#define Y_SIZE (SIM_Y_SIZE / 4)

#define MAX_VALUE 1024

void drawCell(int x, int y, int argb) {
  simPutPixel(x * 4 + 1, y * 4 + 1, argb);
  simPutPixel(x * 4 + 1, y * 4 + 2, argb);
  simPutPixel(x * 4 + 2, y * 4 + 1, argb);
  simPutPixel(x * 4 + 2, y * 4 + 2, argb);
  
}


int calcColor(int value) {
  int intensity = value < 0 ? -value : value;

  intensity *= 6;
  if (intensity > 255)
    intensity = 255;

  if (value > 0)
    return 0xFF000000 | (intensity << 16);   

  if (value < 0)
    return 0xFF000000 | intensity;  
  return 0xFF000000;
}

void drawField(int *field) {
  for (int y = 0; y < Y_SIZE; y++)
    for (int x = 0; x < X_SIZE; x++)
      drawCell(x, y, calcColor(field[X_SIZE * y + x]));
}

int getXLeft(int x) {
  if (x == 0)
    return X_SIZE - 1;
  return x - 1;
}

int getXRight(int x) {
  if (x == X_SIZE - 1)
    return 0;

  return x + 1;
}

int getYUp(int y) {
  if (y == 0)
    return Y_SIZE - 1;

  return y - 1;
}

int getYDown(int y) {

  if (y == Y_SIZE - 1)
    return 0;

  return y + 1;
}

int calcCell(int x, int y, int *current, int *previous) {

  int x_l = getXLeft(x);
  int x_r = getXRight(x);

  int y_u = getYUp(y);
  int y_d = getYDown(y);

  int neighbours =
      current[X_SIZE * y + x_l] +
      current[X_SIZE * y + x_r] +
      current[X_SIZE * y_u + x] +
      current[X_SIZE * y_d + x];

  int value = neighbours/2 - previous[X_SIZE * y + x];

  if (value > MAX_VALUE)
    value = MAX_VALUE;

  if (value < -MAX_VALUE)
    value = -MAX_VALUE;

  return value;
}

void calcField(int *current, int *previous, int *next) {

  for (int y = 0; y < Y_SIZE; y++)
    for (int x = 0; x < X_SIZE; x++)
      next[X_SIZE * y + x] =
          calcCell(x, y, current, previous);

}

void addWave(int *field, int xy) {

  int x = (xy >> 16) / 4;
  int y = (xy & 0xFFFF) / 4;

  int bias = 4;
  field[X_SIZE * y + x] += MAX_VALUE;

  if (x > 0)
    field[X_SIZE * y + x - 1] +=  MAX_VALUE -200;

  if (x < X_SIZE - 1)
    field[X_SIZE * y + x + 1] += MAX_VALUE -200;

  if (y > 0)
    field[X_SIZE * (y - 1) + x] += MAX_VALUE -200;

  if (y < Y_SIZE - 1)
    field[X_SIZE * (y + 1) + x] += MAX_VALUE -200;

}

void app(void) {

  int field1[Y_SIZE * X_SIZE] = {};
  int field2[Y_SIZE * X_SIZE] = {};
  int field3[Y_SIZE * X_SIZE] = {};

  int *previous = field1;
  int *current = field2;
  int *next = field3;

  while (1) {
    while (simHasClick())
    addWave(current, simGetClick());
    drawField(current);
    simFlush();
    calcField(current, previous, next);
    
    int *tmp = previous;

    previous = current;
    current = next;
    next = tmp;
  }

}