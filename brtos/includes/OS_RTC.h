/**
* \file OS_RTC.h
* \brief System Time managment struct declarations and functions prototypes.
*
*
**/

/*********************************************************************************************************
*                                               BRTOS
*                                Brazilian Real-Time Operating System
*                            Acronymous of Basic Real-Time Operating System
*
*                              
*                                  Open Source RTOS under MIT License
*
*
*
*                                  OS Time managment functions Header
*
*
*   Author: Gustavo Weber Denardin
*   Revision: 1.0
*   Date:     20/03/2009
*
*********************************************************************************************************/

#ifndef OS_RTC_H
#define OS_RTC_H

#include "OS_types.h"
//#include "Timer_RTC_DS1307.h"




////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////    OS Time Structure                             /////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/** \addtogroup timersGroup
 *  @{
 */

/**
* \struct OSTime
* Real time clock - shows the current hours, minutes and seconds or the uptime info
*/

typedef struct {

	INT8U RTC_Second;         ///< Seconds of the clock
	INT8U RTC_Minute;         ///< Minutes of the clock
	INT8U RTC_Hour;	          ///< Hours of the clock

} OSTime;

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////    OS Date Structure                             /////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/**
* \struct OSDate
* Operating System Date - Shows the current day, month and year ou the uptime info
*/

typedef struct {

	INT8U      RTC_Day;           ///< Day of the date
	INT8U      RTC_Month;        ///< Month of the date
	INT16U 	   RTC_Year;         ///< Year of the date
} OSDate; 

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////    OS Time and Date Structure                    /////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/**
* \struct OSTime_Date
* Operating System Date and time - Shows the current time and date
*/

typedef struct {
	OSTime  time;
	OSDate  date;
} OSTimeDate;

/**
* \struct OSDateTime
* Operating System Date and time - Shows the current date and time 
*/

typedef struct {
	OSDate  date;
	OSTime  time;
} OSDateTime;



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


struct _OSRTC
{
  INT16U Year;
  INT8U  Month;
  INT8U  Day;
  INT8U  Hour;
  INT8U  Min;
  INT8U  Sec;
};


typedef struct _OSRTC OS_RTC;
  




////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
/////      Functions Prototypes                        /////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/***************************************************************************************** 
* \fn void OSUpdateTime(OSTime *Ptr_Hora)
* \brief Update the system time
*  Used to update the system time
* \param Ptr_Hora - pointer to the current system time
* \return None
*****************************************************************************************/ 
  

// Uptime Functions
void OSUpdateTime(void);
void OSUpdateDate(void);
void OSResetTime(void); 
void OSResetDate(void);
void OSUpdateUptime(void);
OSTime OSUptime(void);
OSDate OSUpDate(void);

// Calendar Functions
void CalendarInputSet(void (*input) (OS_RTC *));
INT8U Init_Calendar(void);
void Resync_calendar(void);
void OSUpdateCalendar(void);
void GetCalendar(OS_RTC *rtc);
void SetCalendar(OS_RTC *rtc);
void GetDateTime(OSDateTime *dt);
void GetCalendarTime(OSTime *t);
void GetCalendarDate(OSDate *d);

// return 0 if equal, 1 if rtc1 > rtc2, -1 if rtc1 < rtc2 
INT8S CompareDateTime(OS_RTC const *rtc1, OS_RTC const *rtc2);

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////







////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


#endif

/** @} */
