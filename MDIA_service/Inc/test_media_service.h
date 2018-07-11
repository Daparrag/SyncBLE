#ifndef __TEST_MEDIA_SERVICE__
#define __TEST_MEDIA_SERVICE__

#include "media_service.h"
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"
#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif


#if !defined (TEST_MEDIA_SERVER) && !defined(TEST_MEDIA_CLIENT)
#error "you should define TEST_MEDIA_SERVER or TEST_MEDIA_CLIENT to use this module"
#endif


void test_media_service(void);

#endif /*__MEDIA_SERVICE_TEST__*/