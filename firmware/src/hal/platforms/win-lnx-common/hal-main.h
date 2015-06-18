/*******************************************************************************
Copyright (C) 2015 OLogN Technologies AG

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/

#ifdef WINLNXCOMMON

#if !defined __HAL_PLATFORM_WINLNXCOMMON_MAIN_H__
#define __HAL_PLATFORM_WINLNXCOMMON_MAIN_H__

#include "hal-time-conversions.h"


// data types
#ifdef _MSC_VER
#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short
#define int16_t short
#define uint32_t unsigned int
#else
#include "stdint.h"
#endif

#ifdef _MSC_VER
#define NOINLINE      __declspec(noinline)
#define INLINE __inline
#define FORCE_INLINE	__forceinline
#else
#define INLINE static inline
#define NOINLINE      __attribute__ ((noinline))
#define	FORCE_INLINE static inline __attribute__((always_inline))
#endif

#endif // __HAL_PLATFORM_WINLNXCOMMON_MAIN_H__

#endif
