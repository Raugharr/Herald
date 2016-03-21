/*
 * File: Date.h
 * Author: David Brotz
 */
#ifndef __DATE_H
#define __DATE_H

#define DAY_BITSHIFT (0)
#define MONTH_BITSHIFT (5)
#define MONTH_BITMASK (1 << 5)
#define YEAR_BITSHIFT (9)
#define YEAR_BITMASK (1 << 9)
#define YEAR_DAYS (365)
#define MONTH_DAYS (30)
#define TO_YEARS(_Months) ((_Months) * MONTHS)
#define TO_MONTHS(_Days) ((int)((_Days) / MONTH_DAYS))
#define TO_DAYS(_Months) ((_Months) * MONTH_DAYS)
#define YEAR(_Date) ((_Date) >> YEAR_BITSHIFT)
#define MONTH(_Date) (((_Date) >> 5) & (0xF))
#define MONTHS (12)
#define DAY(_Date) ((_Date) & (31))

#define DAYS_EVEN (31)
#define DAYS_ODD (30)
#define DAYS_FEB (28)
#define DAYS_LEAP (29)

typedef int DATE;

DATE MonthToInt(const char* _Month);
DATE DateAdd(DATE _One, DATE _Two);
DATE DateAddInt(DATE _Date, int _Two);
DATE DaysBetween(int _DateOne, int _DateTwo);

extern const char* g_ShortMonths[];
/*
 * Returns the number of days in a DATE.
 */
int DateToDays(DATE _Date);
/*
 * Returns a DATE that is _Days old.
 */
DATE DaysToDate(int _Days);
int DateCmp(DATE _One, DATE _Two);
int IsNewMonth(int _Date);
void NextDay(int* _Date);
int YearIsLeap(int _Year);

#endif
