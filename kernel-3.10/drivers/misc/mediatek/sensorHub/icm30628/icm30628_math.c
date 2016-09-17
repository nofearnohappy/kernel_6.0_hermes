/*
* Copyright (C) 2014 Invensense, Inc.
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

#include "icm30628_math.h"

unsigned char *inv_int32_to_little8(long x, unsigned char *little8)
{
	little8[3] = (unsigned char)((x >> 24) & 0xff);
	little8[2] = (unsigned char)((x >> 16) & 0xff);
	little8[1] = (unsigned char)((x >> 8) & 0xff);
	little8[0] = (unsigned char)(x & 0xff);

	return little8;
}

long inv_q30_mult(long a, long b)
{
	long long temp;
	long result;
	
	temp = (long long)a * b;
	result = (long)(temp >> 30);
	
	return result;
}

void inv_q_mult(const long *q1, const long *q2, long *qProd)
{
	qProd[0] = inv_q30_mult(q1[0], q2[0]) - inv_q30_mult(q1[1], q2[1]) -
		inv_q30_mult(q1[2], q2[2]) - inv_q30_mult(q1[3], q2[3]);

	qProd[1] = inv_q30_mult(q1[0], q2[1]) + inv_q30_mult(q1[1], q2[0]) +
		inv_q30_mult(q1[2], q2[3]) - inv_q30_mult(q1[3], q2[2]);

	qProd[2] = inv_q30_mult(q1[0], q2[2]) - inv_q30_mult(q1[1], q2[3]) +
		inv_q30_mult(q1[2], q2[0]) + inv_q30_mult(q1[3], q2[1]);

	qProd[3] = inv_q30_mult(q1[0], q2[3]) + inv_q30_mult(q1[1], q2[2]) -
		inv_q30_mult(q1[2], q2[1]) + inv_q30_mult(q1[3], q2[0]);
}

int get_highest_bit_position(unsigned long *value)
{
	int position;
	position = 0;
	if (*value == 0) return 0;

	if ((*value & 0xFFFF0000) == 0) {
		position += 16;
		*value=*value<<16;
	}
	if ((*value & 0xFF000000) == 0) {
		position += 8;
		*value=*value<<8;
	}
	if ((*value & 0xF0000000) == 0) {
		position += 4;
		*value=*value<<4;
	}
	if ((*value & 0xC0000000) == 0) {
		position += 2;
		*value=*value<<2;
	}

	if ((*value & 0x80000000)) { 
		position -= 1;
		*value=*value>>1;
	}
	
	return position;
}

int test_limits_and_scale(long *x0, int *pow)
{
	long lowerlimit, upperlimit, oneiterlothr, oneiterhithr, zeroiterlothr, zeroiterhithr;

	lowerlimit = 744261118L;
	upperlimit = 1488522236L;
	oneiterlothr = 966367642L;
	oneiterhithr = 1181116006L;
	zeroiterlothr=1063004406L;
	zeroiterhithr=1084479242L;

	if (*x0 > upperlimit) {
		*x0 = *x0>>1;
		*pow=-1;
	} else if (*x0 < lowerlimit) {
		*pow=get_highest_bit_position((unsigned long*)x0);
		if (*x0 >= upperlimit) {
			*x0 = *x0>>1;
			*pow=*pow-1;
		}
		else if (*x0 < lowerlimit) {
			*x0 = *x0<<1;
			*pow=*pow+1;
		}
	} else {
		*pow = 0;
	}

	if ( *x0<oneiterlothr || *x0>oneiterhithr )
		return 3;
	if ( *x0<zeroiterlothr || *x0>zeroiterhithr )
		return 2; 

	return 1;
}

long inv_fast_sqrt(long x0)
{
	long sqrt2, oneoversqrt2, one_pt5;
	long xx, cc;
	int pow2, sq2scale, nr_iters;

	if (x0 <= 0L) {
		return 0L;
	}

	sqrt2 =1518500250L;
	oneoversqrt2=759250125L;
	one_pt5=1610612736L;

	nr_iters = test_limits_and_scale(&x0, &pow2);

	sq2scale = 0;
	if (pow2 > 0) 
		sq2scale=pow2%2;
	pow2 = pow2-sq2scale;

	cc = x0 - (1L<<30);
	xx = x0 - (inv_q30_mult(x0, cc)>>1);
	if ( nr_iters>=2 ) {
		cc = inv_q30_mult( cc, inv_q30_mult(cc, (cc>>1) - one_pt5) ) >> 1;
		xx = xx - (inv_q30_mult(xx, cc)>>1);
		if ( nr_iters==3 ) {
			cc = inv_q30_mult( cc, inv_q30_mult(cc, (cc>>1) - one_pt5) ) >> 1;
			xx = xx - (inv_q30_mult(xx, cc)>>1);
		}
	}
	if (sq2scale)
		xx = inv_q30_mult(xx,oneoversqrt2);
	if (pow2>0)
		xx = (xx >> (pow2>>1)); 
	else if (pow2 == -1)
		xx = inv_q30_mult(xx,sqrt2);
	return xx;
}


int inv_compute_scalar_part(const long * inQuat, long* outQuat)
{
	long scalarPart = 0;

	scalarPart = inv_fast_sqrt((1L<<30) - inv_q30_mult(inQuat[0], inQuat[0])
		- inv_q30_mult(inQuat[1], inQuat[1])
		- inv_q30_mult(inQuat[2], inQuat[2]) );
	outQuat[0] = scalarPart;
	outQuat[1] = inQuat[0];
	outQuat[2] = inQuat[1];
	outQuat[3] = inQuat[2];

	return 0;
}


