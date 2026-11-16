/*
 * liftlog_common.h - small helpers shared by baseline.c, optimized.c and
 * gen_log.c: a monotonic millisecond timer and calendar <-> day-number
 * conversion. Everything here is static inline so each program compiles
 * standalone (e.g. `gcc -Wall baseline.c -o baseline`) with no warnings.
 */
#ifndef LIFTLOG_COMMON_H
#define LIFTLOG_COMMON_H

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/* Number of weeks tracked per lifter. The generated log spans one year. */
#define MAX_WEEKS 64

/* The log starts on 2024-01-01; weeks are counted from this day. */
#define LOG_START_YEAR 2024

/* Monotonic wall-clock time in milliseconds.
 * POSIX: clock_gettime(CLOCK_MONOTONIC) as described in the proposal.
 * Windows (MinGW/TDM-GCC): QueryPerformanceCounter, the equivalent clock. */
static inline double now_ms(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
#endif
}

/* Days since 1970-01-01 for a proleptic Gregorian date
 * (Howard Hinnant's days_from_civil algorithm). */
static inline long days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    long yoe = (long)y - era * 400;
    long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

/* Inverse of days_from_civil. */
static inline void civil_from_days(long z, int *y, int *m, int *d)
{
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long yy  = yoe + era * 400;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp  = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yy + (*m <= 2));
}

#endif /* LIFTLOG_COMMON_H */
