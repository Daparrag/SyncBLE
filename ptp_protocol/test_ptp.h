#ifndef PTP_TEST_H
#define PTP_TEST_H
#include "ble_ptp.h"

uint8_t test_create_header(uint8_t type, ptp_hdr * hdr,uint8_t * buff);
uint8_t test_parse_header(ptp_hdr * hdr,uint8_t * data, uint8_t datalen);
uint8_t compare_headers(ptp_hdr * s1hdr,ptp_hdr * s2hdr);
uint8_t test_header(void);

#endif /*PTP_TEST_H*/