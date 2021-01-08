#ifndef RTC_ALARM_SCHEDULER
#define RTC_ALARM_SCHEDULER

#include <RTCZero.h>


class RtcAlarmScheduler
{
    // public types
    public:
        enum TIME_UNITS
        {
          SECONDS = 1,
          MINUTES = SECONDS * 60,
          HOURS = MINUTES * 60,
          DAYS = HOURS * 24
        };

        struct AlarmDateTime
        {
          uint32_t years_from_2000, month, day, hour, minute, second;
        };

        using AlarmID = size_t;
        const static size_t MAX_ALARMS = 100;

    private:

      class Alarm
      {
        public:

          uint32_t epoch = 0;
          void (*alarmFunc)() = NULL;
          bool isOn = false;
          bool isRecurring = false;
          TIME_UNITS recurUnit = TIME_UNITS::SECONDS;
          uint32_t recurValue = 0;

          Alarm(){}
          Alarm( uint32_t epoch, void (*alarmFunc)(),  bool isOn, bool isRecurring, TIME_UNITS recurUnit, uint32_t recurValue);
      };

      Alarm mAlarms[MAX_ALARMS];
      RTCZero *pRtcAlarm;
      inline static RtcAlarmScheduler *pCurrentInstance;

      Alarm* getNextAlarmFrom(uint32_t epochTime);
      void setRtcAlarm(uint32_t alarmEpoch);
      Alarm* findAlarm(uint32_t epoch);
      uint32_t convertDateTimeToEpoch(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second) const;
      void activateAlarm();
      void static globalInterrupt();
      AlarmID addAlarm(uint32_t secondsEpoch, void (*alarmFunc)(), bool recurring, TIME_UNITS recurUnit, uint32_t recurValue);
      void updateRtcWithNextAlarm(uint32_t epochTime);

    public:
      RtcAlarmScheduler(RTCZero *pRtc);

      AlarmID addSingleAlarm(void (*alarmFunc)(), uint32_t secondsEpoch);
      AlarmID addSingleAlarm(void (*alarmFunc)(), AlarmDateTime alarmTime);

      AlarmID addRecurringAlarm(void (*alarmFunc)(), uint32_t epochInitial, TIME_UNITS recurUnit, uint32_t recurValue);
      AlarmID addRecurringAlarm(void (*alarmFunc)(), AlarmDateTime alarmTimeInitial, TIME_UNITS recurUnit, uint32_t recurValue);

      void enable();
      void disable();

      bool clearAlarm(AlarmID alarm_id);
      void clearOldAlarms(uint32_t epochTime);

      uint32_t getNextAlarmEpoch() const;
};

#endif
