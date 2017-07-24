#ifndef __CRC_16_H__
#define __CRC_16_H__

uint16_t * compute_crc_table();
uint16_t crc_16(const uint16_t table[], const uint8_t msg[], size_t len);

#endif

