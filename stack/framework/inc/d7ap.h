
/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2018 University of Antwerp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file d7ap.h
 * @addtogroup d7ap
 * @ingroup framework
 * @{
 * @brief D7AP public types (used for modem and/or D7AP stack itself)
 *
 */

#ifndef D7AP_H
#define D7AP_H

#include "types.h"
#include "phy.h"

#define ID_TYPE_NBID_ID_LENGTH 1
#define ID_TYPE_NOID_ID_LENGTH 0
#define ID_TYPE_UID_ID_LENGTH   8
#define ID_TYPE_VID_LENGTH      2

typedef enum {
    ID_TYPE_NBID = 0,
    ID_TYPE_NOID = 1,
    ID_TYPE_UID = 2,
    ID_TYPE_VID = 3
} d7ap_addressee_id_type_t;

typedef struct {
    union {
        uint8_t raw;
        struct {
            uint8_t nls_method : 4;
            d7ap_addressee_id_type_t id_type : 2;
            uint8_t _rfu : 2;
        };
    };
} d7ap_addressee_ctrl_t;

typedef struct {
    d7ap_addressee_ctrl_t ctrl;
    union {
        uint8_t access_class;
        struct {
            uint8_t access_mask : 4;
            uint8_t access_specifier : 4;
        };
    };
    uint8_t id[8]; // TODO assuming 8 byte id for now
} d7ap_addressee_t;

typedef struct {
    union {
        uint8_t raw;
        struct {
            uint8_t _rfu : 4;
            bool ucast : 1;
            bool retry : 1;
            bool missed : 1;
            bool nls : 1;
        };
    };
} d7ap_sp_state_t;


typedef struct {
    channel_id_t channel;
    uint8_t rx_level;
    uint8_t link_budget;
    uint8_t target_rx_level;
    d7ap_sp_state_t status;
    uint8_t fifo_token;
    uint8_t seqnr;
    uint8_t response_to;
    d7ap_addressee_t* addressee;
} d7ap_session_result_t;

typedef enum {
    SESSION_STATE_IDLE = 0x00,
    SESSION_STATE_DORMANT = 0x01,
    SESSION_STATE_PENDING = 0x02,
    SESSION_STATE_ACTIVE = 0x03,
    SESSION_STATE_DONE = 0x04,
} d7ap_session_state_t;

typedef enum  {
    SESSION_RESP_MODE_NO = 0,
    SESSION_RESP_MODE_ALL = 1,
    SESSION_RESP_MODE_ANY = 2,
    SESSION_RESP_MODE_NO_RPT = 4,
    SESSION_RESP_MODE_ON_ERR = 5,
    SESSION_RESP_MODE_PREFERRED = 6,
} d7ap_session_resp_mode_t;

typedef enum {
    SESSION_RETRY_MODE_NO = 0
} d7ap_session_retry_mode_t;

typedef struct {
    union {
        uint8_t raw;
        struct {
            d7ap_session_resp_mode_t qos_resp_mode : 3;
            d7ap_session_retry_mode_t qos_retry_mode: 3;
            bool qos_record : 1;
            bool qos_stop_on_error : 1;
        };
    };
} d7ap_session_qos_t;

typedef struct {
    d7ap_session_qos_t qos;
    uint8_t dormant_timeout;
    d7ap_addressee_t addressee;
} d7ap_master_session_config_t;


#endif // D7AP_H

/** @}*/
