# ED_SNTP_time

## Purpose
this library defines a singleton class which handles the connection to a SNTP server to get a clock time referece and use it to convert the ESP32 RTC ticks into a clock time


## Description
refer to the example. the class, to be initialized, requires two (optional) info
- the time zone and daylight saving settings (mapped using the TimeZone enum)
- the preferential SNTP hostname
Both parameters have defaults - the timezone being the Central European Time (CET), the hostname defaults to the first member of a list of SNTP servers stored in the ```NTPSERVER``` array. Notice that the last member of that list is a local SNTP server.

Purpose being having a list of hostnames to query for the time synch and, in case of timeout, switch to the next and finally a secure source from the intranet.

In my setup the local SNTP server is running on a Raspberry Pi.

The goal of this redundancy is providing clock time services for ESP devices which might reboot during internet disconnections and therefore having their central logging performed using a server timestamp, which can be obviously offset with the event time due to transmission issues and delays - which makes integrated systems more difficult to troubleshoot.

The synchronization process, including the retries, is non-blocking - the class will run an internal timer which monitors the SNTP connection and retries switching to the following and so on as needed, using asynchronous callbacks.
In case all the registered server cannot connect with the current timeout, the threshold is increased and the list of attempts retried. until a connection is established - after which the timeou threshold is reset.

This to allow to get data from the most performant server, regardless on the current network issues.
The static functions allow to get
- the Clock time (in the local timezone and daylight setting)
- UTC time in Unix format
- convert the RTC clock ticks of ESP32 into Clock time
- get the current Unix time, which is UTC.
