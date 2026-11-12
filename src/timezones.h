#pragma once
// The Knomi's time zones: network time (NTP) is UTC, and the game runs on local time.
// Sorted west to east by standard time. Each POSIX rule carries its daylight saving dates,
// so the clock changes on its own. The abbreviations are plain letters on purpose (no <+03>
// quoting), the form every C library reads. The IANA name is what test/test_tools.py checks
// each rule against, hour by hour over several years: keep it right when adding a city.
// Mac, Windows and the web follow the computer's own time zone and don't use this list.

struct TimeZoneDef {
  const char *city;   // shown on the picker (the same in every language)
  const char *iana;   // the zone it stands for (the test's reference)
  const char *posix;  // TZ rule for setenv/configTzTime
};

static const TimeZoneDef TIME_ZONES[] = {
  { "Honolulu", "Pacific/Honolulu", "HST10" },
  { "Anchorage", "America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0" },
  { "Los Angeles", "America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0" },
  { "Phoenix", "America/Phoenix", "MST7" },
  { "Denver", "America/Denver", "MST7MDT,M3.2.0,M11.1.0" },
  { "Mexico City", "America/Mexico_City", "CST6" },
  { "Chicago", "America/Chicago", "CST6CDT,M3.2.0,M11.1.0" },
  { "Bogota", "America/Bogota", "COT5" },
  { "New York", "America/New_York", "EST5EDT,M3.2.0,M11.1.0" },
  { "Toronto", "America/Toronto", "EST5EDT,M3.2.0,M11.1.0" },
  { "Caracas", "America/Caracas", "VET4" },
  { "Puerto Rico", "America/Puerto_Rico", "AST4" },
  { "Halifax", "America/Halifax", "AST4ADT,M3.2.0,M11.1.0" },
  { "Santiago", "America/Santiago", "CLT4CLST,M9.1.6/24,M4.1.6/24" },
  { "St. John's", "America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0" },
  { "Buenos Aires", "America/Argentina/Buenos_Aires", "ART3" },
  { "Sao Paulo", "America/Sao_Paulo", "BRT3" },
  { "London", "Europe/London", "GMT0BST,M3.5.0/1,M10.5.0" },
  { "Dublin", "Europe/Dublin", "GMT0IST,M3.5.0/1,M10.5.0" },
  { "Lisbon", "Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0" },
  { "Madrid", "Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Paris", "Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Berlin", "Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Rome", "Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Lagos", "Africa/Lagos", "WAT-1" },
  { "Athens", "Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
  { "Cairo", "Africa/Cairo", "EET-2EEST,M4.5.5/0,M10.5.4/24" },
  { "Johannesburg", "Africa/Johannesburg", "SAST-2" },
  { "Istanbul", "Europe/Istanbul", "TRT-3" },
  { "Moscow", "Europe/Moscow", "MSK-3" },
  { "Dubai", "Asia/Dubai", "GST-4" },
  { "Karachi", "Asia/Karachi", "PKT-5" },
  { "Delhi", "Asia/Kolkata", "IST-5:30" },
  { "Dhaka", "Asia/Dhaka", "BDT-6" },
  { "Bangkok", "Asia/Bangkok", "ICT-7" },
  { "Jakarta", "Asia/Jakarta", "WIB-7" },
  { "Singapore", "Asia/Singapore", "SGT-8" },
  { "Beijing", "Asia/Shanghai", "CST-8" },
  { "Manila", "Asia/Manila", "PHT-8" },
  { "Perth", "Australia/Perth", "AWST-8" },
  { "Seoul", "Asia/Seoul", "KST-9" },
  { "Tokyo", "Asia/Tokyo", "JST-9" },
  { "Adelaide", "Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3" },
  { "Brisbane", "Australia/Brisbane", "AEST-10" },
  { "Sydney", "Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3" },
  { "Auckland", "Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3" },
};
#define TIME_ZONE_COUNT ((int)(sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0])))
#define TIME_ZONE_DEFAULT 8  // New York: what every Knomi used before there was a choice
