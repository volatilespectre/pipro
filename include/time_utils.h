#ifndef TIME_UTILS_H_
#define TIME_UTILS_H_

void timespec_subtract(double *result, struct timespec *start,
                       struct timespec *end, int decimal_places);

void timeval_subtract(double *result, struct timeval *start,
                      struct timeval *end, int decimal_places);

#endif