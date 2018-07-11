/*sync_server_test_header_file*/
#ifndef CTRL_TEST_H
#define CTRL_TEST_H

#include "ptp_core.h"
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"
#include "ptp_interrupt.h"

#if !defined(TEST_CTRL_SERVER) || !defined(TEST_CTRL_CLIENT)
#error "you should define TEST_CTRL_SERVER or TEST_CTRL_CLIENT to use this module"
#endif

#include "ctrl_core.h"




void ctrl_sync_test_server(void);
void ctrl_sync_test_client(void);
//void Ctrl_Sync_client_process(void);
#endif /*CTRL_TEST_H*/