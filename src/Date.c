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
	for(int i = 0; i < MONTHS; ++i)
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
		if(_Day >= DAYS_ODD) {
			_Day = _Day - DAYS_ODD;
			++_Month;
			goto top;
		}
	} else if(_Month == 1) {
		if(_Day >= DAYS_FEB) {
			if(YearIsLeap(_Year) && _Day >= DAYS_LEAP) {
				_Day = _Day - DAYS_LEAP;
			} else {
				_Day = _Day - DAYS_FEB;
			}
			++_Month;
			goto top;
		}
	} else if(_Day >= DAYS_EVEN) {
		_Day = _Day - DAYS_EVEN;
		++_Month;
		goto top;
	}
	while(_Month >= MONTHS) {
		++_Year;
		_Month = _Month - MONTHS;
	}
	return TO_DATE(_Year, _Month, _Day);
}

DATE DateAddInt(DATE _Date, int _Two) {
	int _Day = DAY(_Date);
	int _Month = MONTH(_Date);
	int _Year = YEAR(_Date);

	top:
	if((_Month & 1) == 0 || _Month == 7) {
		if((_Day + _Two) >= DAYS_EVEN) {
			_Two = (_Two - (DAYS_EVEN - _Day));
			_Day = 0;
			++_Month;
			goto top;
		}
	} else if(_Month == 1) {
		if((_Day + _Two) >= 28) {
			if(YearIsLeap(_Year) ) {
				if((_Day + _Two) >= DAYS_LEAP) {
					_Two = (_Two - (DAYS_LEAP - _Day));
				}

			} else {
				_Two = (_Two - (DAYS_FEB - _Day));
			}
			_Day = 0;
			++_Month;
			goto top;
		}
	} else if((_Day + _Two) >= DAYS_ODD) {
		_Two = (_Two - (DAYS_ODD - _Day));
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

uint32_t DaysBetween(DATE _DateOne, DATE _DateTwo) {
	if(_DateTwo < _DateOne)
		return 0;
	return DateToDays(_DateTwo) - DateToDays(_DateOne);
}

uint32_t DateToDays(DATE _Date) {
	int _Total = 0;
	int _Years = YEAR(_Date);
	int _Months = MONTH(_Date);
	int _Result = 0;

	if(_Months == 0)
		goto end;
	_Total = _Years * YEAR_DAYS;
	_Result = _Months / 2;
	if((_Months & 1) == 1)
		_Total += (_Result + 1) * DAYS_EVEN;
	else
		_Total += _Result * DAYS_EVEN;
	_Total += _Result * DAYS_ODD;
	if(_Months > 1) {
		if(YearIsLeap(_Years))
			_Total -= DAYS_ODD - DAYS_LEAP;
		else
			_Total -= DAYS_ODD - DAYS_FEB;
	}
	if(_Months >= 7)
		_Total -= DAYS_EVEN - DAYS_ODD;
	end:
	return _Total + DAY(_Date);
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
	int16_t _Year = YEAR(_One) - YEAR(_Two);
	int16_t _Month = MONTH(_One) - MONTH(_Two);

	if(_Year != 0)
		return _Year;
	if(_Month != 0)
		return _Month;
	return DAY(_One) - DAY(_Two);
}

int IsNewMonth(int _Date) {
	return (DAY(_Date) == 0);
}

void NextDay(DATE* _Date) {
	int _Day = DAY(*_Date);
	int _Month = MONTH(*_Date);
	int _Year = YEAR(*_Date);

	switch(_Month) {
		case 0:
		case 2:
		case 4:
		case 6:
		case 7:
		case 8:
		case 10:
			if(_Day == 31)
				goto new_month;
			break;
		case 1:
			if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29))
				goto new_month;
		case 11:
			if(_Day == 30) {
				++_Year;
				_Month = 0;
				_Day = 0;
				goto end;
			}
		default:
			if(_Day == 30)
				goto new_month;
	}
	++_Day;
	end:
	*_Date = TO_DATE(_Year, _Month, _Day);
	return;
	new_month:
	_Day = 0;
	++_Month;
	goto end;
/*	if((_Month & 1) == 0 || _Month == 7) {
		if(_Day == 31)
			goto new_month;
	} else if(_Month == 1) {
		if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29))
			goto new_month;
	} else if(_Day == 30)
		goto new_month;
	++_Day;
	end:
	if(_Month >= 12) {
		++_Year;
		_Month = 0;
	}
	*_Date = TO_DATE(_Year, _Month, _Day);
	return;
	new_month:
	_Day = 0;
	++_Month;
	goto end;
	*/
}

int YearIsLeap(int _Year) {
	if((_Year % 4) == 0) {
		if((_Year % 100) == 0) {
			if((_Year % 400) == 0) {
				return 1;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

DATE DateAddMonths(const DATE Date, int Months) {
	int DateMonths = MONTH(Date) + Months;
	int Years = YEAR(Date) + DateMonths / MONTHS;
	int Days = DAY(Date);

	DateMonths = DateMonths % MONTHS;
	return TO_DATE(Years, DateMonths, Days);
}
