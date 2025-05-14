/**
  @AUTHOR Pluimvee
 
  based on the work of
  https://github.com/rhmswink/Resideo-R200C2-ESPHome-Mod
  https://github.com/RobTillaart/CHT8305/tree/master
  
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf
 */
#include <Arduino.h>
#include "Timer.h"

hw_timer_t *My_timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;


////////////////////////////
//#define LOG_REMOTE
#define LOG_LEVEL 3
#include <Logging.h>

////////////////////////////
//// Interrupt handlers
/////////////////////////////

void ARDUINO_ISR_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter = isrCounter + 1;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool Timer::setup()
{
   // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

    // Set timer frequency to 1Mhz
  My_timer = timerBegin(1000000);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(My_timer, &onTimer);

    // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(My_timer, 1000000, true, 0);

  return true;
}