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

#if !defined __HAL_PLATFORM_WIRING_TIME_CONVERTIONS_H__
#define __HAL_PLATFORM_WIRING_TIME_CONVERTIONS_H__

// present implementation assumes that resolution of h/w timer is 1 ms
// macros below must be reimplemented if this is not the case
#define HAL_TIME_MILLISECONDS16_TO_TIMEVAL( mslow, timeval ) {}
#define HAL_TIME_MILLISECONDS32_TO_TIMEVAL( mslow, mshigh, timeval ) {}

#endif