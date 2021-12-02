// StatsRun - class which performs short run statistics on data points
// author: Kevin Tetreault
// version: 1.0
// date: Nov 5, 2021

#include <stdint.h>
#include "StatsRun.h"

StatsRun::StatsRun()
{
  init();
  length = 100;   // default to 100 points
}

StatsRun::StatsRun(int16_t length)
{
  init();
  this->length = length;  
}

void StatsRun::getResults( int16_t* min, int16_t* max, int16_t* avg)
{
  *min = rsltMin;
  *max = rsltMax;
  *avg = rsltAvg; 
}

void StatsRun::init()
{
  min = INT16_MAX;
  max = INT16_MIN; 
  avg = rsltMin = rsltMax = rsltAvg = cnt = 0;
  sum = 0;
  rsltsChanged = false;
}

void StatsRun::setLength(int16_t length)
{
  this->length = length;  
}

bool StatsRun::resultsChanged()
{
  return rsltsChanged;
}

void StatsRun::update(int16_t value)
{
  if (cnt >= length) {  // start a new running min, max, avg every new running period
    rsltMin = min;      // save the results for the last run period
    rsltMax = max;
    rsltAvg = avg;
    rsltsChanged = true;
    
    min = value;
    max = value;
    sum = 0;
    cnt = 0;
    avg = 0;
  }
  else {
    if (value < min)
      min = value;
    if (value > max)
      max = value;
      
    sum += value;
    ++cnt;
    avg = sum /cnt;
    rsltsChanged = false;    
  }
}

