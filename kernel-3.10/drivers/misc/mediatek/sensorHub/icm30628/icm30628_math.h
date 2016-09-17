/*
 * Copyright (C) 2012 Invensense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef ICM30628_MATH_H
#define ICM30628_MATH_H

#ifndef ABS
#define ABS(x) (((x)>=0)?(x):-(x))
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif

unsigned char *inv_int32_to_little8(long x, unsigned char *little8);
long inv_q30_mult(long a, long b);
void inv_q_mult(const long *q1, const long *q2, long *qProd);
int get_highest_bit_position(unsigned long *value);
int test_limits_and_scale(long *x0, int *pow);
long inv_fast_sqrt(long x0);
int inv_compute_scalar_part(const long * inQuat, long* outQuat);

#endif /* ICM30628_MATH_H */

