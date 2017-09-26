/*sync_server_test_header_file*/
#ifndef TEST_CTRL_SYNC_H
#define TEST_CTRL_SYNC_H

#include "ptp_core.h"
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"
#include "ptp_interrupt.h"

#ifdef TEST_CLIENT
#include "media_sync_client.h"
#elif TEST_SERVER	
#include "media_sync_server.h"
#endif



void ctrl_sync_test_server(void);
void ctrl_sync_test_client(void);
#endif /*ptp_test*/