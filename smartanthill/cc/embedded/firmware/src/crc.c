/**
 * \file crc.c
 * Functions and types for CRC checks.
 *
 * Generated on Sun Feb  9 18:28:00 2014,
 * by pycrc v0.8.1, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 16
 *    Poly         = 0x8005
 *    XorIn        = 0x0000
 *    ReflectIn    = True
 *    XorOut       = 0x0000
 *    ReflectOut   = True
 *    Algorithm    = table-driven
 *****************************************************************************/
#include "crc.h"     /* include the header file generated with pycrc */
/* #include <stdlib.h>*/
/* #include <stdint.h>*/

/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const crc_t crc_table[16] = {
    0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
    0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
};

/**
 * Reflect all bits of a \a data word of \a data_len bytes.
 *
 * \param data         The data word to be reflected.
 * \param data_len     The width of \a data expressed in number of bits.
 * \return             The reflected data.
 *****************************************************************************/
/* crc_t crc_reflect(crc_t data, size_t data_len)*/
/* {*/
/*     unsigned int i;*/
/*     crc_t ret;*/

/*     ret = data & 0x01;*/
/*     for (i = 1; i < data_len; i++) {*/
/*         data >>= 1;*/
/*         ret = (ret << 1) | (data & 0x01);*/
/*     }*/
/*     return ret;*/
/* }*/


/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
crc_t crc_update(crc_t crc, const unsigned char *data, size_t data_len)
{
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = crc ^ (*data >> (0 * 4));
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4);
        tbl_idx = crc ^ (*data >> (1 * 4));
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4);

        data++;
    }
    return crc & 0xffff;
}

