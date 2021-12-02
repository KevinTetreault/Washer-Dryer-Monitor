# Washer-Dryer-Monitor
Arduino sketch built on MySensors software library for home automation.

Sketch which monitors a washing machine for leaks and monitors a dryer for operation (on/off).
Leak sensing is performed via 2 parallel probes and a transistor based signal conditioner which generates an analog voltage proportional to the probe's conductivity. Conductivity is proportional to the length of probes bridged by water.
Dryer operation is sensed by measuring the vibration of the dryer when it is running using an accelerometer.

Also contains Debounce library for debouncing digital inputs or boolean variables. This is handy for debouncing on/off states of a signal versus a threshold.
Also contains StatsRun library which calculates minimum, maximum, and average running statistics over a given time interval.
