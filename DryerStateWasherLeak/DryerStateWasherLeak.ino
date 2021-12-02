/**
 *  MySensors node - Dryer Vibration & Washer Leak
 *  Measures dryer state (on/off) using an accelerometer and leak using a transistor, probes, and analog channel
 *  Author: Kevin Tetreault
 *  Rev  Date       Description
 *  1.0  Nov 05/21  Initial release
 */
 
// Enable and select radio type attached
//#define MY_RADIO_RF24
#define MY_RADIO_RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY  RFM69_915MHZ

#include <MySensors.h>
#include <I2Cdev.h>
#include <Wire.h>
#include <MPU6050.h>
#include <SoftwareSerial.h>
#include <Debounce.h>
#include <MeanFilterLib.h>
#include <StatsRun.h>

#define LEAK_SENSOR   A0          // Leak sensor generates an analog voltage (low v-> ok, high v-> leak)
const int16_t leakThreshold = 650;    // ADC counts, reads 800-850 when immersing sensor

MPU6050 imu;  //use default address 0x68
int16_t aZero;
int16_t vibeThreshold = 500;   // default threshold, > indicates dryer is running
MeanFilter<int32_t> avgFilter(100); // 100 point moving avg filter
Debounce dbncLeak;            // we will pass leak variable in update()
Debounce dbncDryer;

// serial port for Bluetooth (JDY-31)
SoftwareSerial swSerial(3, 4); // RX (interrupt pin), TX

//MySensor variables
#define CHILD_ID_DRYER  0
#define CHILD_ID_LEAK   1
#define CHILD_ID_THRSHLD  2

// Initialize message objects
MyMessage msgDryer(CHILD_ID_DRYER, V_STATUS);
MyMessage msgVibeMin(CHILD_ID_DRYER, V_VAR1);
MyMessage msgVibeMax(CHILD_ID_DRYER, V_VAR2);
MyMessage msgVibeAvg(CHILD_ID_DRYER, V_VAR3);
MyMessage msgThreshold(CHILD_ID_THRSHLD, V_TEXT);
MyMessage msgLeak(CHILD_ID_LEAK, V_TRIPPED);

#define RUN_PER 30000    //running period for stats (#points)
StatsRun sRun(RUN_PER);

bool iniValDryerRecvd = false;
bool iniValLeakRecvd = false;
bool iniValVibeMinRecvd = false;
bool iniValVibeMaxRecvd = false;
bool iniValVibeAvgRecvd = false;
bool iniValVibeThrshRecvd = false;

void setup() 
{
  Wire.begin();
  // set the data rate for the SoftwareSerial port (JDY-31)
  swSerial.begin(38400);
  // initialize device
  bool initDone = false;
  wait(10);
  do {
    swSerial.println(F("Initializing I2C devices..."));
    imu.initialize();
    wait(10);
    // verify connection
    swSerial.println(F("Testing MPU6050 connection...")); 
    if (imu.testConnection()) {
      swSerial.println(F("MPU6050 connection successful"));
      initDone = true;
    }
    else {
      swSerial.println(F("MPU6050 connection failed"));
      wait(10);
    }
  } while (!initDone);
  
  swSerial.println(F("Tare Z axis"));
  tare(&aZero);
  swSerial.print(F("Zero: ")); swSerial.println(aZero);

  // Set debounce intervals
  dbncLeak.interval(1000);  //ms, longer time also ignores plug/unplug of probe
  dbncDryer.interval(2000); //ms, ignore transients (door closure)
  
  // configure Arduino LED pin for output
  pinMode(LED_BUILTIN, OUTPUT);

  // get vibe threshold from controller (HA)
  bool stsOk = false;
  int16_t retry = 0;
  // try up to 4 times
  do {
    request(CHILD_ID_THRSHLD, V_TEXT);
    stsOk = wait(2000, C_SET, V_TEXT);
    ++retry;
  } while (!stsOk && retry < 4);
  
  if (!iniValVibeThrshRecvd) {
    // Home Assistant won't send a value if the variable doesn't exist
    // so we send it this one time.
    char tmpStr[16];
    itoa(vibeThreshold, tmpStr, 10);
    send(msgThreshold.set(tmpStr));
    swSerial.println(F("Sent default threshold val to ctlr"));
  }
}

void presentation()
{
  swSerial.println(F("Sending presentation"));
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Dryer-Washer", "1.0");
  // Present all sensors to controller
  present(CHILD_ID_DRYER, S_BINARY, "Dryer On/Off");
  present(CHILD_ID_LEAK, S_WATER_LEAK, "Washer leak");
  present(CHILD_ID_THRSHLD, S_INFO, "Dryer Threshold");
}

void loop()
{
  static int16_t a, vibAbs, vibAvg, leak, cnt = 0;
  static bool dryerState, lastDryerState = true;
  static bool bLeak, bLastLeak = true;
  static unsigned long now, lastTime = millis(), hbStartTime = millis();
  static unsigned long vibStartTime = millis(), leakStartTime = millis();

  // Dryer vibration occurs mainly in Z axis of accelerometer
  a = imu.getAccelerationZ();
  vibAbs = abs(a - aZero);
  vibAvg = avgFilter.AddValue(vibAbs);
  // display on Bluetooth, average vibration, every 5s
  if ( millis() - vibStartTime >= 10000) {
     swSerial.print(F("Avg vib: ")); swSerial.println(vibAvg);
     vibStartTime = millis(); 
  }
  // calc stats - min, max, avg over 30s running period
  sRun.update(vibAvg);  
  dryerState = vibAvg > vibeThreshold ? true : false;
  dbncDryer.update(dryerState);

  if ( dbncDryer.read() != lastDryerState ) {
    send(msgDryer.set(dbncDryer.read()));
    swSerial.print(F("Sent Dryer state message dbncDryer: ")); swSerial.println(dbncDryer.read());
    lastDryerState = dbncDryer.read();
    
    // home assistant requires this initial request
    if (!iniValDryerRecvd) {
      swSerial.println(F("Requesting initial dryer value from ctlr"));
      request(CHILD_ID_DRYER, V_STATUS);
      wait(2000, C_SET, V_STATUS);      
    }
  }
  if (sRun.resultsChanged()) {
    int16_t vibeMin, vibeMax, vibeAvg;
    sRun.getResults(&vibeMin, &vibeMax, &vibeAvg);
    send(msgVibeMin.set(vibeMin));
    send(msgVibeMax.set(vibeMax));
    send(msgVibeAvg.set(vibeAvg));
    swSerial.println(F("Sent vibe min, max, avg"));

    if (!iniValVibeMinRecvd) {
      request(CHILD_ID_DRYER, V_VAR1);
      wait(2000, C_SET, V_VAR1);            
    }
    if (!iniValVibeMaxRecvd) {
      request(CHILD_ID_DRYER, V_VAR2);
      wait(2000, C_SET, V_VAR2);            
    }
    if (!iniValVibeAvgRecvd) {
      request(CHILD_ID_DRYER, V_VAR3);
      wait(2000, C_SET, V_VAR3);            
    }
  }

  // Get leak sensor value ////////////////////////////////////
  leak = analogRead(LEAK_SENSOR);
  bLeak = leak > leakThreshold ? true : false;
  dbncLeak.update(bLeak);

  if (millis() - leakStartTime >= 10000) {
     swSerial.print(F("Leak binary: ")); swSerial.println(bLeak);
     swSerial.print(F("Leak analog: ")); swSerial.println(leak);
     leakStartTime = millis();     
  }
  if (dbncLeak.read() != bLastLeak) {
    send(msgLeak.set(dbncLeak.read()));
    swSerial.print(F("Sent leak message, dbncLeak: ")); swSerial.println(dbncLeak.read());
    bLastLeak = dbncLeak.read();
    swSerial.print(F("Leak sensor reading: ")); swSerial.println(leak);
    
    // home assistant requires this initial request
    if (!iniValLeakRecvd) {
      swSerial.println(F("Requesting initial Leak value from ctlr"));
      request(CHILD_ID_LEAK, V_TRIPPED);
      wait(2000, C_SET, V_TRIPPED);
    }
  }
  // loop time is roughly 2ms based on testing
  wait(1);  // ensure at least 1ms between readings
  now = millis();
  if (cnt++ %30000 == 0) {
    swSerial.print(F("Loop time: ")); 
    swSerial.print(now - lastTime);
    swSerial.println(" ms");
  }
  lastTime = now;

  // send heart beat every 2 minutes
  if (now - hbStartTime > 120000) {
    sendHeartbeat();
    hbStartTime = millis();
  }
}

void receive(const MyMessage &message)
{
  const char *recvStr PROGMEM = "Recvd initial value from ctlr, ";
  
  if (message.isAck()) {
    swSerial.println("This is an ack from gateway");
  }
  if (message.type == V_STATUS && message.sensor == CHILD_ID_DRYER) {
    if (!iniValDryerRecvd) {
      swSerial.print(recvStr); swSerial.println("dryer state");
      iniValDryerRecvd = true;
    }
  }
  else if (message.type == V_TRIPPED && message.sensor == CHILD_ID_LEAK) {
    if (!iniValLeakRecvd) {
      swSerial.print(recvStr); swSerial.println("leak");
      iniValLeakRecvd = true;
    }
  }
  else if (message.type == V_VAR1 && message.sensor == CHILD_ID_DRYER) {
    if (!iniValVibeMinRecvd) {
      swSerial.print(recvStr); swSerial.println("vibeMin");
      iniValVibeMinRecvd = true;
    }
  }
  else if (message.type == V_VAR2 && message.sensor == CHILD_ID_DRYER) {
    if (!iniValVibeMaxRecvd) {
      swSerial.print(recvStr); swSerial.println("vibeMax");
      iniValVibeMaxRecvd = true;
    }
  }
  else if (message.type == V_VAR3 && message.sensor == CHILD_ID_DRYER) {
    if (!iniValVibeAvgRecvd) {
      swSerial.print(recvStr); swSerial.println("vibeAvg");
      iniValVibeAvgRecvd = true;
    }
  }
  else if (message.type == V_TEXT && message.sensor == CHILD_ID_THRSHLD) {
    if (!iniValVibeThrshRecvd) {
      swSerial.print(recvStr); swSerial.println("vibeThrsh");
      iniValVibeThrshRecvd = true;
    }
 
    // update our local vibe threshold variable
    char tmpStr[16];
    message.getString(tmpStr);
    vibeThreshold = atoi(tmpStr);
    swSerial.print(F("Received from ctlr, vibeThreshold: "));
    swSerial.println(vibeThreshold);

    // Send message back to controller for display on UI
    send(msgThreshold.set(message.getString()));
  }
}

// Zero out accelerometer 
void tare(int16_t *zero)
{
  int32_t sum = 0;
  for (int i=0; i < 1000; ++i) {
    sum += imu.getAccelerationZ();
    delay(1);
  }
  *zero = sum/1000;
}
