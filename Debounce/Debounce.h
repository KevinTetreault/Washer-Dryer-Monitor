// Kevin Tetreault 2020-07-03 
// Version of debounce which can support debouncing bool variables (non digital inputs)
// Based on code from Bounce2

#ifndef Debounce_h
#define Debounce_h

#include <inttypes.h>

class Debounce
{

public:
   // Debounce constructor for non digital inputs
  Debounce();
   // Debounce constructor for an assigned digital inputs
  Debounce(uint8_t pin);
	 // Sets the debounce interval
  void interval(unsigned long interval_millis); 
   // update for an assigned digital input
	 // Returns true if the state changed
	 // Returns false if the state did not change
  bool update();
   // update non digital inputs, pass in variable to update
  bool update(bool var);
	// Returns the debounced pin state
  bool read();
  
private:
  bool _update(bool var);
  unsigned long  previous_millis, interval_millis;
  bool debouncedState;
  bool unstableState;
  uint8_t pin;
  bool stateChanged;
  const uint8_t NO_PIN;
};

#endif


