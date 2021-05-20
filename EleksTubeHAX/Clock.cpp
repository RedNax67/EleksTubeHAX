#include "Clock.h"
//#include "timezone_set.h"

String Date_str, Time_str, Time_format;
//const char* Timezone = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";       // NL

struct timeval prev_sync;
char hour_output[7], day_output[7];


void Clock::begin(StoredConfig::Config::Clock *config_, const char* Timezone) {
  config = config_;

  if (config->is_valid != StoredConfig::valid) {
    // Config is invalid, probably a new device never had its config written.
    // Load some reasonable defaults.
    Serial.println("Loaded Clock config is invalid, using default.  This is normal on first boot.");
    setTwelveHour(false);
    //setTimeZoneOffset(0);
    config->is_valid = StoredConfig::valid;
  }
  
  //configTzTime(Timezone, "pool.ntp.org", "time.nist.gov");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  tzset();
  Time_format = "M"; // or StartTime("I"); for Imperial 12:00 PM format and Date format MM-DD-CCYY e.g. 12:30PM 31-Mar-2019
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  //sntp_set_sync_interval(15000);

  prev_sync.tv_sec=0;
  

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

char* Clock::getLocalTime(String Format) {
  time_t now;
  time(&now);
  //See http://www.cplusplus.com/reference/ctime/strftime/
  if (Format == "M") {
    //strftime(day_output, 7, "%d%m%y", localtime(&now));  // Formats date as: 240617
    strftime(hour_output, 7, "%H%M%S",localtime(&now)); // Formats time as: 140549
  } else {
    strftime(day_output, 6, "%m%d%y",  localtime(&now));  // Formats date as: 062417
    strftime(hour_output, 6, "%I%M%S",  localtime(&now)); // Formats time as: 020549 maybe use backled to indicate am/pm? Should be obvous though
  }
  return hour_output;
}



void Clock::time_sync_notification_cb(struct timeval *tv)
{
    Serial.println("Notification of a time synchronization event");
    Serial.println(sntp_get_sync_status());
    Serial.print("Seconds since last sync = ");
    Serial.println(tv->tv_sec - prev_sync.tv_sec);
    prev_sync.tv_sec=tv->tv_sec;
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
