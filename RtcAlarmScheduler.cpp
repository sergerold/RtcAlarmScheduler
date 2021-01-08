#include "RtcAlarmScheduler.h"

#include <time.h> 

RtcAlarmScheduler::Alarm::Alarm( uint32_t epoch, void (*alarmFunc)(),  bool isOn, bool isRecurring, TIME_UNITS recurUnit, uint32_t recurValue) :
epoch {epoch},
alarmFunc {alarmFunc},
isOn {isOn},
isRecurring {isRecurring},
recurUnit {recurUnit},
recurValue {recurValue}
{
}

RtcAlarmScheduler::Alarm* RtcAlarmScheduler::getNextAlarmFrom(uint32_t epochTime)
{
    Alarm* pNextAlarm = NULL;
    for (Alarm& alarm: mAlarms)
    {
        if ( !alarm.isOn )
        {
            continue;
        }
        if ( pNextAlarm == NULL && alarm.epoch >= epochTime )
        {
            pNextAlarm = &alarm;
            continue;
        }
        if ( pNextAlarm != NULL && alarm.epoch < pNextAlarm->epoch && alarm.epoch >= epochTime )
        {
            pNextAlarm = &alarm;
        }
    }
    return pNextAlarm;
}

void RtcAlarmScheduler::setRtcAlarm(uint32_t alarmEpoch)
{
    pCurrentInstance = this;
    pRtcAlarm->setAlarmEpoch(alarmEpoch);
    pRtcAlarm->enableAlarm(pRtcAlarm->MATCH_YYMMDDHHMMSS);
}

RtcAlarmScheduler::Alarm* RtcAlarmScheduler::findAlarm(uint32_t epoch)
{
    for (Alarm& alarm : mAlarms)
    {
        if( alarm.epoch == epoch && alarm.isOn )
        {
            return &alarm;
        }
    }
    return NULL; // no matching alarm
}

 uint32_t RtcAlarmScheduler::convertDateTimeToEpoch(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second) const
 {
      struct tm dateTime;
      const uint32_t EPOCH_TIME_YEAR_OFF = 100;

      dateTime.tm_isdst = -1, dateTime.tm_yday = 0, dateTime.tm_wday = 0, dateTime.tm_year = year + EPOCH_TIME_YEAR_OFF, dateTime.tm_mon = month - 1, dateTime.tm_mday = day, dateTime.tm_hour = hour, dateTime.tm_min = minute, dateTime.tm_sec = second;
      return mktime(&dateTime);
 }

void RtcAlarmScheduler::activateAlarm()
{
    uint32_t currentRtcAlarmEpoch = convertDateTimeToEpoch( pRtcAlarm->getAlarmYear(), pRtcAlarm->getAlarmMonth(), pRtcAlarm->getAlarmDay(), pRtcAlarm->getAlarmHours(), pRtcAlarm->getAlarmMinutes(), pRtcAlarm->getAlarmSeconds() );
    if (findAlarm( currentRtcAlarmEpoch ) != NULL) // there are at least some alarms to trigger
    {
        // find all alarms with alarmEpoch
        Alarm *pCurrentAlarm = findAlarm(currentRtcAlarmEpoch);
        while (pCurrentAlarm!= NULL)
        {
            pCurrentAlarm->alarmFunc(); // call the alarm func in the alarm

            if ( !pCurrentAlarm->isRecurring ) // non recurring
            {
                 pCurrentAlarm->isOn = false; // turn off the alarm
            }
            else // recurring
            {
                uint32_t newEpoch = currentRtcAlarmEpoch + ( pCurrentAlarm->recurUnit * pCurrentAlarm->recurValue );
                pCurrentAlarm->epoch = newEpoch;
            }
            pCurrentAlarm = findAlarm(currentRtcAlarmEpoch); // find the next alarm with currentRtcAlarmEpoch, otherwise set to Null and exit loop
        }
        // set next alarm
        updateRtcWithNextAlarm( currentRtcAlarmEpoch );
    }

    else
    {
      Serial.println("Something went wrong here");
    }
}

void RtcAlarmScheduler::globalInterrupt()
{
    pCurrentInstance->activateAlarm();
}

  RtcAlarmScheduler::AlarmID RtcAlarmScheduler::addAlarm(uint32_t secondsEpoch, void (*alarmFunc)(), bool recurring, TIME_UNITS recurUnit, uint32_t recurValue)
  {
      for (AlarmID alarmNo = 0; alarmNo < MAX_ALARMS; alarmNo++)
      {
          if ( !mAlarms[alarmNo].isOn)
          {
              mAlarms[alarmNo] =  Alarm (secondsEpoch, alarmFunc, true, recurring, recurUnit, recurValue);
              //update the next scheduleded alarm
              updateRtcWithNextAlarm ( pRtcAlarm->getEpoch() );

              return alarmNo;
          }
      }
      return MAX_ALARMS; // error as no alarms free
  }

  void RtcAlarmScheduler::updateRtcWithNextAlarm(uint32_t epochTime)
  {
      Alarm* pNextAlarm = getNextAlarmFrom( epochTime ); // this is the next alarm from now
      if (pNextAlarm != NULL)
      {
          setRtcAlarm(pNextAlarm->epoch ); // write the next alarm to the rtc
      }
      else
      {
           Serial.println("No further alarms scheduled");
      }
  }

// public

RtcAlarmScheduler::RtcAlarmScheduler(RTCZero *pRtcAlarm) : pRtcAlarm{pRtcAlarm}
{
    enable(); // enable interrupts by default
    setRtcAlarm(0); // reset the rtc alarm
}

RtcAlarmScheduler::AlarmID RtcAlarmScheduler::addSingleAlarm(void (*alarmFunc)(), uint32_t secondsEpoch)
{
    return addAlarm(secondsEpoch, alarmFunc, false, TIME_UNITS::SECONDS, 0);
}

RtcAlarmScheduler::AlarmID RtcAlarmScheduler::addSingleAlarm(void (*alarmFunc)(), AlarmDateTime alarmTime)
{
  return ( addSingleAlarm(alarmFunc, convertDateTimeToEpoch(alarmTime.years_from_2000, alarmTime.month, alarmTime.day, alarmTime.hour, alarmTime.minute, alarmTime.second) ));
}

RtcAlarmScheduler::AlarmID RtcAlarmScheduler::addRecurringAlarm(void (*alarmFunc)(), uint32_t epochInitial, TIME_UNITS recurUnit, uint32_t recurValue)
{
    return addAlarm(epochInitial, alarmFunc, true, recurUnit, recurValue);
}

RtcAlarmScheduler::AlarmID RtcAlarmScheduler::addRecurringAlarm(void (*alarmFunc)(), AlarmDateTime alarmTimeInitial, TIME_UNITS recurUnit, uint32_t recurValue)
{
    uint32_t alarmTimeInitialAsEpoch = convertDateTimeToEpoch(alarmTimeInitial.years_from_2000, alarmTimeInitial.month, alarmTimeInitial.day, alarmTimeInitial.hour, alarmTimeInitial.minute, alarmTimeInitial.second);
    return addRecurringAlarm(alarmFunc, alarmTimeInitialAsEpoch, recurUnit, recurValue);
}

void RtcAlarmScheduler::enable()
{
    pCurrentInstance = this;
    pRtcAlarm->attachInterrupt(globalInterrupt);
}

void RtcAlarmScheduler::disable()
{
    pRtcAlarm->detachInterrupt();
}

bool RtcAlarmScheduler::clearAlarm(AlarmID alarm_id)
{
    if (alarm_id < 0 || alarm_id >= MAX_ALARMS)
    {
        return false;
    }
    mAlarms[alarm_id].isOn = false;
    // update alarms
    updateRtcWithNextAlarm( pRtcAlarm->getEpoch());
    return true;
}

void RtcAlarmScheduler::clearOldAlarms(uint32_t epochTime)
{
    for (Alarm& alarm : mAlarms)
    {
        if ( alarm.epoch < epochTime )
        {
            alarm.isOn = false;
        }
    }
}

uint32_t RtcAlarmScheduler::getNextAlarmEpoch() const
{
  return convertDateTimeToEpoch( pRtcAlarm->getAlarmYear(), pRtcAlarm->getAlarmMonth(), pRtcAlarm->getAlarmDay(), pRtcAlarm->getAlarmHours(), pRtcAlarm->getAlarmMinutes(), pRtcAlarm->getAlarmSeconds() );
}
