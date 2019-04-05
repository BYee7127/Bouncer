/**
 * This file draws a ball with given coordinates from the bouncer.cpp
 * file.
 *
 * Date:   2019.04.05
 * Author: Kade Challis, Beverly Yee
 **/

#include "ball.h"

ball::ball(int x, int y) {

  // ball constructor that takes in two values that
  // determine the position of the center of the ball to be drawn
  this->x = x;     // set the x-position
  this-> y = y;    // set the y-position
}

