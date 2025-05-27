/*

*/

#include "HAResideo.h"
#include "Sensors.h"
#include "Timer.h"
#include <String.h>
#include <DatedVersion.h>
DATED_VERSION(0, 1)
#define DEVICE_NAME  "Resideo"
#define DEVICE_MODEL "Resideo Mod ESP32"
#define LED 8

////////////////////////////////////////////////////////////////////////////////////////////
//#define LOG_REMOTE
#define LOG_LEVEL 3
#include <Logging.h>

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
#define CONSTRUCT_P0(var)       var(#var, HABaseDeviceType::PrecisionP0)
#define CONSTRUCT_P1(var)       var(#var, HABaseDeviceType::PrecisionP1)

#define CONFIGURE_BASE(var, name, class, icon)  var.setName(name); var.setDeviceClass(class); var.setIcon("mdi:" icon)
#define CONFIGURE(var, name, class, icon, unit) CONFIGURE_BASE(var, name, class, icon); var.setUnitOfMeasurement(unit)

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
HAResideo::HAResideo()
: CONSTRUCT_P0(co2_level), CONSTRUCT_P1(humidity), CONSTRUCT_P1(resido_temp)
{
  CONFIGURE(resido_temp,"Temperature","temperature",    "thermometer",    "°C");
  CONFIGURE(co2_level,  "CO2",        "carbon_dioxide", "molecule-co2",   "ppm");
  CONFIGURE(humidity,   "Humidity",   "humidity",       "water-percent",  "%");
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAResideo::setup(const byte mac[6], HAMqtt *mqtt) 
{
  setUniqueId(mac, 6); //c:\Users\erikv\OneDrive\Archive\Erik\Hobby\Domotica\Resideo\LICENSE
  setManufacturer("Honeywell");
  setName(DEVICE_NAME);
  setSoftwareVersion(VERSION);
  setModel(DEVICE_MODEL);

  mqtt->addDeviceType(&humidity);  
  mqtt->addDeviceType(&resido_temp);  
  mqtt->addDeviceType(&co2_level);  

  CHT8305::setup();
  CM1106::setup();
  Timer::setup();

  pinMode(LED, OUTPUT);

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAResideo::loop()
{
  // If Timer has fired
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {

    digitalWrite(LED, !digitalRead(LED));

    INFO("T:%.1f   H:%.1f   C:%.1u\n", CHT8305::temperature(), CHT8305::humidity(), CM1106::ppm());

    resido_temp.setValue(CHT8305::temperature());
    humidity.setValue(CHT8305::humidity());
    co2_level.setValue(CM1106::ppm());
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

