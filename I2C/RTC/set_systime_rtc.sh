#!/usr/bin/env bash

# Path to compiled rtc program
PATH=/RedPitaya/I2C/RTC/rtc

# Execute
RTC_TIME=$($PATH --iso8601)

# Set system time 
/usr/bin/date -s "$RTC_TIME"

# Log
echo "System time synhronized from RTC at $RTC_TIME" >> /var/log/sys.log