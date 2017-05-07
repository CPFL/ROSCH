/*
 * bitops.h:	bitmap operation macros.
 *
 * the operations are based on the Linux's implementation.
 * bit counting starts from zero, meaning that the LSB is 
 * considered to be the 0th bit.
 */
#ifndef __RESCH_BITOPS_H__
#define __RESCH_BITOPS_H__

#if (BITS_PER_LONG != 64) && (BITS_PER_LONG != 32)
#error BITS_PER_LONG not defined
#endif

/**
 * find the first set bit.
 * @b: the pointer to a bitmap to search. 
 * @len: the length of the bitmap.
 */
static inline int resch_ffs(unsigned long *b, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (b[i]) {
			return __ffs(b[i]) + i * BITS_PER_LONG;
		}
	}
	return -1;
}

/**
 * find the first zero bit.
 * @b: the pointer to a bitmap to search. 
 * @len: the length of the bitmap.
 */
static inline int resch_ffz(unsigned long *b, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (~b[i]) {
			return __ffs(~b[i]) + i * BITS_PER_LONG;
		}
	}
	return -1;
}

/**
 * find the last set bit.
 * @b: the pointer to a bitmap to search. 
 * @len: the length of the bitmap.
 */
static inline int resch_fls(unsigned long *b, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--) {
		if (b[i]) {
			return __fls(b[i]) + i * BITS_PER_LONG;
		}
	}
	return -1;
}

/**
 * find the last zero bit.
 * @b: the pointer to a bitmap to search. 
 * @len: the length of the bitmap.
 */
static inline int resch_flz(unsigned long *b, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--) {
		if (~b[i]) {
			return __fls(~b[i]) + i * BITS_PER_LONG;
		}
	}
	return -1;
}

/**
 * find the next set bit, including @offset..
 * @b: the pointer to a bitmap to search. 
 * @offset: the bit number to start searching at.
 * @len: the length of the bitmap.
 */
static inline int resch_fns(unsigned long *b, int offset, int len)
{
	int i;
	unsigned long mask;
	unsigned long tmp;
	unsigned long long_offset;

	if (BITS_TO_LONGS(offset) > len || offset < 0) {
		return -1;
	}

	for (i = 0; i < len; i++) {
		long_offset = i * BITS_PER_LONG;
		if (long_offset + BITS_PER_LONG < offset) {
			continue;
		}
		if (long_offset < offset) {
			mask = (~0UL) << (offset - long_offset);
			tmp = b[i] & mask;
		}
		else {
			tmp = b[i];
		}
		if (tmp) {
			return __ffs(tmp) + long_offset;
		}
	}

	return -1;
}

/**
 * find the next zero bit, including @offset..
 * @b: the pointer to a bitmap to search. 
 * @offset: the bit number to start searching at.
 * @len: the length of the bitmap.
 */
static inline int resch_fnz(unsigned long *b, int offset, int len)
{
	int i;
	unsigned long mask;
	unsigned long tmp;
	unsigned long long_offset;

	if (BITS_TO_LONGS(offset) > len) {
		return -1;
	}

	for (i = 0; i < len; i++) {
		long_offset = i * BITS_PER_LONG;
		if (long_offset + BITS_PER_LONG < offset) {
			continue;
		}
		if (long_offset < offset) {
			mask = (~0UL) << (offset - long_offset);
			tmp = b[i] & mask;
		}
		else {
			tmp = b[i];
		}
		if (~tmp) {
			return __ffs(~tmp) + long_offset;
		}
	}

	return -1;
}

/**
 * find the previous set bit, including @offset.
 * @b: the pointer to a bitmap to search. 
 * @offset: the bit number to start searching at.
 * @len: the length of the bitmap.
 */
static inline int resch_fps(unsigned long *b, int offset, int len)
{
	int i;
	unsigned long mask;
	unsigned long tmp;
	unsigned long long_offset;

	if (BITS_TO_LONGS(offset) > len || offset < 0) {
		return -1;
	}

	for (i = len - 1; i >= 0; i--) {
		long_offset = i * BITS_PER_LONG;
		if (long_offset > offset) {
			continue;
		}
		if (long_offset + BITS_PER_LONG > offset) {
			mask = (~0UL) >> ((BITS_PER_LONG - 1) - (offset - long_offset));
			tmp = b[i] & mask;
		}
		else {
			tmp = b[i];
		}
		if (tmp) {
			return __fls(tmp) + long_offset;
		}
	}

	return -1;
}

/**
 * find the previous zero bit, including @offset..
 * @b: the pointer to a bitmap to search. 
 * @offset: the bit number to start searching at.
 * @len: the length of the bitmap.
 */
static inline int resch_fpz(unsigned long *b, int offset, int len)
{
	int i;
	unsigned long mask;
	unsigned long tmp;
	unsigned long long_offset;

	if (BITS_TO_LONGS(offset) > len || offset < 0) {
		return -1;
	}

	for (i = len - 1; i >= 0; i--) {
		long_offset = i * BITS_PER_LONG;
		if (long_offset > offset) {
			continue;
		}
		if (long_offset + BITS_PER_LONG > offset) {
			mask = (~0UL) >> ((BITS_PER_LONG - 1) - (offset - long_offset));
			tmp = b[i] & mask;
		}
		else {
			tmp = b[i];
		}
		if (~tmp) {
			return __fls(~tmp) + long_offset;
		}
	}

	return -1;
}

#endif
