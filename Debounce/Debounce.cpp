#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Debounce.h"

// debounce constructor for non digital input
Debounce::Debounce() : NO_PIN(255)
{
	pin = NO_PIN;
	this->interval_millis = 10;
    debouncedState = unstableState = 0;
    previous_millis = millis();
	stateChanged = false;
}

// debounce constructor for an assigned digital input
Debounce::Debounce(uint8_t pin) : NO_PIN(255)
{
  this->interval_millis = 10;
  this->pin = pin;
  debouncedState = unstableState = digitalRead(pin);
  previous_millis = millis();
  stateChanged = false;
}

void Debounce::interval(unsigned long interval_millis)
{
  this->interval_millis = interval_millis; 
}

// update for an assigned digital input
bool Debounce::update()
{
	bool ret = false;

	if (pin != NO_PIN)
		ret = _update(digitalRead(pin));
	else
		ret = false;

	return ret;
}

// update for non digital inputs
bool Debounce::update(bool var)
{
	return _update(var);
}

// protected update function
bool Debounce::_update(bool var)
{
	bool currentState = var;
	stateChanged = false;

	// restart the timer if the switch changes state
	if ( currentState != unstableState ) {
		previous_millis = millis();
	}
	else if (millis() - previous_millis >= interval_millis) {
		// switch is stable
		// did switch change state?
		if ( currentState != debouncedState ) {
			debouncedState = currentState;
			stateChanged = true;				
		}
	}
	 
	unstableState = currentState;
	return stateChanged;
}

bool Debounce::read()
{
	return debouncedState;
}

