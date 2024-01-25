count=0;
pass=0;

tot = x => (x.forEach(x => pass += x[0] == x[1]), count += x.length);
arr = x => {const r = Int32Array(x.length); r.set(x); return r};
rule = (endrel, mon, dom, dow, min, idx) => 0x80000000 
  | mon - (endrel ? mon < 1 : mon < 2) << 0x17
  | dom << 0x12 | dow << 0x0f | min << 0x04 | idx;
state = (offs, dst, start, len) => 0xc0000000
  | offs << 0x11 | dst << 0x10 | start << 0x06 | len;
up = x => new Date(new Date(x) + 1000);
dn = x => new Date(new Date(x) - 1000);

var gmt = [
[ Date.parse("2011-10-20") , 1319068800000.0 ],
[ Date.parse("2011-10-20T14:48:12.345") , 1319122092345.0 ],
[ Date.parse("Aug 9, 1995") , 807926400000.0 ],
[ new Date("Wed, 09 Aug 1995 00:00:00").getTime() , 807926400000.0 ],
[ Date.parse("Thu, 01 Jan 1970 00:00:00 GMT") , 0.0 ],
[ Date.parse("Thu, 01 Jan 1970 00:00:00 GMT-0100") , 3600000.0 ],
[ Date.parse("Mon, 25 Dec 1995 13:30:00 +0430"), 819882000000.0 ],
[ new Date(807926400000.0).toString() , "Wed Aug 9 1995 00:00:00 GMT+0000" ],
[ new Date("Fri, 20 Jun 2014 15:27:22 GMT").toString(), "Fri Jun 20 2014 15:27:22 GMT+0000"],
[ new Date("Fri, 20 Jun 2014 15:27:22 GMT").toISOString(), "2014-06-20T15:27:22.000Z"],
[ new Date("Fri, 20 Jun 2014 17:27:22 GMT+0200").toISOString(), "2014-06-20T15:27:22.000Z"],
[ new Date("2011-10-20").getTimezoneOffset(), 0 ],
[ new Date("2011-10-20").getTimeLabel(), "+0" ],
[ new Date("2011-10-20").getTimezoneLabel(), "+0" ],
[ new Date("2011-10-20").getTimezoneID(), undefined ],
[ new Date("2011-10-20").getIsDST(), false ],
[ new Date("2011-10-20").getNextTransition(), undefined ],
[ new Date("2011-10-20").getPreviousTransition(), undefined ]
];
tot(gmt);

E.setTimeZone(2, true, "SAST", "Africa/Johannesburg");
var sast = [
[ Date.parse("2011-10-20") , 1319061600000.0 ],
[ Date.parse("2011-10-20T14:48:12.345") , 1319114892345.0 ],
[ Date.parse("Aug 9, 1995") , 807919200000.0 ],
[ new Date("Wed, 09 Aug 1995 00:00:00").getTime() , 807919200000.0 ],
[ Date.parse("Thu, 01 Jan 1970 00:00:00 GMT") , 0.0 ],
[ Date.parse("Thu, 01 Jan 1970 00:00:00 GMT-0100") , 3600000.0 ],
[ Date.parse("Mon, 25 Dec 1995 13:30:00 +0430"), 819882000000.0 ],
[ new Date(807926400000.0).toString() , "Wed Aug 9 1995 02:00:00 GMT+0200" ],
[ new Date("Fri, 20 Jun 2014 15:27:22 GMT").toString(), "Fri Jun 20 2014 17:27:22 GMT+0200"],
[ new Date("Fri, 20 Jun 2014 15:27:22 GMT").toISOString(), "2014-06-20T15:27:22.000Z"],
[ new Date("Fri, 20 Jun 2014 17:27:22 GMT+0200").toISOString(), "2014-06-20T15:27:22.000Z"],
[ new Date("2011-10-20").getTimezoneOffset(), -120 ],
[ new Date("2011-10-20").getTimeLabel(), "SAST" ],
[ new Date("2011-10-20").getTimezoneLabel(), "SAST" ],
[ new Date("2011-10-20").getTimezoneID(), "Africa/Johannesburg" ],
[ new Date("2011-10-20").getIsDST(), true ],
[ new Date("2011-10-20").getNextTransition(), undefined ],
[ new Date("2011-10-20").getPreviousTransition(), undefined ]
];
tot(sast);

// Northern hemisphere, +2h winter and +3h summer. Change last Sun Mar @ 3am and last Sun Oct @ 4am.
E.setDST(60,120,4,0,2,0,180,4,0,9,0,240,"CET","CEST","Europe/Berlin");
var cet = [
[ new Date("2011-02-10T14:12:00").toLocalISOString() , "2011-02-10T14:12:00.000+0200" ],
[ new Date("2011-06-11T11:12:00").toLocalISOString() , "2011-06-11T11:12:00.000+0300" ],
[ new Date("2011-11-04T09:25:00").toLocalISOString() , "2011-11-04T09:25:00.000+0200" ],
[ new Date("2011-03-27T00:59:59.9Z").toLocalISOString() , "2011-03-27T02:59:59.900+0200" ],
[ new Date("2011-03-27T01:00:00.0Z").toLocalISOString() , "2011-03-27T04:00:00.000+0300" ],
[ new Date("2011-10-30T00:59:59.9Z").toLocalISOString() , "2011-10-30T03:59:59.900+0300" ],
[ new Date("2011-10-30T01:00:00.0Z").toLocalISOString() , "2011-10-30T03:00:00.000+0200" ],
[ new Date("2011-10-20").getTimezoneOffset(), -180 ],
[ new Date("2011-10-20").getTimeLabel(), "CEST" ],
[ new Date("2011-10-20").getTimezoneLabel(), "CET/CEST" ],
[ new Date("2011-10-20").getTimezoneID(), "Europe/Berlin" ],
[ new Date("2011-10-20").getIsDST(), true ],
[ new Date("2011-10-20").getNextTransition().toString(), "Sun Oct 30 2011 03:00:00 GMT+0200" ],
[ new Date("2011-10-20").getPreviousTransition().toString(), "Sun Mar 27 2011 04:00:00 GMT+0300" ]
];
tot(cet);

// Southern hemisphere. -3h winter and -2h summer. Change 2nd Sun Mar @2am and 2nd Sun Oct @ 2am.
E.setDST(60,-180,1,0,9,0,120,1,0,2,0,120);
var dstSouth = [
[ new Date("2011-02-10T14:12:00").toLocalISOString() , "2011-02-10T14:12:00.000-0200" ],
[ new Date("2011-06-11T11:12:00").toLocalISOString() , "2011-06-11T11:12:00.000-0300" ],
[ new Date("2011-11-04T09:25:00").toLocalISOString() , "2011-11-04T09:25:00.000-0200" ],
[ new Date("2011-03-13T03:59:59.9Z").toLocalISOString() , "2011-03-13T01:59:59.900-0200" ],
[ new Date("2011-03-13T04:00:00.0Z").toLocalISOString() , "2011-03-13T01:00:00.000-0300" ],
[ new Date("2011-10-09T04:59:59.9Z").toLocalISOString() , "2011-10-09T01:59:59.900-0300" ],
[ new Date("2011-10-09T05:00:00.0Z").toLocalISOString() , "2011-10-09T03:00:00.000-0200" ],
[ new Date("2011-10-8").getTimezoneOffset(), 180 ],
[ new Date("2011-10-8").getTimeLabel(), "-3" ],
[ new Date("2011-10-8").getTimezoneLabel(), "-3/-2" ],
[ new Date("2011-10-8").getTimezoneID(), undefined ],
[ new Date("2011-10-8").getIsDST(), false ],
[ new Date("2024-1-4").getTimezoneOffset(), 120 ],
[ new Date("2024-1-4").getTimeLabel(), "-2(DST)" ],
[ new Date("2024-1-4").getTimezoneLabel(), "-3/-2" ],
[ new Date("2024-1-4").getTimezoneID(), undefined ],
[ new Date("2024-1-4").getIsDST(), true ],
[ new Date("2011-10-8").getNextTransition().toString(), "Sun Oct 9 2011 03:00:00 GMT-0200" ],
[ new Date("2011-10-8").getPreviousTransition().toString(), "Sun Mar 13 2011 01:00:00 GMT-0300" ],
[ new Date("2011-10-10").getNextTransition().toString(), "Sun Mar 11 2012 01:00:00 GMT-0300" ],
[ new Date("2011-10-10").getPreviousTransition().toString(), "Sun Oct 9 2011 03:00:00 GMT-0200" ]
];
tot(dstSouth);

E.setTimeZone(-3.5);
var mixed = [
[ new Date("2024-1-4 21:12").toString(), "Thu Jan 4 2024 21:12:00 GMT-0330" ],
[ new Date("2024-1-4 21:12").toUTCString(), "Fri, 5 Jan 2024 00:42:00 GMT" ],
[ new Date("2024-01-05T00:42:00Z").toString(), "Thu Jan 4 2024 21:12:00 GMT-0330" ],
[ new Date("2024-1-4 21:12", -8).toString(), "Thu Jan 4 2024 21:12:00 GMT-0800" ],
[ new Date("2024-1-4 21:12", -8).toUTCString(), "Fri, 5 Jan 2024 05:12:00 GMT" ],
[ new Date("2024-01-05T00:42:00Z", -8).toString(), "Thu Jan 4 2024 16:42:00 GMT-0800" ],
[ new Date("Jan 5, 2024 02:42:00 +0200", -8).toString(), "Thu Jan 4 2024 16:42:00 GMT-0800" ],
[ new Date("Jan 4, 2024 16:42:00 -0800", true).toString(), "Thu Jan 4 2024 16:42:00 GMT-0800" ],
[ new Date("Jan 4, 2024 21:12:00", true).toString(), "Thu Jan 4 2024 21:12:00 GMT-0330" ],
];
tot(mixed);

E.setTimeZone({i:'The Unknown Timezone', l:'[???]'});
var unknown = [
[ new Date("2024-1-4 21:12").toString(), "Thu Jan 4 2024 21:12:00 GMT+0000" ],
[ new Date("2024-1-4 21:12").getTimeLabel(), "[???]" ],
[ new Date("2024-1-4 21:12").getTimezoneLabel(), "[???]" ],
[ new Date("2024-1-4 21:12").getNextTransition(), undefined ],
[ new Date("2024-1-4 21:12").getPreviousTransition(), undefined ],
];
tot(unknown);

// From the end of the first day of February to the start of the last.
E.setDST(90, -14.5 * 60, 0, -1, 1, 0, 1440, 4, -1, 1, 0, 0);
var feb = [
    [ new Date("2019-1-1").getNextTransition().toString(),
        "Sat Feb 2 2019 01:30:00 GMT-1300" ],
    [ new Date("2019-1-1").getNextTransition().getNextTransition().toString(),
        "Wed Feb 27 2019 22:30:00 GMT-1430" ],
    [ new Date("2019-1-1").getNextTransition().getNextTransition()
        .getNextTransition().toString(),
        "Sun Feb 2 2020 01:30:00 GMT-1300" ],
    [ new Date("2019-1-1").getNextTransition().getNextTransition()
        .getNextTransition().getNextTransition().toString(),
        "Fri Feb 28 2020 22:30:00 GMT-1430" ],
];
tot(feb);

// Last Saturday in February to first Monday in November
E.setDST(17, 15 * 60, 4, 6, 1, 0, 120, 0, 1, 10, 0, 120);
var lastSatInFeb = [
    [ new Date("2020-12-31").getPreviousTransition().toString(),
        "Mon Nov 2 2020 01:43:00 GMT+1500" ],
    [ new Date("2020-12-31").getPreviousTransition().getPreviousTransition().toString(),
        "Sat Feb 29 2020 02:17:00 GMT+1517" ],
    [ new Date("2020-12-31").getPreviousTransition().getPreviousTransition()
        .getPreviousTransition().toString(),
        "Mon Nov 4 2019 01:43:00 GMT+1500" ],
    [ new Date("2020-12-31").getPreviousTransition().getPreviousTransition()
        .getPreviousTransition().getPreviousTransition().toString(),
        "Sat Feb 23 2019 02:17:00 GMT+1517" ],
];
tot(lastSatInFeb);

// Recrossing the dateline. Hopefully just a technical edge case! 2nd Sun Mar / 1st Sun Nov
E.setDST(26 * 60, -13 * 60, 1, 0, 2, 0, 120, 0, 0, 10, 0, 120);
var dateline = [
[ new Date("2024-1-4 21:12").toString(), "Thu Jan 4 2024 21:12:00 GMT-1300" ],
[ new Date("2024-1-4 21:12").getNextTransition().toString(),
  "Mon Mar 11 2024 04:00:00 GMT+1300" ],
[ new Date("2024-1-4 21:12").getNextTransition().getNextTransition().toString(),
  "Sat Nov 2 2024 00:00:00 GMT-1300" ],
[ new Date("2024-1-4 21:12").getPreviousTransition().toString(),
  "Sat Nov 4 2023 00:00:00 GMT-1300" ],
[ new Date("2024-1-4 21:12").getPreviousTransition().getPreviousTransition().toString(),
  "Mon Mar 13 2023 04:00:00 GMT+1300" ],
[ dn("2024-3-10 02:00:00").toString(), "Sun Mar 10 2024 01:59:59 GMT-1300" ],
[ up("2024-3-10 02:00:00").toString(), "Mon Mar 11 2024 04:00:01 GMT+1300" ],
[ dn("2024-3-11 04:00:00").toString(), "Sun Mar 10 2024 01:59:59 GMT-1300" ],
[ up("2024-3-11 04:00:00").toString(), "Mon Mar 11 2024 04:00:01 GMT+1300" ],
[ dn("2024-11-3 02:00:00 +1300").toString(), "Sun Nov 3 2024 01:59:59 GMT+1300" ],
[ up("2024-11-3 02:00:00 +1300").toString(), "Sat Nov 2 2024 00:00:01 GMT-1300" ],
];
tot(dateline);

// Stepped double daylight time.
// +1 Last Sunday in February, +2 second last Monday in April,
// +1 second Friday in September, +0 first Saturday in November
E.setTimeZone({ll:'STDSTDDT',
  d:arr([
    rule(true ,  2, 22, 7, 120, 1),
    rule(true,   4, 17, 1, 120, 2),
    rule(false,  9,  8, 5, 120, 1),
    rule(false, 11,  1, 6, 120, 0),
    state(120,  true, 5, 3),
    state( 60,  true, 2, 3),
    state(  0, false, 0, 2)
  ])});
var ddt = [
    [ new Date ("2024-1-1").getTimeLabel(), "ST" ],
    [ new Date ("2024-1-1").getIsDST(), false ],
    [ new Date ("2024-1-1").getNextTransition().toString(), "Sun Feb 25 2024 03:00:00 GMT+0100" ],
    [ new Date ("2024-3-1").getTimeLabel(), "DST" ],
    [ new Date ("2024-3-1").getIsDST(), true ],
    [ new Date ("2024-3-1").getNextTransition().toString(), "Mon Apr 22 2024 03:00:00 GMT+0200" ],
    [ new Date ("2024-6-1").getTimeLabel(), "DDT" ],
    [ new Date ("2024-6-1").getIsDST(), true ],
    [ new Date ("2024-6-1").getNextTransition().toString(), "Fri Sep 13 2024 01:00:00 GMT+0100" ],
    [ new Date ("2024-10-1").getTimeLabel(), "DST" ],
    [ new Date ("2024-10-1").getIsDST(), true ],
    [ new Date ("2024-10-1").getNextTransition().toString(), "Sat Nov 2 2024 01:00:00 GMT+0000" ],
];
tot(ddt);

//Zoned time objects
E.setTimeZone(2);
var zoned = [
[ new Date("2024-10-21 11:10").toString(), "Mon Oct 21 2024 11:10:00 GMT+0200" ],
[ new Date(2024, 9, 21, 11, 11, 0, 0).toString(), "Mon Oct 21 2024 11:11:00 GMT+0200" ],
[ new Date(1729501860000).toString(), "Mon Oct 21 2024 11:11:00 GMT+0200" ],
[ new Date("2024-10-21 11:12 +0600").toString(), "Mon Oct 21 2024 07:12:00 GMT+0200" ],
[ new Date("2024-10-21 11:12 +0600", true).toString(), "Mon Oct 21 2024 11:12:00 GMT+0600" ],
[ new Date("2024-10-21 11:13", 4).toString(), "Mon Oct 21 2024 11:13:00 GMT+0400" ],
[ new Date("2024-10-21 11:14 +0600", 4).toString(), "Mon Oct 21 2024 09:14:00 GMT+0400" ],
[ new Date(2024, 9, 21, 11, 15, 0, 0, 4).toString(), "Mon Oct 21 2024 11:15:00 GMT+0400" ],
[ new Date(1729501860000, 4).toString(), "Mon Oct 21 2024 13:11:00 GMT+0400" ],
[ new Date("2024-10-21 11:16", {ll:'STDSTDDT',
    d:arr([
        rule(true ,  2, 22, 7, 120, 1),
        rule(true,   4, 17, 1, 120, 2),
        rule(false,  9,  8, 5, 120, 1),
        rule(false, 11,  1, 6, 120, 0),
        state(120,  true, 5, 3),
        state( 60,  true, 2, 3),
        state(  0, false, 0, 2)
    ])}).toString(), "Mon Oct 21 2024 11:16:00 GMT+0100" ],
[ new Date(2024, 9, 21, 11, 17, 0, 0, {ll:'STDSTDDT',
    d:arr([
        rule(true ,  2, 22, 7, 120, 1),
        rule(true,   4, 17, 1, 120, 2),
        rule(false,  9,  8, 5, 120, 1),
        rule(false, 11,  1, 6, 120, 0),
        state(120,  true, 5, 3),
        state( 60,  true, 2, 3),
        state(  0, false, 0, 2)
    ])}).toString(), "Mon Oct 21 2024 11:17:00 GMT+0100" ],
[ new Date(1729501860000, {ll:'STDSTDDT',
    d:arr([
        rule(true ,  2, 22, 7, 120, 1),
        rule(true,   4, 17, 1, 120, 2),
        rule(false,  9,  8, 5, 120, 1),
        rule(false, 11,  1, 6, 120, 0),
        state(120,  true, 5, 3),
        state( 60,  true, 2, 3),
        state(  0, false, 0, 2)
    ])}).toString(), "Mon Oct 21 2024 10:11:00 GMT+0100" ],
// Global settings are still intact:
[ new Date("2024-10-21 11:18").toString(), "Mon Oct 21 2024 11:18:00 GMT+0200" ],
[ new Date(2024, 9, 21, 11, 19, 0, 0).toString(), "Mon Oct 21 2024 11:19:00 GMT+0200" ],
[ new Date(1729501860000).toString(), "Mon Oct 21 2024 11:11:00 GMT+0200" ],    
];
tot(zoned);

//Real world data

E.setTimeZone({"i":"America\/Los_Angeles","l":"PT","ll":"PSTPDT","d":arr([
    622849920,629622721,632365440,638009281,642203520,646557121,650590080,654943681,
    659137920,663330241,667524480,671716801,675911040,680103361,684297600,688489921,
    692684160,697037761,701070720,705424321,709618560,713810881,718005120,722197441,
    726391680,730584001,734294400,738970561,742680960,747518401,751067520,755904961,
    759454080,764291521,768001920,772678081,776388480,781064641,784775040,789612481,
    793161600,797999041,801548160,806385601,810096000,814772161,818482560,823158721,
    826869120,831545281,835255680,840093121,843642240,848479681,852028800,856866241,
    860576640,865252801,868963200,873639361,877349760,882187201,885736320,890573761,
    894122880,898960321,902025600,907508161,910412160,915894721,
    -2119989376,-2054715519,
    -868220925,-860290877])});
var california = [
[ new Date("2024-1-4 21:12").toString(), "Thu Jan 4 2024 21:12:00 GMT-0800" ],
[ new Date("2024-1-4 21:12").getTimeLabel(), "PST" ],
[ new Date("2024-1-4 21:12").getTimezoneLabel(), "PT" ],
[ new Date("2024-1-4 21:12").getNextTransition().toString(),
  "Sun Mar 10 2024 03:00:00 GMT-0700" ],
[ new Date("2024-1-4 21:12").getNextTransition().getTimeLabel(), "PDT" ],
[ new Date("2024-1-4 21:12").getNextTransition().getTimezoneLabel(), "PT" ],
[ new Date("2024-1-4 21:12").getNextTransition().getNextTransition().toString(),
  "Sun Nov 3 2024 01:00:00 GMT-0800" ],
[ new Date("2024-1-4 21:12").getPreviousTransition().toString(),
  "Sun Nov 5 2023 01:00:00 GMT-0800" ],
[ new Date("2024-1-4 21:12").getPreviousTransition().getPreviousTransition().toString(),
  "Sun Mar 12 2023 03:00:00 GMT-0700" ],
[ new Date("2020-6-1 21:12").getPreviousTransition().toString(), "Sun Mar 8 2020 03:00:00 GMT-0700" ],
[ new Date("2020-6-1 21:12").getNextTransition().toString(), "Sun Nov 1 2020 01:00:00 GMT-0800" ],
[ dn("2024-3-10 01:00:00").toString(), "Sun Mar 10 2024 00:59:59 GMT-0800" ],
[ new Date("2024-3-10 01:00:00").toString(), "Sun Mar 10 2024 01:00:00 GMT-0800" ],
[ up("2024-3-10 01:00:00").toString(), "Sun Mar 10 2024 01:00:01 GMT-0800" ],
[ dn("2024-3-10 02:00:00").toString(), "Sun Mar 10 2024 01:59:59 GMT-0800" ],
[ new Date("2024-3-10 02:00:00").toString(), "Sun Mar 10 2024 03:00:00 GMT-0700" ],
[ up("2024-3-10 02:00:00").toString(), "Sun Mar 10 2024 03:00:01 GMT-0700" ],
[ dn("2024-3-10 03:00:00").toString(), "Sun Mar 10 2024 01:59:59 GMT-0800" ],
[ new Date("2024-3-10 03:00:00").toString(), "Sun Mar 10 2024 03:00:00 GMT-0700" ],
[ up("2024-3-10 03:00:00").toString(), "Sun Mar 10 2024 03:00:01 GMT-0700" ],
[ dn("2020-3-8 03:00:00").toString(), "Sun Mar 8 2020 01:59:59 GMT-0800" ],
[ up("2020-3-8 03:00:00").toString(), "Sun Mar 8 2020 03:00:01 GMT-0700" ],
[ dn("2024-11-3 00:00:00").toString(), "Sat Nov 2 2024 23:59:59 GMT-0700" ],
[ new Date("2024-11-3 00:00:00").toString(), "Sun Nov 3 2024 00:00:00 GMT-0700" ],
[ up("2024-11-3 00:00:00").toString(), "Sun Nov 3 2024 00:00:01 GMT-0700" ],
[ dn("2024-11-3 01:00:00").toString(), "Sun Nov 3 2024 01:59:59 GMT-0700" ],
[ new Date("2024-11-3 01:00:00").toString(), "Sun Nov 3 2024 01:00:00 GMT-0800" ],
[ up("2024-11-3 01:00:00").toString(), "Sun Nov 3 2024 01:00:01 GMT-0800" ],
[ dn("2024-11-3 02:00:00").toString(), "Sun Nov 3 2024 01:59:59 GMT-0800" ],
[ new Date("2024-11-3 02:00:00").toString(), "Sun Nov 3 2024 02:00:00 GMT-0800" ],
[ up("2024-11-3 02:00:00").toString(), "Sun Nov 3 2024 02:00:01 GMT-0800" ],
[ dn("2020-11-1 01:00:00").toString(), "Sun Nov 1 2020 01:59:59 GMT-0700" ],
[ up("2020-11-1 01:00:00").toString(), "Sun Nov 1 2020 01:00:01 GMT-0800" ],

[ up("2009-3-8 02:00:00").toString(), "Sun Mar 8 2009 03:00:01 GMT-0700" ],
[ dn("2009-3-8 02:00:00").toString(), "Sun Mar 8 2009 01:59:59 GMT-0800" ],
[ up("2008-11-2 01:00:00").toString(), "Sun Nov 2 2008 01:00:01 GMT-0800" ],
[ dn("2008-11-2 01:00:00").toString(), "Sun Nov 2 2008 01:59:59 GMT-0700" ],
[ up("2008-11-2 02:00:00 -0700").toString(), "Sun Nov 2 2008 01:00:01 GMT-0800" ],
[ dn("2008-11-2 02:00:00 -0700").toString(), "Sun Nov 2 2008 01:59:59 GMT-0700" ],
[ up("2008-3-9 02:00:00").toString(), "Sun Mar 9 2008 03:00:01 GMT-0700" ],
[ dn("2008-3-9 02:00:00").toString(), "Sun Mar 9 2008 01:59:59 GMT-0800" ],
[ up("2007-11-4 02:00:00 -0700").toString(), "Sun Nov 4 2007 01:00:01 GMT-0800" ],
[ dn("2007-11-4 02:00:00 -0700").toString(), "Sun Nov 4 2007 01:59:59 GMT-0700" ],
[ up("2007-3-11 02:00:00").toString(), "Sun Mar 11 2007 03:00:01 GMT-0700" ],
[ dn("2007-3-11 02:00:00").toString(), "Sun Mar 11 2007 01:59:59 GMT-0800" ],
[ up("2006-10-29 02:00:00 -0700").toString(), "Sun Oct 29 2006 01:00:01 GMT-0800" ],
[ dn("2006-10-29 02:00:00 -0700").toString(), "Sun Oct 29 2006 01:59:59 GMT-0700" ],
[ up("2006-4-2 02:00:00").toString(), "Sun Apr 2 2006 03:00:01 GMT-0700" ],
[ dn("2006-4-2 02:00:00").toString(), "Sun Apr 2 2006 01:59:59 GMT-0800" ],
[ up("2005-10-30 02:00:00 -0700").toString(), "Sun Oct 30 2005 01:00:01 GMT-0800" ],
[ dn("2005-10-30 02:00:00 -0700").toString(), "Sun Oct 30 2005 01:59:59 GMT-0700" ],
[ up("2005-4-3 02:00:00").toString(), "Sun Apr 3 2005 03:00:01 GMT-0700" ],
[ dn("2005-4-3 02:00:00").toString(), "Sun Apr 3 2005 01:59:59 GMT-0800" ],

// The point where the representation method changes in the database:
[ Date("2009-1-1").getPreviousTransition().toString(), "Sun Nov 2 2008 01:00:00 GMT-0800" ],
[ Date("2009-1-1").getNextTransition().toString(), "Sun Mar 8 2009 03:00:00 GMT-0700" ],
[ Date("2009-1-1").getNextTransition().getPreviousTransition().toString(), "Sun Nov 2 2008 01:00:00 GMT-0800" ],
[ Date("2009-1-1").getPreviousTransition().getNextTransition().toString(), "Sun Mar 8 2009 03:00:00 GMT-0700" ],
[ Date("1974-12-31").toString(), "Tue Dec 31 1974 00:00:00 GMT-0800" ],
];
tot(california);

// Morocco backs out of DST for Ramadan
E.setTimeZone({"i":"Africa\/Casablanca","d":arr([
    570100800,626734081,628322880,642332161,644450880,650741761,654196800,659865601,
    661339200,708595202,723708480,912337921,914456640,920747521,922612800,928465921,
    930722880,936207361,938948160,945240961,947130240,947844481,948789120,953627521,
    955240320,956023681,957820800,961368961,963442560,964248961,966207360,969755521,
    971529600,972336001,974593920,978142081,979754880,980561281,983141760,986528641,
    987818880,988786561,991528320,994915201,996044160,996850561,1004269440,1005075841,
    1012333440,1013301121,1020558720,1021365121,1028622720,1029590401,1036848000,1037654401,
    1045073280,1045879681,1053137280,1054104961,1061362560,1062168961,1069587840,1070394241,
    1077651840,1078619521,1085877120,1086683521,1093941120,1094908801,1102166400,1102972801,
    1110391680,1111198081,1118455680,1119423361,1126680960,1127487361,1134744960,1135712641,
    1142970240,1143937921,1151195520,1152001921,1159259520,1160227201,1167484800,1168291201,
    1175710080,1176516481,1183774080,1184741761,1191999360,1192805761,1200063360,1201031041,
    1208288640,1209256321,1216513920,1217320321,1224577920,1225545601,1232803200,1233609601,
    1241028480,1241834881,1249092480,1250060161,1257317760,1258124161,1265381760,1266349441,
    1273607040,1274574721,1281832320,1282638721,1289896320,1290864001,1298121600,1298928001,
    1306346880,1307153281,1314410880,1315378561,1322636160,1323442561,1330700160,1331667841,
    1338925440,1339893121,1347150720,1347957121,1355214720,1356182401,1363440000,1364246401,
    1371665280,1372471681,1379729280,1380696961,1387954560,1388760961,1396018560,1396986241,
    1404243840,1405211521,1412469120,1413275521,1420533120,1421500801,1428758400,1429564801,
    1436983680,1437790081,1445047680,1446015361,1453272960,1454079361,1461336960,1462304641,
    1469562240,1470529921,1477787520,1478593921,1485851520,1486819201,1494076800,1494883201,
    1502302080,1503108481,1510366080,1511333761,1518591360,1519397761,1526655360,1527623041,
    1534880640,1535687041,1543105920,1543912321,1551169920,1552137601,1559395200,1560201601,
    1567620480,1568426881,1575684480,1576652161,
    -1065877504,-1065811968,-1073741824])});
var morocco = [
[ Date("2010-1-1").getNextTransition().toString(), "Sun May 2 2010 01:00:00 GMT+0100" ],
[ Date("2010-1-1").getNextTransition().getIsDST(), true ],
[ Date("2010-1-1").getNextTransition().getNextTransition().toString(), "Sat Aug 7 2010 23:00:00 GMT+0000" ],
[ Date("2010-1-1").getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2011-1-1").getNextTransition().toString(), "Sun Apr 3 2011 01:00:00 GMT+0100" ],
[ Date("2011-1-1").getNextTransition().getIsDST(), true ],
[ Date("2011-1-1").getNextTransition().getNextTransition().toString(), "Sat Jul 30 2011 23:00:00 GMT+0000" ],
[ Date("2011-1-1").getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2012-1-1").getNextTransition().toString(), "Sun Apr 29 2012 03:00:00 GMT+0100" ],
[ Date("2012-1-1").getNextTransition().getIsDST(), true ],
[ Date("2012-1-1").getNextTransition().getNextTransition().toString(), "Fri Jul 20 2012 02:00:00 GMT+0000" ],
[ Date("2012-1-1").getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2012-1-1").getNextTransition().getNextTransition().getNextTransition().toString(), "Mon Aug 20 2012 03:00:00 GMT+0100" ],
[ Date("2012-1-1").getNextTransition().getNextTransition().getNextTransition().getIsDST(), true ],
[ Date("2012-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().toString(), 
    "Sun Sep 30 2012 02:00:00 GMT+0000" ],
[ Date("2012-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2013-1-1").getNextTransition().toString(), "Sun Apr 28 2013 03:00:00 GMT+0100" ],
[ Date("2013-1-1").getNextTransition().getIsDST(), true ],
[ Date("2013-1-1").getNextTransition().getNextTransition().toString(), "Sun Jul 7 2013 02:00:00 GMT+0000" ],
[ Date("2013-1-1").getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2013-1-1").getNextTransition().getNextTransition().getNextTransition().toString(), "Sat Aug 10 2013 03:00:00 GMT+0100" ],
[ Date("2013-1-1").getNextTransition().getNextTransition().getNextTransition().getIsDST(), true ],
[ Date("2013-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().toString(), "Sun Oct 27 2013 02:00:00 GMT+0000" ],
[ Date("2013-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().getIsDST(), false ],

[ Date("2018-1-1").getNextTransition().toString(), "Sun Mar 25 2018 03:00:00 GMT+0100" ],
[ Date("2018-1-1").getNextTransition().getIsDST(), true ],
[ Date("2018-1-1").getNextTransition().getNextTransition().toString(), "Sun May 13 2018 02:00:00 GMT+0000" ],
[ Date("2018-1-1").getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2018-1-1").getNextTransition().getNextTransition().getNextTransition().toString(), "Sun Jun 17 2018 03:00:00 GMT+0100" ],
[ Date("2018-1-1").getNextTransition().getNextTransition().getNextTransition().getIsDST(), true ],
[ Date("2018-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().toString(), "Sun May 5 2019 02:00:00 GMT+0000" ],
[ Date("2018-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2019-1-1").getNextTransition().toString(), "Sun May 5 2019 02:00:00 GMT+0000" ],
[ Date("2019-1-1").getNextTransition().getIsDST(), false ],
[ Date("2019-1-1").getNextTransition().getNextTransition().toString(), "Sun Jun 9 2019 03:00:00 GMT+0100" ],
[ Date("2019-1-1").getNextTransition().getNextTransition().getIsDST(), true ],
[ Date("2019-1-1").getNextTransition().getNextTransition().getNextTransition().toString(), "Sun Apr 19 2020 02:00:00 GMT+0000" ],
[ Date("2019-1-1").getNextTransition().getNextTransition().getNextTransition().getIsDST(), false ],
[ Date("2019-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().toString(), "Sun May 31 2020 03:00:00 GMT+0100" ],
[ Date("2019-1-1").getNextTransition().getNextTransition().getNextTransition().getNextTransition().getIsDST(), true ],
];
tot(morocco);

// (Part of) Kiribati crossed the dateline
E.setTimeZone({"i":"Pacific\/Kiritimati","d":arr([
    8419680,671096321,799428482,
    -963641344,-883949568,-889192448])});
var kiribati = [
[ Date("2020-1-1").getPreviousTransition().toString(),
    "Sun Jan 1 1995 00:00:00 GMT+1400" ],
[ Date("2020-1-1").getPreviousTransition().getPreviousTransition().toString(),
    "Mon Oct 1 1979 00:40:00 GMT-1000" ],
[ Date("1970-1-1").toString(),
    "Thu Jan 1 1970 00:00:00 GMT-1040" ],
];
tot(kiribati);

// Troll has, or had, a stepped 2 hour DST shift. This may not be current, but it's real:
// #   GMT +1 - From March 1 to the last Sunday in March
// #   GMT +2 - From the last Sunday in March until the last Sunday in October
// #   GMT +1 - From the last Sunday in October until November 7
// #   GMT +0 - From November 7 until March 1
E.setTimeZone({i:"Antarctica/Troll",ll:"UTCCETCEST",d:arr([
    0x818403c0, // 1000 0:001 1:000 01:00 0:000 0011 1100: 0000
    0x81e783c1, // 1000 0:001 1:110 01:11 1:000 0011 1100: 0001
    0x856783c0, // 1000 0:101 0:110 01:11 1:000 0011 1100: 0000
    0x859c03c2, // 1000 0:101 1:001 11:00 0:000 0011 1100: 0010
    0xc0000003, // 1100: 0000 0000 000:0: 0000 0000 00:00 0011
    0xc0f10184, // 1100: 0000 1111 000:1: 0000 0001 10:00 0100
    0xc07900c3  // 1100: 0000 0111 100:1: 0000 0000 11:00 0011
])});
var troll = [
[ Date("2020-1-1").toString(), "Wed Jan 1 2020 00:00:00 GMT+0000" ],
[ Date("2020-1-1").getTimeLabel(), "UTC" ],
[ Date("2020-3-15").toString(), "Sun Mar 15 2020 00:00:00 GMT+0100" ],
[ Date("2020-3-15").getTimeLabel(), "CET" ],
[ Date("2020-7-15").toString(), "Wed Jul 15 2020 00:00:00 GMT+0200" ],
[ Date("2020-7-15").getTimeLabel(), "CEST" ],
[ Date("2020-7-1").getNextTransition().toString(), "Sun Oct 25 2020 00:00:00 GMT+0100"],
[ Date("2020-11-1").toString(), "Sun Nov 1 2020 00:00:00 GMT+0100" ],
[ Date("2020-11-1").getTimeLabel(), "CET" ],
];
tot(troll);

result = pass == count;
