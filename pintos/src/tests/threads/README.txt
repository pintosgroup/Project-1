Jarrod Krebs
CIS 520 Project 0
Sept. 1, 2011

The files that I needed to alter were:
ALL FILES ARE IN pintos/src/tests/threads

Make.tests - added alarm-mucho to test names on line 5
Rubric.alarm - added alarm-mucho to list and set priority
tests.c - added {"alarm-mucho", test_alarm_mucho}, to test names array on line 17
tests.h - added extern test_func test_alarm_mucho; to function prototypes on line 12
alarm-wait.c - added alarm-mucho function, very similar to alarm-many

And I also created a file named: alarm-mucho.ck that looked similar to the other alarm tests 
except that the iterations (check_alarm) was different.