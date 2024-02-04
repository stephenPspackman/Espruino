/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 *  This file is designed to be parsed during the build process
 *
 * JavaScript methods for Date Handling
 * ----------------------------------------------------------------------------
 */
#include "jswrap_date.h"
#include "jswrap_string.h"
#include "jsparse.h"
#include "jshardware.h"
#include "jslex.h"

#define MSDAY (24*60*60*1000)
#define BASE_DOW 4
static const char *MONTHNAMES = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
static const char *DAYNAMES = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";
static JsVarFloat parse_date(JsVar *str, JsVar **tz);

#ifdef ESPR_LIMIT_DATE_RANGE
// This rounds towards zero - which is not what the algorithm needs. Hence the range for Date() is further limited when ESPR_LIMIT_DATE_RANGE is set
#define INTEGER_DIVIDE_FLOOR(a,b) ((a)/(b))
#else
// This rounds down, which is what the algorithm needs
int integerDivideFloor(int a, int b) {
  return (a < 0 ? a-b+1 : a)/b;
}
#define INTEGER_DIVIDE_FLOOR(a,b) integerDivideFloor((a),(b))
#endif

// Convert y,m,d into a number of days since 1970, where 0<=m<=11
// https://github.com/deirdreobyrne/CalendarAndDST
static int get_day_number_from_date(int y, int m, int d) {
  int ans;
  
#ifdef ESPR_LIMIT_DATE_RANGE
  if (y < 1500 || y >= 1250000) { // Should actually work down to 1101, but since the Gregorian calendar started in 1582 . . .
#else
  if (y < -1250000 || y >= 1250000) {
#endif
    jsExceptionHere(JSET_ERROR, "Date out of bounds");
    return 0; // Need to head off any overflow error
  }
  while (m < 2) {
    y--;
    m+=12;
  }
  // #2456 was created by integer division rounding towards zero, rather than the FLOOR-behaviour required by the algorithm.
  ans = INTEGER_DIVIDE_FLOOR(y,100);
  return 365*y + INTEGER_DIVIDE_FLOOR(y,4) - ans + INTEGER_DIVIDE_FLOOR(ans,4) + 30*m + ((3*m+6)/5) + d - 719531;
}

// Convert a number of days since 1970 into y,m,d. 0<=m<=11
// https://github.com/deirdreobyrne/CalendarAndDST
static void get_date_from_day_number(int day, int *y, int *m, int *date) {
  int a = day + 135081;
  int b,c,d;

  // Bug #2456 fixed here too
  a = INTEGER_DIVIDE_FLOOR(a - INTEGER_DIVIDE_FLOOR(a,146097) + 146095,36524);
  a = day + a - INTEGER_DIVIDE_FLOOR(a,4);
  b = INTEGER_DIVIDE_FLOOR((a<<2)+2877911,1461);
  c = a + 719600 - 365*b - INTEGER_DIVIDE_FLOOR(b,4);
  d = (5*c-1)/153; // Floor behaviour not needed, as c is always positive
  if (date) *date=c-30*d-((3*d)/5);
  if (m) {
    if (d<14)
      *m=d-2;
    else
      *m=d-14;
  }
  if (y) {
    if (d>13)
      *y=b+1;
    else
      *y=b;
  }
}

#ifndef ESPR_NO_DAYLIGHT_SAVING
// Given a set of DST change settings, calculate the time (in GMT minutes since 1900) that the change happens in year y
// If as_local_time is true, then returns the number of minutes in the timezone in effect, as opposed to GMT
// Originally: https://github.com/deirdreobyrne/CalendarAndDST
static JsVarInt get_dst_change_time(int y, int month, int day, int dow, int timeOfDay) {
  int ans = get_day_number_from_date(y, month - (month > 2), 1);
  if (month == 2) ans -= 28; // Pseudo-February counts back from March.
  // ans % 7 is 0 for THU; (ans + 4) % 7 is 0 for SUN
  // ((14 - ((ans + 4) % 7) + dow) % 7) is zero if 1st is our dow, 1 if 1st is the day before our dow etc
  ans += day - 1;
  if (dow) ans += (10 - ans % 7 + dow) % 7;
  ans = ans * 1440 + timeOfDay;
  return ans + 36816480;
}

static JsVar *time_zone(JsVar *parent) {
  return (parent ? jspGetNamedField(parent, JS_TIMEZONE_VAR, false) : 0) ?:
      jspGetNamedField(execInfo.hiddenRoot, JS_TIMEZONE_VAR, false);
}

static JsVarInt get_local_time_descriptor(JsVar *buffer, JsVarInt size, JsVarInt trigger)
  {return jsvGetIntegerAndUnLock(jsvArrayBufferGet(buffer, size - (trigger & 0xF) - 1));}

static JsVarInt get_local_time_offset_from_descriptor(JsVarInt ltd)
  {return ltd << 0x04 >> 0x15; }

/* Time zone descriptors:

A time zone is described by a sequenced of 32 bit records, sorted
in _unsigned_ numerical order (except that order doesn't matter
between the C records at the end). These have three formats:

(A) Single fixed time transition. This records a moment at which
the wall clock time has changed or will change. Dates before 1900
are not supported.
 f e d c b a 9 8 7 6 5 4 3 2 1 0 f e d c b a 9 8 7 6 5 4 3 2 1 0
+-+-----------------------------------------------------+-------+
|0|                         instant                     | index |
+-+-----------------------------------------------------+-------+
  instant: Moment of change in minutes since 1900-1-1 00:00, GMT.
  index: Reverse index of associated local time descriptor.

(B) Repeating, date-driven transition. This records a wall clock
time when the time changes each year, on into the future.
 f e d c b a 9 8 7 6 5 4 3 2 1 0 f e d c b a 9 8 7 6 5 4 3 2 1 0
+---------+-------+---------+-----+---------------------+-------+
|1 0 0 0 0| month |   day   | dow |       minute        | index |
+---------+-------+---------+-----+---------------------+-------+0
  month:
    0,1: January, February
    2: Backuary, an imaginary month starting 28 days before March
    3-12: March - December
  day: 1-origin. 0 is reserved.
  dow: 1-origin, 1=Monday, 7=Sunday. 0 means no constraint.
  minute: local time, with both midnights valid (0 and 1440).
  index: Position of associated local time descriptor, as in (A).
The day represented is the first day satisfying the dow constraint
on or after the month and day specified.
Typically there would be either 0 or 2 type (B) entries, but
stepped double-daylight time with four entries has been used.

(C) Local time descriptor.
 f e d c b a 9 8 7 6 5 4 3 2 1 0 f e d c b a 9 8 7 6 5 4 3 2 1 0
+-------+---------------------+-+-------------------+-----------+
|1 1 0 0|       offset        |D|       start       |   length  |
+-------+---------------------+-+-------------------+-----------+
  offset: zone offset in minutes of delay from GMT: west is positive.
  D: true if nominally daylight time; returned by Date::getIsDST().
  start: start offset of the label for this state.
  length: length of the label for this state.
Start and length index into an externally stored table of state labels.
GMT might use "GMT" with a (sole) start:label of 0:3; America/Los_Angeles
might use "PDTPST" with values 3:3 (DST) and 0:3 (standard time). If
length is 0 a label is synthesised in the form of -1(DST) or +3:30.

How to encode relative descriptions (e.g. second Thursday or last
Sunday of the month):
  <n>th day of the month: day = n, dow = 0, using 1 for February.
  <n>th DOW of the month: day = 7n-6, using 1 for February.
  <n>th last day of month: day = e-n+1, using 2 (‘Backuary’) for February.
  <n>th last DOW of month: day = e-7n+1, using 2 for February.
where e is the more common length of the month (February has e=28).
*/
JsVarInt get_tz_local_time_descriptor(JsVarFloat ms, JsVar *tz, bool is_local_time, JsVarFloat *prev, JsVarFloat *next) {
  JsVar *d = jspGetNamedField(tz, "d", false);
  JsVarInt s = jsvGetLength(d);
  if (!jsvIsArrayBuffer(d) || !s) {
    jsvUnLock(d);
    if (prev) *prev = -INFINITY;
    if (next) *next = INFINITY;
    return -1;
  }
  JsVarInt start = -1;
  bool have_prev = false;
  unsigned end = 0x7FFFFFFF;
  JsVarInt min = ms / (60 * 1000) + 36816480; // Or … should we use 1887 (advent of time zones)?
  JsVarInt bound = is_local_time ? min - 16 * 60 : min; 
  JsvArrayBufferIterator it;
  // The data structure is designed so it could be binary or interpolation searched
  // in the unlikely event that someone wanted to do historical calendar work, but
  // array buffers seem to favour sequential access and typical embedded uses would
  // want a year or two of historical data, at most.
  jsvArrayBufferIteratorNew(&it, d, 0);
  while (jsvArrayBufferIteratorHasElement(&it)) {
    // Unsigned comparison ensures that repeating triggers are not accepted here.
    unsigned trig = jsvArrayBufferIteratorGetIntegerValue(&it);
    if ((trig >> 4) <= bound
        || is_local_time
        && (trig >> 4) <= min - get_local_time_offset_from_descriptor(get_local_time_descriptor(d, s, trig))) {
      have_prev = start >= 0;
      start = trig;
      jsvArrayBufferIteratorNext(&it);
    } else {
      end = trig;
      break;
    }
  }
  JsVarInt r = start >= 0 ? get_local_time_descriptor(d, s, start) : -1;
  if ((end & 0xC0000000) == 0x80000000) { // We must consider a rule
    JsVarInt trigs[4]; // Enough for stepped double daylight time.
    trigs[0] = end;
    int n = 1;
    jsvArrayBufferIteratorNext(&it);
    while (jsvArrayBufferIteratorHasElement(&it)) {
      JsVarInt x = jsvArrayBufferIteratorGetIntegerValue(&it);
      if (x & 0x40000000 || n == 4) break;
      jsvArrayBufferIteratorNext(&it);
      trigs[n++] = x;
    }
    int yr, mo, dy;
    // We need to get back a full cycle, which is: a long year, plus a week for
    // day-of-week adjustments, plus a day because of time zones themselves.
    get_date_from_day_number((bound - 36816480) / (24 * 60) - 366 - 7 - 1, &yr, &mo, &dy);
    int i;
    JsVarInt rebound = 0x80000000 | mo + !!mo << 0x17 | dy << 0x12;
    for (i = 0; i < n && trigs[i] < rebound; i++) ;
    if (i == n) yr++, i = 0;
    for (;;) {
      JsVarInt trig = get_dst_change_time(
          yr, trigs[i] >> 0x17 & 0xF, trigs[i] >> 0x12 & 0x1F, trigs[i] >> 0x0F & 0x7,
          trigs[i] >> 0x04 & 0x7ff
        ) - get_local_time_offset_from_descriptor(r);
      // Ignore any prefix of generated times that overlap listed ones.
      if ((start >> 0x04) + 16 * 60 < trig) {
        if (trig <= bound 
              || is_local_time && trig <= min - get_local_time_offset_from_descriptor(get_local_time_descriptor(d, s, trigs[i]))) {
            have_prev = true;
            start = trig << 0x04;
            r = get_local_time_descriptor(d, s, trigs[i]);
          } else {
            end = trig << 0x04;
            break;
          }
      }
      if (++i == n) yr++, i = 0;
    }
  }
  if (prev) *prev = have_prev ? ((start >> 0x04) - 36816480) * 60. * 1000 : -INFINITY;
  if (next) *next = end < 0x7FFFFFFF ? ((end >> 0x04) - 36816480) * 60. * 1000 : INFINITY;
  jsvArrayBufferIteratorFree(&it);
  jsvUnLock(d);
  return (r & 0xC0000000) == 0xC0000000 ? r : -1; // In case of malformed table.
}
#endif

// Returns the effective timezone in minutes east
// is_local_time is true if ms is referenced to local time, false if it's referenced to GMT
// if is_dst is not null, then it will be set according as DST is in effect
// https://github.com/deirdreobyrne/CalendarAndDST
int jsdGetEffectiveTimeZone(JsVarFloat ms, JsVar *tz, bool is_local_time, bool *is_dst) {
  if (is_dst) *is_dst = false;
#ifndef ESPR_NO_DAYLIGHT_SAVING
  if (tz && !jsvIsNumeric(tz)) {
    // We might cache the LTD, though it would take some replumbing to expose the parent.
    JsVarInt ltd = get_tz_local_time_descriptor(ms, tz, is_local_time, 0, 0);
    if (ltd == -1) return 0; // No data were loaded for this time
    if (is_dst) *is_dst = !!(ltd & (1 << 0x10));
    return get_local_time_offset_from_descriptor(ltd);
  }
#endif
  return jsvGetFloat(tz) * 60;
}

/* NOTE: we use / and % here because the compiler is smart enough to
 * condense them into one op. */
// If tz is null, then GMT.
TimeInDay getTimeFromMilliSeconds(JsVarFloat ms_in, JsVar *tz) {
  TimeInDay t;
  if(jsvIsNullish(tz)) {
    t.zone = t.is_dst = 0;
  } else {
    t.zone = jsdGetEffectiveTimeZone(ms_in, tz, false, &(t.is_dst));
    ms_in += t.zone*60000;
  }
  t.daysSinceEpoch = (int)(ms_in / MSDAY);
  int ms = (int)(ms_in - ((JsVarFloat)t.daysSinceEpoch * MSDAY));
  if (ms<0) {
    ms += MSDAY;
    t.daysSinceEpoch--;
  }
  int s = ms / 1000;
  t.ms = ms % 1000;
  t.hour = s / 3600;
  s = s % 3600;
  t.min = s/60;
  t.sec = s%60;

  return t;
}

JsVarFloat fromTimeInDay(TimeInDay *td, JsVar *tz) {
  if (tz) {
    td->zone = jsdGetEffectiveTimeZone(fromTimeInDay(td,0),tz,true,&td->is_dst);
  }
  return (JsVarFloat)(td->ms + (((td->hour*60+td->min - td->zone)*60+td->sec)*1000) + (JsVarFloat)td->daysSinceEpoch*MSDAY);
}

static JsVarFloat from_time_in_day_with_default(TimeInDay *td, JsVar *tz) {
  JsVarFloat r;
  if (!tz) {
    tz = time_zone(0);
    r = fromTimeInDay(td, tz);
    jsvUnLock(tz);
  } else r = fromTimeInDay(td, tz);
  return r;
}

CalendarDate getCalendarDate(int d) {
  CalendarDate date;

  get_date_from_day_number(d, &date.year, &date.month, &date.day);
  date.daysSinceEpoch = d;
  // Calculate day of week. Sunday is 0
  date.dow=(date.daysSinceEpoch+BASE_DOW)%7;
  if (date.dow<0) date.dow+=7;
  return date;
};

int fromCalendarDate(CalendarDate *date) {
  while (date->month < 0) {
    date->year--;
    date->month += 12;
  }
  while (date->month > 11) {
    date->year++;
    date->month -= 12;
  }
  return get_day_number_from_date(date->year, date->month, date->day);
};


static int getMonth(const char *s) {
  int i;
  for (i=0;i<12;i++)
    if (s[0]==MONTHNAMES[i*4] &&
        s[1]==MONTHNAMES[i*4+1] &&
        s[2]==MONTHNAMES[i*4+2])
      return i;
  return -1;
}

static int getDay(const char *s) {
  int i;
  for (i=0;i<7;i++)
    if (strcmp(s, &DAYNAMES[i*4])==0)
      return i;
  return -1;
}

static TimeInDay getTimeFromDateVar(JsVar *date, bool forceGMT) {
  JsVar *tz = forceGMT ? 0 : time_zone(date);
  TimeInDay r = getTimeFromMilliSeconds(jswrap_date_getTime(date), tz);
  jsvUnLock(tz);
  return r;
}

static CalendarDate getCalendarDateFromDateVar(JsVar *date, bool forceGMT) {
  JsVar *tz = forceGMT ? 0 : time_zone(date);
  CalendarDate r = getCalendarDate(getTimeFromDateVar(date, tz).daysSinceEpoch);
  jsvUnLock(tz);
  return r;
}

/*JSON{
  "type" : "class",
  "class" : "Date"
}
The built-in class for handling Dates.

**Note:** Initially the time zone is GMT+0, however you can change the default timezone
using the `E.setTimeZone(...)` or E.setDST functions.

For example `E.setTimeZone(1)` will be GMT+0100.

 */

/*JSON{
  "type" : "staticmethod",
  "class" : "Date",
  "name" : "now",
  "generate" : "jswrap_date_now",
  "return" : ["float",""]
}
Get the number of milliseconds elapsed since 1970 (or on embedded platforms,
since startup).

**Note:** Desktop JS engines return an integer value for `Date.now()`, however Espruino
returns a floating point value, accurate to fractions of a millisecond.
 */
JsVarFloat jswrap_date_now() {
  // Not quite sure why we need this, but (JsVarFloat)jshGetSystemTime() / (JsVarFloat)jshGetTimeFromMilliseconds(1) in inaccurate on STM32
  return ((JsVarFloat)jshGetSystemTime() / (JsVarFloat)jshGetTimeFromMilliseconds(1000)) * 1000;
}


JsVar *jswrap_date_from_milliseconds(JsVarFloat time) {
  JsVar *d = jspNewObject(0,"Date");
  jswrap_date_setTime(d, time);
  return d;
}

static JsVar *zoned_date_from_milliseconds_and_unlock(JsVarFloat time, JsVar *tz) {
  JsVar *r = jswrap_date_from_milliseconds(time);
  if (tz && r) jsvObjectSetChildAndUnLock(r, JS_TIMEZONE_VAR, tz);
  return r;
}

// Note: unlocks *tz.
static JsVar *date_constructor(JsVar *args, JsVarInt offs, JsVar **tz) {
  JsVarFloat time = 0;
  if (jsvGetArrayLength(args) == offs) {
    time = jswrap_date_now();
  } else if (jsvGetArrayLength(args) == offs + 1) {
    JsVar *arg = jsvGetArrayItem(args, offs);
    if (jsvIsNumeric(arg))
      time = jsvGetFloat(arg);
    else if (jsvIsString(arg))
      time = parse_date(arg, tz); // Note a tz of true could be replaced here.
    else
      jsExceptionHere(JSET_TYPEERROR, "Variables of type %t are not supported in date constructor", arg);
    jsvUnLock(arg);
  } else {
    CalendarDate date;
    date.year = (int)jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++));
    date.month = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    date.day = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    TimeInDay td;
    td.daysSinceEpoch = fromCalendarDate(&date);
    td.hour = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    td.min = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    td.sec = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    td.ms = (int)(jsvGetIntegerAndUnLock(jsvGetArrayItem(args, offs++)));
    td.zone = 0;
    time = from_time_in_day_with_default(&td, *tz);
  }
  return *tz
    ? zoned_date_from_milliseconds_and_unlock(time, *tz)
    : jswrap_date_from_milliseconds(time);
}

/*JSON{
  "type" : "constructor",
  "class" : "Date",
  "name" : "Date",
  "generate" : "jswrap_date_constructor",
  "params" : [
    ["args","JsVarArray","Either nothing (current time), one numeric argument (milliseconds since 1970), a date string (see `Date.parse`), or [year, month, day, hour, minute, second, millisecond]"]
  ],
  "return" : ["JsVar","A Date object"],
  "return_object" : "Date",
  "typescript" : [
    "new(): Date;",
    "new(value: number | string): Date;",
    "new(year: number, month: number, date?: number, hours?: number, minutes?: number, seconds?: number, ms?: number): Date;",
    "(arg?: any): string;"
  ]
}
Creates a date object.
 */
JsVar *jswrap_date_constructor(JsVar *args) {
  JsVar *tz = 0; // No inherent zone.
  return date_constructor(args, 0, &tz);
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Date",
  "name" : "zoned",
  "generate" : "jswrap_date_zoned",
  "params" : [
    ["args","JsVarArray","A time zone followed by arguments for the Date constructor (q.v.)"]
  ],
  "return" : ["JsVar","A Date object with inherent time zone"],
  "return_object" : "Date",
  "typescript" : [
    "new(tz?: boolean | number | {i:? string, d: number[], l:? string}): Date;",
    "new(tz: boolean | number | {i:? string, d: number[], l:? string}, value: number | string): Date;",
    "new(tz: number | {i:? string, d: number[], l:? string}, year: number, month: number, date?: number, hours?: number, minutes?: number, seconds?: number, ms?: number): Date;",
    "(arg?: any): string;"
  ]
}
Creates a date object with an inherent time zone. The first argument (which defaults to false)
represents the time zone, and any subsequent arguments follow the conventions of `new Date()`. A Boolean
time zone argument means to capture the global setting from `E.setTimeZone()` or `E.setDST()`, or,
if true and associated with a string parameter having an explicit GMT offset, one captured from that
string.
 */
JsVar *jswrap_date_zoned(JsVar *args) {
  JsVar *tz = jsvGetArrayLength(args) ? jsvGetArrayItem(args, 0) : jsvNewFromBool(false);
  return date_constructor(args, 1, &tz);
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "getTimezoneOffset",
  "generate" : "jswrap_date_getTimezoneOffset",
  "return" : ["int32","The difference, in minutes, between UTC and local time"]
}
This returns the time-zone offset from UTC, in minutes.

 */
int jswrap_date_getTimezoneOffset(JsVar *parent) {
  return -getTimeFromDateVar(parent, false/*system timezone*/).zone;
}

// I'm assuming SAVE_ON_FLASH always goes with ESPR_NO_DAYLIGHT_SAVING
/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "getIsDST",
  "generate" : "jswrap_date_getIsDST",
  "return" : ["bool","true if daylight savings time is in effect"],
  "typescript" : "getIsDST(): boolean"
}
This returns a boolean indicating whether daylight savings time is in effect.

 */
bool jswrap_date_getIsDST(JsVar *parent) {
  return getTimeFromDateVar(parent, false/*system timezone*/).is_dst ? 1 : 0;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getTime",
  "generate" : "jswrap_date_getTime",
  "return" : ["float",""]
}
Return the number of milliseconds since 1970
 */
/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "valueOf",
  "generate" : "jswrap_date_getTime",
  "return" : ["float",""]
}
Return the number of milliseconds since 1970
 */
JsVarFloat jswrap_date_getTime(JsVar *date) {
  return jsvObjectGetFloatChild(date, "ms");
}
/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "setTime",
  "generate" : "jswrap_date_setTime",
  "params" : [
    ["timeValue","float","the number of milliseconds since 1970"]
  ],
  "return" : ["float","the number of milliseconds since 1970"]
}
Set the time/date of this Date class
 */
JsVarFloat jswrap_date_setTime(JsVar *date, JsVarFloat timeValue) {
#ifdef ESPR_LIMIT_DATE_RANGE
  if (timeValue < -1.48317696e13 || timeValue >= 3.93840543168E+016) { // This should actually work down to 1101AD . . .
#else
  if (timeValue < -3.95083256832E+016 || timeValue >= 3.93840543168E+016) {
#endif
    jsExceptionHere(JSET_ERROR, "Date out of bounds");
    return 0.0;
  }
  if (date)
    jsvObjectSetChildAndUnLock(date, "ms", jsvNewFromFloat(timeValue));
  return timeValue;
}


/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getHours",
  "generate" : "jswrap_date_getHours",
  "return" : ["int32",""]
}
0..23
 */
int jswrap_date_getHours(JsVar *parent) {
  return getTimeFromDateVar(parent, false/*system timezone*/).hour;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getMinutes",
  "generate" : "jswrap_date_getMinutes",
  "return" : ["int32",""]
}
0..59
 */
int jswrap_date_getMinutes(JsVar *parent) {
  return getTimeFromDateVar(parent, false/*system timezone*/).min;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getSeconds",
  "generate" : "jswrap_date_getSeconds",
  "return" : ["int32",""]
}
0..59
 */
int jswrap_date_getSeconds(JsVar *parent) {
  return getTimeFromDateVar(parent, false/*system timezone*/).sec;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getMilliseconds",
  "generate" : "jswrap_date_getMilliseconds",
  "return" : ["int32",""]
}
0..999
 */
int jswrap_date_getMilliseconds(JsVar *parent) {
  return getTimeFromDateVar(parent, false/*system timezone*/).ms;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getDay",
  "generate" : "jswrap_date_getDay",
  "return" : ["int32",""]
}
Day of the week (0=sunday, 1=monday, etc)
 */
int jswrap_date_getDay(JsVar *parent) {
  return getCalendarDateFromDateVar(parent, false/*system timezone*/).dow;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getDate",
  "generate" : "jswrap_date_getDate",
  "return" : ["int32",""]
}
Day of the month 1..31
 */
int jswrap_date_getDate(JsVar *parent) {
  return getCalendarDateFromDateVar(parent, false/*system timezone*/).day;
}


/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getMonth",
  "generate" : "jswrap_date_getMonth",
  "return" : ["int32",""]
}
Month of the year 0..11
 */
int jswrap_date_getMonth(JsVar *parent) {
  return getCalendarDateFromDateVar(parent, false/*system timezone*/).month;
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getFullYear",
  "generate" : "jswrap_date_getFullYear",
  "return" : ["int32",""]
}
The year, e.g. 2014
 */
int jswrap_date_getFullYear(JsVar *parent) {
  return getCalendarDateFromDateVar(parent, false/*system timezone*/).year;
}

#ifndef ESPR_NO_DAYLIGHT_SAVINGS
/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "getTimezone",
  "generate" : "jswrap_date_getTimezone",
  "return" : ["JsVar",""]
}
Returns the inherent time zone of a Date, if any. This object may be highly encoded, and
is most useful for transferring to other objects.
 */
JsVar *jswrap_date_getTimezone(JsVar *parent) {
  return jsvObjectGetChildIfExists(parent, JS_TIMEZONE_VAR);
}
#endif

/// -------------------------------------------------------

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setHours",
  "generate" : "jswrap_date_setHours",
  "params" : [
    ["hoursValue","int","number of hours, 0..23"],
    ["minutesValue","JsVar","number of minutes, 0..59"],
    ["secondsValue","JsVar","[optional] number of seconds, 0..59"],
    ["millisecondsValue","JsVar","[optional] number of milliseconds, 0..999"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "setHours(hoursValue: number, minutesValue?: number, secondsValue?: number, millisecondsValue?: number): number;"
}
0..23
 */
JsVarFloat jswrap_date_setHours(JsVar *parent, int hoursValue, JsVar *minutesValue, JsVar *secondsValue, JsVar *millisecondsValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  td.hour = hoursValue;
  if (jsvIsNumeric(minutesValue))
    td.min = jsvGetInteger(minutesValue);
  if (jsvIsNumeric(secondsValue))
    td.sec = jsvGetInteger(secondsValue);
  if (jsvIsNumeric(millisecondsValue))
    td.ms = jsvGetInteger(millisecondsValue);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setMinutes",
  "generate" : "jswrap_date_setMinutes",
  "params" : [
    ["minutesValue","int","number of minutes, 0..59"],
    ["secondsValue","JsVar","[optional] number of seconds, 0..59"],
    ["millisecondsValue","JsVar","[optional] number of milliseconds, 0..999"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "setMinutes(minutesValue: number, secondsValue?: number, millisecondsValue?: number): number;"
}
0..59
 */
JsVarFloat jswrap_date_setMinutes(JsVar *parent, int minutesValue, JsVar *secondsValue, JsVar *millisecondsValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  td.min = minutesValue;
  if (jsvIsNumeric(secondsValue))
    td.sec = jsvGetInteger(secondsValue);
  if (jsvIsNumeric(millisecondsValue))
    td.ms = jsvGetInteger(millisecondsValue);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setSeconds",
  "generate" : "jswrap_date_setSeconds",
  "params" : [
    ["secondsValue","int","number of seconds, 0..59"],
    ["millisecondsValue","JsVar","[optional] number of milliseconds, 0..999"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "setSeconds(secondsValue: number, millisecondsValue?: number): number;"
}
0..59
 */
JsVarFloat jswrap_date_setSeconds(JsVar *parent, int secondsValue, JsVar *millisecondsValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  td.sec = secondsValue;
  if (jsvIsNumeric(millisecondsValue))
    td.ms = jsvGetInteger(millisecondsValue);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setMilliseconds",
  "generate" : "jswrap_date_setMilliseconds",
  "params" : [
    ["millisecondsValue","int","number of milliseconds, 0..999"]
  ],
  "return" : ["float","The number of milliseconds since 1970"]
}
 */
JsVarFloat jswrap_date_setMilliseconds(JsVar *parent, int millisecondsValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  td.ms = millisecondsValue;
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setDate",
  "generate" : "jswrap_date_setDate",
  "params" : [
    ["dayValue","int","the day of the month, between 0 and 31"]
  ],
  "return" : ["float","The number of milliseconds since 1970"]
}
Day of the month 1..31
 */
JsVarFloat jswrap_date_setDate(JsVar *parent, int dayValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  CalendarDate d = getCalendarDate(td.daysSinceEpoch);
  d.day = dayValue;
  td.daysSinceEpoch = fromCalendarDate(&d);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}


/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setMonth",
  "generate" : "jswrap_date_setMonth",
  "params" : [
    ["monthValue","int","The month, between 0 and 11"],
    ["dayValue","JsVar","[optional] the day, between 0 and 31"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "setMonth(monthValue: number, dayValue?: number): number;"
}
Month of the year 0..11
 */
JsVarFloat jswrap_date_setMonth(JsVar *parent, int monthValue, JsVar *dayValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  CalendarDate d = getCalendarDate(td.daysSinceEpoch);
  d.month = monthValue;
  if (jsvIsNumeric(dayValue))
    d.day = jsvGetInteger(dayValue);
  td.daysSinceEpoch = fromCalendarDate(&d);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "setFullYear",
  "generate" : "jswrap_date_setFullYear",
  "params" : [
    ["yearValue","int","The full year - eg. 1989"],
    ["monthValue","JsVar","[optional] the month, between 0 and 11"],
    ["dayValue","JsVar","[optional] the day, between 0 and 31"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "setFullYear(yearValue: number, monthValue?: number, dayValue?: number): number;"
}
 */
JsVarFloat jswrap_date_setFullYear(JsVar *parent, int yearValue, JsVar *monthValue, JsVar *dayValue) {
  TimeInDay td = getTimeFromDateVar(parent, false/*system timezone*/);
  CalendarDate d = getCalendarDate(td.daysSinceEpoch);
  d.year = yearValue;
  if (jsvIsNumeric(monthValue))
    d.month = jsvGetInteger(monthValue);
  if (jsvIsNumeric(dayValue))
    d.day = jsvGetInteger(dayValue);
  td.daysSinceEpoch = fromCalendarDate(&d);
  return jswrap_date_setTime(parent, fromTimeInDay(&td,parent));
}

#ifndef ESPR_NO_DAYLIGHT_SAVINGS
/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "setTimezone",
  "generate" : "jswrap_date_setTimezone",
  "params" : [["timeZone","JsVar","The time zone, either a floating point offset from GMT or an encoded time zone descriptor"]],
  "return" : ["int","The updated difference, in minutes, between UTC and this time"]
}
Sets the inherent time zone of a Date.
 */
int jswrap_date_setTimezone(JsVar *parent, JsVar *timeZone) {
  jsvObjectSetChild(parent, JS_TIMEZONE_VAR, timeZone);
  return jswrap_date_getTimezoneOffset(parent);
}
#endif

/// -------------------------------------------------------


/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "toString",
  "generate" : "jswrap_date_toString",
  "return" : ["JsVar","A String"],
  "typescript" : "toString(): string;"
}
Converts to a String, e.g: `Fri Jun 20 2014 14:52:20 GMT+0000`

 **Note:** This uses the timezone inherent to the object, or by default that set
with `E.setTimeZone()` or `E.setDST()`
*/
JsVar *jswrap_date_toString(JsVar *parent) {
  TimeInDay time = getTimeFromDateVar(parent, false/*proper timezone*/);
  CalendarDate date = getCalendarDate(time.daysSinceEpoch);
  char zonesign;
  int zone;
  if (time.zone<0) {
    zone = -time.zone;
    zonesign = '-';
  } else {
    zone = +time.zone;
    zonesign = '+';
  }
  return jsvVarPrintf("%s %s %d %d %02d:%02d:%02d GMT%c%04d",
      &DAYNAMES[date.dow*4], &MONTHNAMES[date.month*4], date.day, date.year,
      time.hour, time.min, time.sec,
      zonesign, ((zone/60)*100)+(zone%60));
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "toUTCString",
  "generate" : "jswrap_date_toUTCString",
  "return" : ["JsVar","A String"],
  "typescript" : "toUTCString(): string;"
}
Converts to a String, e.g: `Fri, 20 Jun 2014 14:52:20 GMT`

 **Note:** This always assumes a timezone of GMT
 */
JsVar *jswrap_date_toUTCString(JsVar *parent) {
  TimeInDay time = getTimeFromDateVar(parent, true/*GMT*/);
  CalendarDate date = getCalendarDate(time.daysSinceEpoch);

  return jsvVarPrintf("%s, %d %s %d %02d:%02d:%02d GMT", &DAYNAMES[date.dow*4], date.day, &MONTHNAMES[date.month*4], date.year, time.hour, time.min, time.sec);
}

/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "toISOString",
  "generate" : "jswrap_date_toISOString",
  "return" : ["JsVar","A String"],
  "typescript" : "toISOString(): string;"
}
Converts to a ISO 8601 String, e.g: `2014-06-20T14:52:20.123Z`

 **Note:** This always assumes a timezone of GMT
 */
/*JSON{
  "type" : "method",
  "class" : "Date",
  "name" : "toJSON",
  "generate" : "jswrap_date_toISOString",
  "return" : ["JsVar","A String"],
  "typescript" : "toJSON(): string;"
}
Calls `Date.toISOString` to output this date to JSON
*/
JsVar *jswrap_date_toISOString(JsVar *parent) {
  TimeInDay time = getTimeFromDateVar(parent, true/*GMT*/);
  CalendarDate date = getCalendarDate(time.daysSinceEpoch);

  return jsvVarPrintf("%d-%02d-%02dT%02d:%02d:%02d.%03dZ", date.year, date.month+1, date.day, time.hour, time.min, time.sec, time.ms);
}
/*JSON{
  "type" : "method",
  "ifndef" : "SAVE_ON_FLASH",
  "class" : "Date",
  "name" : "toLocalISOString",
  "generate" : "jswrap_date_toLocalISOString",
  "return" : ["JsVar","A String"],
  "typescript" : "toLocalISOString(): string;"
}
Converts to a ISO 8601 String (with timezone information), e.g:
`2014-06-20T14:52:20.123-0500`
 */
JsVar *jswrap_date_toLocalISOString(JsVar *parent) {
  TimeInDay time = getTimeFromDateVar(parent, false/*proper timezone*/);
  CalendarDate date = getCalendarDate(time.daysSinceEpoch);
  char zonesign;
  int zone;
  if (time.zone<0) {
    zone = -time.zone;
    zonesign = '-';
  } else {
    zone = +time.zone;
    zonesign = '+';
  }
  zone = 100*(zone/60) + (zone%60);
  return jsvVarPrintf("%d-%02d-%02dT%02d:%02d:%02d.%03d%c%04d", date.year, date.month+1, date.day, time.hour, time.min, time.sec, time.ms, zonesign, zone);
}

static JsVarInt _parse_int() {
  return (int)stringToIntWithRadix(jslGetTokenValueAsString(), 10, NULL, NULL);
}

static bool _parse_time(TimeInDay *time, bool *timezoneSet, int initialChars) {
  *timezoneSet = false;
  time->hour = (int)stringToIntWithRadix(&jslGetTokenValueAsString()[initialChars], 10, NULL, NULL);
  jslGetNextToken();
  if (lex->tk==':') {
    jslGetNextToken();
    if (lex->tk == LEX_INT) {
      time->min = _parse_int();
      jslGetNextToken();
      if (lex->tk==':') {
        jslGetNextToken();
        if (lex->tk == LEX_INT || lex->tk == LEX_FLOAT) {
          JsVarFloat f = stringToFloat(jslGetTokenValueAsString());
          time->sec = (int)f;
          time->ms = (int)(f*1000) % 1000;
          jslGetNextToken();
        }
      }
      if (lex->tk == LEX_ID) {
        const char *tkstr = jslGetTokenValueAsString();
        if (strcmp(tkstr,"GMT")==0 || strcmp(tkstr,"Z")==0) {
          time->zone = 0;
          *timezoneSet = true;
          jslGetNextToken();
          if (lex->tk == LEX_EOF) return true;
        }
      }
      if (lex->tk == '+' || lex->tk == '-') {
        int sign = lex->tk == '+' ? 1 : -1;
        jslGetNextToken();
        if (lex->tk == LEX_INT) {
          int i = _parse_int();
          // correct the fact that it's HHMM and turn it into just minutes
          i = (i%100) + ((i/100)*60);
          time->zone = i*sign;
          *timezoneSet = true;
          jslGetNextToken();
        }
      }
      return true;
    }
  }
  return false;
}

// tz is an in/out parameter. If it is (exactly) true, then it is replaced by the timezone
// found in the input string.
static JsVarFloat parse_date(JsVar *str, JsVar **tz) {
  if (!jsvIsString(str)) return 0;
  TimeInDay time;
  time.daysSinceEpoch = 0;
  time.hour = 0;
  time.min = 0;
  time.sec = 0;
  time.ms = 0;
  time.zone = 0;
  time.is_dst = false;
  CalendarDate date = getCalendarDate(0);
  bool timezoneSet = false;

  JsLex lex;
  JsLex *oldLex = jslSetLex(&lex);
  jslInit(str);

  if (lex.tk == LEX_ID) {
    date.month = getMonth(jslGetTokenValueAsString());
    date.dow = getDay(jslGetTokenValueAsString());
    if (date.month>=0) {
      // Aug 9, 1995
      jslGetNextToken();
      if (lex.tk == LEX_INT) {
        date.day = _parse_int();
        jslGetNextToken();
        if (lex.tk==',') {
          jslGetNextToken();
          if (lex.tk == LEX_INT) {
            date.year = _parse_int();
            time.daysSinceEpoch = fromCalendarDate(&date);
            jslGetNextToken();
            if (lex.tk == LEX_INT) {
              _parse_time(&time, &timezoneSet, 0);
            }
          }
        }
      }
    } else if (date.dow>=0) {
      // Mon, 25 Dec 1995
      date.month = 0;
      jslGetNextToken();
      if (lex.tk==',') {
        jslGetNextToken();
        if (lex.tk == LEX_INT) {
          date.day = _parse_int();
          jslGetNextToken();
          if (lex.tk == LEX_ID && getMonth(jslGetTokenValueAsString())>=0) {
            date.month = getMonth(jslGetTokenValueAsString());
            jslGetNextToken();
            if (lex.tk == LEX_INT) {
              date.year = _parse_int();
              time.daysSinceEpoch = fromCalendarDate(&date);
              jslGetNextToken();
              if (lex.tk == LEX_INT) {
                _parse_time(&time, &timezoneSet, 0);
              }
            }
          }
        }
      }
    } else {
      date.dow = 0;
      date.month = 0;
    }
  } else if (lex.tk == LEX_INT) {
    // assume 2011-10-10T14:48:00 format
    date.year = _parse_int();
    jslGetNextToken();
    if (lex.tk=='-') {
      jslGetNextToken();
      if (lex.tk == LEX_INT) {
        date.month = _parse_int() - 1;
        jslGetNextToken();
        if (lex.tk=='-') {
          jslGetNextToken();
          if (lex.tk == LEX_INT) {
            date.day = _parse_int();
            time.daysSinceEpoch = fromCalendarDate(&date);
            jslGetNextToken();
            if (lex.tk == LEX_ID && jslGetTokenValueAsString()[0]=='T' || lex.tk == LEX_INT) {
              _parse_time(&time, &timezoneSet, lex.tk == LEX_ID);
            }
          }
        }
      }
    }
  }
  jslKill();
  jslSetLex(oldLex);
  if (jsvIsBoolean(*tz)) {
    *tz = jsvGetBoolAndUnLock(*tz) && timezoneSet ? jsvNewFromFloat(time.zone / 60.) : 0;
  }
  return timezoneSet ? fromTimeInDay(&time, 0) : from_time_in_day_with_default(&time, *tz);
}

/*JSON{
  "type" : "staticmethod",
  "class" : "Date",
  "name" : "parse",
  "generate" : "jswrap_date_parse",
  "params" : [
    ["str","JsVar","A String"],
    ["tz","JsVar","[optional] A specific timezone"]
  ],
  "return" : ["float","The number of milliseconds since 1970"],
  "typescript" : "parse(str: string, tz?: number | {i:? string, d: number[], l:? string}): number;"
}
Parse a date string and return milliseconds since 1970. Data can be either
'2011-10-20T14:48:00', '2011-10-20', '2011-10-20 14:48:00' or 'Mon, 25 Dec 1995 13:30:00 +0430'
 */
JsVarFloat jswrap_date_parse(JsVar *str, JsVar *tz) {
  return parse_date(str, jsvIsBoolean(tz) ? 0 : &tz);
}

static JsVarInt get_tz_local_time_descriptor_from_date(JsVar *date, JsVar *tz) {
  if (jsvIsObject(tz)) {
    // We might cache the ltd here.
    return get_tz_local_time_descriptor(jsvObjectGetFloatChild(date, "ms"), tz, false, 0, 0);
  } else {
    return (JsVarInt)(0xc0000000 | ((int)(jsvGetFloat(tz) * 60) & 0x03ff) << 0x11);
  }
}

#ifndef ESPR_NO_DAYLIGHT_SAVING
static JsVar *string_from_local_time_descriptor_offset(int ltd, bool suffix) {
  bool dst = suffix && !!(ltd & 1 << 0x10);
  ltd = get_local_time_offset_from_descriptor(ltd);
  char sign = '+';
  if (ltd < 0) {
    ltd = -ltd;
    sign = '-';
  }
  return ltd % 60 ?
    jsvVarPrintf("%c%d:%02d%s", sign, ltd / 60, ltd % 60, dst ? "*" : "") :
    jsvVarPrintf("%c%d%s", sign, ltd / 60, dst ? "(DST)" : "");
}

JsVar *time_label(JsVar *date, bool suffix) {
  JsVar *tz = time_zone(date);
  JsVar *l = jspGetNamedField(tz, "ll", false);
  JsVarInt ltd = get_tz_local_time_descriptor_from_date(date, tz);
  JsVar *r = 0;
  if (jsvIsString(l) && ltd & 0x3f) {
    JsVar *len = jsvNewFromInteger(ltd & 0x3f);
    r = jswrap_string_substr(l, ltd >> 0x06 & 0x3ff, len);
    jsvUnLock(len);
  }
  if (!r) {
    JsVar *t = jswrap_date_getNextTransition(date);
    if (t) jsvUnLock(t);
    else r = jspGetNamedField(tz, "l", false);
  }
  if (!r) r = string_from_local_time_descriptor_offset(ltd, suffix);
  jsvUnLock2(tz, l);
  return r;
}

/*JSON{
  "type" : "method",
  "ifndef" : "ESPR_NO_DAYLIGHT_SAVING",
  "class" : "Date",
  "name" : "getTimezoneLabel",
  "generate" : "jswrap_date_getTimezoneLabel",
  "return" : ["JsVar", "A short user-facing label for the time-zone (year-round)"],
  "return_object" : "String",
  "typescript" : "getTimezoneLabel(date: Date) : string"
}
This returns the short (typically 2–6 character) user-facing label for the overall time-zone
(e.g. PT), if known, or a synthetisised stand-in like +3:30 or -8/-7.

*/
JsVar *jswrap_date_getTimezoneLabel(JsVar *date) {
  JsVar *tz = time_zone(date);
  JsVar *r = jspGetNamedField(tz, "l", false);
  if (!r) {
    JsVar *next = jswrap_date_getNextTransition(date);
    if (next) {
      // Synthesising from current and next is not perfect, but it's probably better than nothing
      JsVarInt a = get_tz_local_time_descriptor_from_date(date, tz);
      JsVarInt b = get_tz_local_time_descriptor_from_date(next, tz);
      JsVar *ar = time_label(date, false);
      JsVar *br = time_label(next, false);
      if (a & 1 << 0x10 && !(b & 1 << 0x10)) {JsVar *t = ar; ar = br; br = t;}
      r = jsvVarPrintf("%v/%v", ar, br);
      jsvUnLock3(next, ar, br);
    } else {
      r = string_from_local_time_descriptor_offset(get_tz_local_time_descriptor_from_date(date, tz), true);
    }
  }
  jsvUnLock(tz);
  return r;
}

/*JSON{
  "type" : "method",
  "ifndef" : "ESPR_NO_DAYLIGHT_SAVING",
  "class" : "Date",
  "name" : "getTimeLabel",
  "generate" : "jswrap_date_getTimeLabel",
  "return" : ["JsVar", "A short user-facing label for the effective time-zone"],
  "return_object" : "String",
  "typescript" : "getTimeLabel(date: Date) : string"
}
This returns the short (typically 2–6 character) user-facing label for the effective time-zone
(e.g. PDT), if known, or else a synthetisised stand-in like +3:30 or +2DST (during DST).

*/
JsVar *jswrap_date_getTimeLabel(JsVar *date) {
  return time_label(date, true);
}

/*JSON{
  "type" : "method",
  "ifndef" : "ESPR_NO_DAYLIGHT_SAVING",
  "class" : "Date",
  "name" : "getTimezoneID",
  "generate" : "jswrap_date_getTimezoneID",
  "return" : ["JsVar","The canonical ID of the effective time-zone"],
  "return_object" : "String",
  "typescript" : "gettimezoneID(date: Date) : string | undefined"
}
This returns the canonical ID of the effective time-zone, e.g. America/Los_Angeles,
if known.

*/
JsVar *jswrap_date_getTimezoneID(JsVar *date) {
  JsVar *tz = time_zone(date);
  JsVar *id = 0;
  if (jsvIsNumeric(tz)) {
    JsVarFloat o = jsvGetFloat(tz);
    if (o == 0) {
      static const char *gmt = "Etc/GMT";
      id = jsvNewNativeString(gmt, strlen(gmt));
    } else if (o == (int)o) { // India is SOL, but what to do?
      id = jsvVarPrintf("Etc/GMT%c%d", o > 0 ? '+' : '-', abs((int)o));
    }
  } else {
    id = jspGetNamedField(tz, "i", false);
  }
  jsvUnLock(tz);
  return id;
}

static JsVar *get_transition(JsVar *date, bool next) {
  JsVarFloat time = jsvObjectGetFloatChild(date, "ms");
  time -= !next; // Simplify reverese iteration
  JsVar *tz = time_zone(date);
  get_tz_local_time_descriptor(time, tz, false, next ? 0 : &time, next ? &time : 0);
  if (isinf(time)) {
    jsvUnLock(tz);
    return 0;
  }
  return zoned_date_from_milliseconds_and_unlock(time, tz);
}

/*JSON{
  "type" : "method",
  "ifndef" : "ESPR_NO_DAYLIGHT_SAVING",
  "class" : "Date",
  "name" : "getNextTransition",
  "generate" : "jswrap_date_getNextTransition",
  "return" : ["JsVar", "The moment the time is next known to change"],
  "return_object" : "Date",
  "typescript" : "getNextTransition(date: Date) : Date | undefined"
}
This returns the moment when the next time zone transition (e.g. DST start or end)
will occur, if known.

*/
JsVar *jswrap_date_getNextTransition(JsVar *date) {
  return get_transition(date, true);
}

/*JSON{
  "type" : "method",
  "ifndef" : "ESPR_NO_DAYLIGHT_SAVING",
  "class" : "Date",
  "name" : "getPreviousTransition",
  "generate" : "jswrap_date_getPreviousTransition",
  "return" : ["JsVar", "The moment the time was last known to change"],
  "return_object" : "Date",
  "typescript" : "getPreviousTransition(date: Date) : Date | undefined"
}
This returns the moment when the next previous zone transition (e.g. DST start or end)
occured, if known.

*/
JsVar *jswrap_date_getPreviousTransition(JsVar *date) {
  return get_transition(date, false);
}
#endif
