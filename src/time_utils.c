/*
 * Copyright (C) 2026 volatilespectre
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <math.h>
#include <sys/time.h>
#include <time.h>

// Subtract seconds and nanoseconds seperately, return result as double to
// decimal places (default = 3)
void timespec_subtract(double *result, struct timespec *start,
                       struct timespec *end, int decimal_places) {
  int sec, nsec;
  sec = end->tv_sec - start->tv_sec;
  nsec = end->tv_nsec - start->tv_nsec;

  // If nanoseconds are negative, borrow 1 second from tv_sec
  if (nsec < 0) {
    sec -= 1;
    nsec += 1000000000L; // Add 1 billion nanoseconds (1 second)
  }

  *result = sec + (nsec / 1000000000.0);
  int factor = pow(10, decimal_places);
  *result = round(*result * factor) / factor;
}

// Subtract seconds and microseconds seperately, return result as double to
// decimal places (default = 3)
void timeval_subtract(double *result, struct timeval *start,
                      struct timeval *end, int decimal_places) {
  int sec, usec;
  sec = end->tv_sec - start->tv_sec;
  usec = end->tv_usec - start->tv_usec;

  // If nanoseconds are negative, borrow 1 second from tv_sec
  if (usec < 0) {
    sec -= 1;
    usec += 1000000L; // Add 1 million microseconds (1 second)
  }

  *result = sec + (usec / 1000000.0);
  int factor = pow(10, decimal_places);
  *result = round(*result * factor) / factor;
}

double timeval_seconds(const struct timeval time) {
  return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}
