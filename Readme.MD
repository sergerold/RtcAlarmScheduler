This library allows you to schedule alarms on the Arduino Zero (and other boards using the SAMD21 chip).

This library relies upon the RTCZero library (https://github.com/arduino-libraries/RTCZero). When constructing the RtcAlarmScheduler object you must pass the address of the RTCZero object which is being used.

You shouldn't create more than one instance of the RtcAlarmScheduler object.
