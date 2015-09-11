/*
 * File: Date.c
 * Author: David Brotz
 */

#include "Date.h"

#include <string.h>

/*
 *FIXME: TO_DATE does no checking to see if _Day is a possible day inside on _Month.
 * There is no 31st of Feburary ever.
 * Overflow is also a possibility by giving _Day a parameter larger than 32.
 */
#define TO_DATE(_Year, _Month, _Day) (DAY(_Day) | (_Month << MONTH_BITSHIFT) | (_Year << YEAR_BITSHIFT))

const char* g_ShortMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

DATE MonthToInt(const char* _Month) {
	int i;

	for(i = 0; i < MONTHS; ++i)
		if(strcmp(_Month, g_ShortMonths[i]) == 0)
			return i;
	return -1;
}

DATE DateAdd(DATE _One, DATE _Two) {
	int _Day = DAY(_One) + DAY(_Two);
	int _Month = MONTH(_One);
	int _Year = YEAR(_One);

	top:
	if((_Month & 1) == 0 || _Month == 6) {
		if(_Day >= 31) {
			_Day = _Day - 31;
			++_Month;
			goto top;
		}
	} else if(_Month == 1) {
		if(_Day >= 28) {
			if(((_Year % 4) == 0 && _Day >= 29)) {
				_Day = _Day - 29;
			} else {
				_Day = _Day - 28;
			}
			++_Month;
			goto top;
		}
	} else if(_Day >= 30) {
		_Day = _Day - 30;
		++_Month;
		goto top;
	}
	while(_Month >= MONTHS) {
		++_Year;
		_Month = _Month - MONTHS;
	}
	return TO_DATE(_Year, _Month, _Day);
}

DATE DateAddInt(DATE _One, int _Two) {
	int _Day = DAY(_One);
	int _Month = MONTH(_One);
	int _Year = YEAR(_One);

	top:
	if((_Month & 1) == 0 || _Month == 6) {
		if((_Day + _Two) >= 31) {
			_Two = (_Two - (31 - _Day)) - 1;
			_Day = 0;
			++_Month;
			goto top;
		}
	} else if(_Month == 1) {
		if((_Day + _Two) >= 28) {
			if(((_Year % 4) == 0 && (_Day + _Two) >= 29)) {
				_Two = (_Two - (29 - _Day)) - 1;

			} else {
				_Two = (_Two - (28 - _Day)) - 1;
			}
			_Day = 0;
			++_Month;
			goto top;
		}
	} else if((_Day + _Two) >= 30) {
		_Two = (_Two - (30 - _Day)) - 1;
		_Day = 0;
		++_Month;
		goto top;
	}
	_Day += _Two;
	while(_Month >= MONTHS) {
		++_Year;
		_Month = _Month - MONTHS;
	}
	return TO_DATE(_Year, _Month, _Day);
}

DATE DaysBetween(int _DateOne, int _DateTwo) {
	if(_DateTwo < _DateOne)
		return 0;
	return DateToDays(_DateTwo) - DateToDays(_DateOne);
}

int DateToDays(int _Date) {
	int _Total = 0;
	int _Years = YEAR(_Date);
	int _Months = MONTH(_Date);
	int _Result = 0;

	_Total = _Years * YEAR_DAYS;
	_Result = _Months / 2;
	_Total += (_Result + 1) * 31;
	_Total += _Result * 30;
	if(_Months >= 1) {
		if(_Years % 4 == 0)
			_Total += 28;
		else
			_Total += 29;
	}
	if(_Months >= 8)
		++_Total;
	return _Total;
}

/*
 * FIXME: This is an approximate instead of an exact way to determine
 * how many years and months are comprised of _Days.
 */
DATE DaysToDate(int _Days) {
	int _Years = 0;
	int _Months = 0;

	while(_Days >= YEAR_DAYS) {
		_Days -= YEAR_DAYS;
		++_Years;
	}

	while(_Days >= MONTH_DAYS) {
		_Days -= MONTH_DAYS;
		++_Months;
	}
	return (_Years << YEAR_BITSHIFT) | (_Months << MONTH_BITSHIFT) | (_Days);
}

int DateCmp(DATE _One, DATE _Two) {
	if(YEAR(_One) < YEAR(_Two))
		return -1;
	else if(YEAR(_One) > YEAR(_Two))
		return 1;

	if(MONTH(_One) < MONTH(_Two))
		return -1;
	else if(MONTH(_One) > MONTH(_Two))
		return 1;

	if(DAY(_One) < DAY(_Two))
		return -1;
	else if(DAY(_One) > DAY(_Two))
		return 1;
	return 0;
}

int IsNewMonth(int _Date) {
	return (DAY(_Date) == 0);
}

void NextDay(int* _Date) {
	int _Day = DAY(*_Date);
	int _Month = MONTH(*_Date);
	int _Year = YEAR(*_Date);

	if((_Month & 1) == 0 || _Month == 7) {
		if(_Day == 31)
			goto new_month;
	} else if(_Month == 1) {
		if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29))
			goto new_month;
	} else if(_Day == 30)
		goto new_month;
	++_Day;
	if(_Month >= 12) {
		++_Year;
		_Month = 0;
	}
	end:
	*_Date = TO_DATE(_Year, _Month, _Day);
	return;
	new_month:
	_Day = 0;
	++_Month;
	goto end;
}
