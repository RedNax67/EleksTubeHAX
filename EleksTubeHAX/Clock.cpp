#include "Clock.h"
//#include "timezone_set.h"

String Date_str, Time_str, Time_format;
//const char* Timezone = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";       // NL


void Clock::begin(StoredConfig::Config::Clock *config_, const char* Timezone) {
  config = config_;

  if (config->is_valid != StoredConfig::valid) {
    // Config is invalid, probably a new device never had its config written.
    // Load some reasonable defaults.
    Serial.println("Loaded Clock config is invalid, using default.  This is normal on first boot.");
    setTwelveHour(true);
    setTimeZoneOffset(0);
    config->is_valid = StoredConfig::valid;
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  Time_format = "M"; // or StartTime("I"); for Imperial 12:00 PM format and Date format MM-DD-CCYY e.g. 12:30PM 31-Mar-2019

  //ntpTimeClient.begin();
  //ntpTimeClient.update();
  //Serial.println(ntpTimeClient.getFormattedTime());
  //setSyncProvider(&Clock::syncProvider);
}

void Clock::loop() {
  if (timeStatus() == timeNotSet) {
    time_valid = false;
  }
  else {
    time(&local_time);
    time_valid = true;
  }
}


String Clock::getLocalTime(String Format) {
  time_t now;
  time(&now);
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char hour_output[30], day_output[30];
  if (Format == "M") {
    strftime(day_output, 30, "%a  %d-%m-%y", localtime(&now)); // Formats date as: Sat 24-Jun-17
    strftime(hour_output, 30, "%H%M%S", localtime(&now));    // Formats time as: 14:05:49
  }
  else {
    strftime(day_output, 30, "%a  %m-%d-%y", localtime(&now)); // Formats date as: Sat Jun-24-17
    strftime(hour_output, 30, "%I%M%S", localtime(&now));          // Formats time as: 2:05:49pm
  }
  Date_str = day_output;
  Time_str = hour_output;
  return Time_str;
}



// Static methods used for sync provider to TimeLib library.
time_t Clock::syncProvider() {
  Serial.println("syncProvider()");
  time_t ntp_now, rtc_now;
  rtc_now = RTC.get();

  if (millis() - millis_last_ntp > refresh_ntp_every_ms || millis_last_ntp == 0) {
    // It's time to get a new NTP sync
    Serial.print("Getting NTP.");
    ntpTimeClient.forceUpdate();
    Serial.print(".");
    ntp_now = ntpTimeClient.getEpochTime();
    Serial.println(" Done.");

    // Sync the RTC to NTP if needed.
    if (ntp_now != rtc_now) {
      RTC.set(ntp_now);
      Serial.println("NTP, RTC, Diff: ");
      Serial.println(ntp_now);
      Serial.println(rtc_now);
      Serial.println(ntp_now-rtc_now);
    }
    millis_last_ntp = millis();
    return ntp_now;    
  }
  Serial.println("Using RTC time.");
  return rtc_now;
}

uint32_t Clock::millis_last_ntp = 0;
WiFiUDP Clock::ntpUDP;
NTPClient Clock::ntpTimeClient(ntpUDP);
