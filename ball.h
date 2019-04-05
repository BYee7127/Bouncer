/**
 * Header file for ball.cpp
 *
 * Date:   2019.04.05
 * Author: Kade Challis, Beverly Yee
 **/

// Prevent double inclusion
#ifndef BALL_H
#define BALL_H

#include <graphics.h>

class ball{
 private:
  int xPos, yPos;
 public:
  ball(int x, int y);   // constructor
};

#endif
