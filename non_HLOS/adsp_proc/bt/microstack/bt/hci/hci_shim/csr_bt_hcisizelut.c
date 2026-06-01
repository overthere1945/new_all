/*******************************************************************************
   Copyright (c) 2010-2026 Qualcomm Technologies International, Ltd.
   All Rights Reserved.
   Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*******************************************************************************/

#include "csr_bt_hcisizelut.h"


#ifdef CSR_BT_LE_ENABLE
static const uint8_t csrBtHcishimUlpLengths[HCI_MAX_ULP_OCF & HCI_OPCODE_MASK] =
{
    0,                                                        /* 0x0000 -- not used */
    HCI_ULP_SET_EVENT_MASK_PARAM_LEN,                         /* 0x0001 HCI_ULP_SET_EVENT_MASK */
    HCI_ULP_READ_BUFFER_SIZE_PARAM_LEN,                       /* 0x0002 HCI_ULP_READ_BUFFER_SIZE */
    HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_PARAM_LEN,          /* 0x0003 HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES */
    0,                                                        /* 0x0004 HCI_ULP_SET_LOCAL_USED_FEATURES -- not used  */
    HCI_ULP_SET_RANDOM_ADDRESS_PARAM_LEN,                     /* 0x0005 HCI_ULP_SET_RANDOM_ADDRESS */
    HCI_ULP_SET_ADVERTISING_PARAMETERS_PARAM_LEN,             /* 0x0006 HCI_ULP_SET_ADVERTISING_PARAMETERS */
    HCI_ULP_READ_ADVERTISING_CHANNEL_TX_POWER_PARAM_LEN,      /* 0x0007 HCI_ULP_READ_ADVERTISING_CHANNEL_TX_POWER */
    HCI_ULP_SET_ADVERTISING_DATA_PARAM_LEN,                   /* 0x0008 HCI_ULP_SET_ADVERTISING_DATA */
    HCI_ULP_SET_SCAN_RESPONSE_DATA_PARAM_LEN,                 /* 0x0009 HCI_ULP_SET_SCAN_RESPONSE_DATA */ 
    HCI_ULP_SET_ADVERTISE_ENABLE_PARAM_LEN,                   /* 0x000A HCI_ULP_SET_ADVERTISE_ENABLE */
    HCI_ULP_SET_SCAN_PARAMETERS_PARAM_LEN,                    /* 0x000B HCI_ULP_SET_SCAN_PARAMETERS */
    HCI_ULP_SET_SCAN_ENABLE_PARAM_LEN,                        /* 0x000C HCI_ULP_SET_SCAN_ENABLE */
    HCI_ULP_CREATE_CONNECTION_PARAM_LEN,                      /* 0x000D HCI_ULP_CREATE_CONNECTION */ 
    HCI_ULP_CREATE_CONNECTION_CANCEL_PARAM_LEN,               /* 0x000E HCI_ULP_CREATE_CONNECTION_CANCEL */
    HCI_ULP_READ_WHITE_LIST_SIZE_PARAM_LEN,                   /* 0x000F HCI_ULP_READ_WHITE_LIST_SIZE */
    HCI_ULP_CLEAR_WHITE_LIST_PARAM_LEN,                       /* 0x0010 HCI_ULP_CLEAR_WHITE_LIST */
    HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_PARAM_LEN,               /* 0x0011 HCI_ULP_ADD_DEVICE_TO_WHITE_LIST */
    HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_PARAM_LEN,          /* 0x0012 HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LISTLIST */
    HCI_ULP_CONNECTION_UPDATE_PARAM_LEN,                      /* 0x0013 HCI_ULP_CONNECTION_UPDATELIST */
    HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_PARAM_LEN,        /* 0x0014 HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATIONLIST */
    HCI_ULP_READ_CHANNEL_MAP_PARAM_LEN,                       /* 0x0015 HCI_ULP_READ_CHANNEL_MAPLIST */
    HCI_ULP_READ_REMOTE_USED_FEATURES_PARAM_LEN,              /* 0x0016 HCI_ULP_READ_REMOTE_USED_FEATURES */
    HCI_ULP_ENCRYPT_PARAM_LEN,                                /* 0x0017 HCI_ULP_ENCRYPT */
    HCI_ULP_RAND_PARAM_LEN,                                   /* 0x0018 HCI_ULP_RAND */
    HCI_ULP_START_ENCRYPTION_PARAM_LEN,                       /* 0x0019 HCI_ULP_START_ENCRYPTION */
    HCI_ULP_LONG_TERM_KEY_REQUEST_REPLY_PARAM_LEN,          /* 0x001A HCI_ULP_LONG_TERM_KEY_REQUEST_REPLY */
    HCI_ULP_LONG_TERM_KEY_REQUEST_NEGATIVE_REPLY_PARAM_LEN, /* 0x001B HCI_ULP_LONG_TERM_KEY_REQUEST_NEGATIVE_REPLY */
    HCI_ULP_READ_SUPPORTED_STATES_PARAM_LEN,                  /* 0x001C HCI_ULP_READ_SUPPORTED_STATES */
    HCI_ULP_RECEIVER_TEST_PARAM_LEN,                          /* 0x001D HCI_ULP_RECEIVER_TEST */
    HCI_ULP_TRANSMITTER_TEST_PARAM_LEN,                       /* 0x001E HCI_ULP_TRANSMITTER_TEST */
    HCI_ULP_TEST_END_PARAM_LEN,                               /* 0x001F HCI_ULP_TEST_END */
    HCI_ULP_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY_PARAM_LEN, /* 0x0020 HCI_ULP_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY*/
    HCI_ULP_REMOTE_CONNECTION_PARRAMTER_REQUEST_NEGATIVE_REPLY_PARAM_LEN, /* 0x0021 HCI_ULP_REMOTE_CONNECTION_PARAMETER_REQUEST_NEGATIVE_REPLY*/
    HCI_ULP_SET_DATA_LENGTH_PARAM_LEN,                        /* 0x0022 HCI_LE_Set_Data_Length */
    0,                                                        /* 0x0023 HCI_LE_Read_Suggested_Default_Data_Length not supported*/
    HCI_ULP_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH_PARAM_LEN,    /* 0x0024 HCI_LE_Write_Suggested_Default_Data_Length not supported*/
    HCI_ULP_READ_LOCAL_P256_PUBLIC_KEY_PARAM_LEN,             /* 0x0025 HCI_ULP_READ_LOCAL_P256_PUBLIC_KEY*/
    HCI_ULP_GENERATE_DHKEY_PARAM_LEN,                         /* 0x0026 HCI_ULP_GENERATE_DHKEY*/
    HCI_ULP_ADD_DEVICE_TO_RESOLVING_LIST_PARAM_LEN,           /* 0x0027 HCI_ULP_ADD_DEVICE_TO_RESOLVING_LIST */
    HCI_ULP_REMOVE_DEVICE_FROM_RESOLVING_LIST_PARAM_LEN,      /* 0x0028 HCI_ULP_REMOVE_DEVICE_FROM_RESOLVING_LIST */
    HCI_ULP_CLEAR_RESOLVING_LIST_PARAM_LEN,                   /* 0x0029 HCI_ULP_CLEAR_RESOLVING_LIST */
    HCI_ULP_READ_RESOLVING_LIST_SIZE_PARAM_LEN,               /* 0x002A HCI_ULP_READ_RESOLVING_LIST_SIZE */
    HCI_ULP_READ_PEER_RESOLVABLE_ADDRESS_PARAM_LEN,           /* 0x002B HCI_ULP_READ_PEER_RESOLVABLE_ADDRESS */
    HCI_ULP_READ_LOCAL_RESOLVABLE_ADDRESS_PARAM_LEN,          /* 0x002C HCI_ULP_READ_LOCAL_RESOLVABLE_ADDRESS */
    HCI_ULP_SET_ADDRESS_RESOLUTION_ENABLE_PARAM_LEN,          /* 0x002D HCI_ULP_SET_ADDRESS_RESOLUTION_ENABLE */
    HCI_ULP_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT_PARAM_LEN, /* 0x002E HCI_ULP_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT */
    0,                                                        /* 0x002F -- not used */
    HCI_ULP_READ_PHY_PARAM_LEN,                               /* 0x0030  HCI_ULP_READ_PHY */
    HCI_ULP_SET_DEFAULT_PHY_PARAM_LEN,                        /* 0x0031 HCI_ULP_SET_DEFAULT_PHY */
    HCI_ULP_SET_PHY_PARAM_LEN,                                /* 0x0032 HCI_ULP_SET_PHY */
    HCI_ULP_ENHANCED_RECEIVER_TEST_PARAM_LEN,                 /* 0x0033 HCI_ULP_ENHANCED_RECEIVER_TEST */
    HCI_ULP_ENHANCED_TRANSMITTER_TEST_PARAM_LEN,              /* 0x0034 HCI_ULP_ENHANCED_TRANSMITTER_TEST */
    HCI_ULP_EXT_ADV_SET_RANDOM_ADDR_PARAM_LEN,                /* 0x0035 HCI_ULP_EXT_ADV_SET_RANDOM_ADDR */
    HCI_ULP_EXT_ADV_SET_PARAMS_PARAM_LEN,                     /* 0x0036 HCI_ULP_EXT_ADV_SET_PARAMS */
    HCI_ULP_EXT_ADV_SET_DATA_PARAM_LEN,                       /* 0x0037 HCI_ULP_EXT_ADV_SET_DATA -- variable length */
    HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_PARAM_LEN,             /* 0x0038 HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA -- variable length */
    HCI_ULP_EXT_ADV_ENABLE_PARAM_LEN,                         /* 0x0039 HCI_ULP_EXT_ADV_ENABLE */
    HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_PARAM_LEN,          /* 0x003A HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA */
    HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS_PARAM_LEN,           /* 0x003B HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS */
    HCI_ULP_EXT_ADV_REMOVE_ADV_SET_PARAM_LEN,                 /* 0x003C HCI_ULP_EXT_ADV_REMOVE_ADV_SET */
    HCI_ULP_EXT_ADV_CLEAR_ADV_SETS_PARAM_LEN,                 /* 0x003D HCI_ULP_EXT_ADV_CLEAR_ADV_SETS */
    HCI_ULP_PERIODIC_ADV_SET_PARAMS_PARAM_LEN,                /* 0x003E HCI_ULP_PERIODIC_ADV_SET_PARAMS */
    HCI_ULP_PERIODIC_ADV_SET_DATA_PARAM_LEN,                  /* 0x003F HCI_ULP_PERIODIC_ADV_SET_DATA -- variable length */
    HCI_ULP_PERIODIC_ADV_ENABLE_PARAM_LEN,                    /* 0x0040 HCI_ULP_PERIODIC_ADV_ENABLE */
    HCI_ULP_EXT_SCAN_SET_PARAMS_PARAM_LEN,                    /* 0x0041 HCI_ULP_EXT_SCAN_SET_PARAMS */
    HCI_ULP_EXT_SCAN_ENABLE_PARAM_LEN,                        /* 0x0042 HCI_ULP_EXT_SCAN_ENABLE */
    HCI_ULP_EXT_SCAN_CREATE_CONNECTION_PARAM_LEN,             /* 0x0043 HCI_ULP_EXT_SCAN_CREATE_CONNECTION */
    HCI_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_PARAM_LEN,            /* 0x0044 HCI_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN */
    HCI_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_PARAM_LEN,     /* 0x0045 HCI_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL */
    HCI_ULP_PERIODIC_SCAN_SYNC_TERMINATE_PARAM_LEN,           /* 0x0046 HCI_ULP_PERIODIC_SCAN_SYNC_TERMINATE */
    HCI_ULP_PERIODIC_SCAN_ADD_DEVICE_TO_PA_LIST_PARAM_LEN,    /* 0x0047 HCI_ULP_PERIODIC_SCAN_ADD_DEVICE_TO_PA_LIST */
    HCI_ULP_PERIODIC_SCAN_REMOVE_DEVICE_FROM_PA_LIST_PARAM_LEN, /* 0x0048 HCI_ULP_PERIODIC_SCAN_REMOVE_DEVICE_FROM_PA_LIST */
    HCI_ULP_PERIODIC_SCAN_CLEAR_PA_LIST_PARAM_LEN,            /* 0x0049 HCI_ULP_PERIODIC_SCAN_CLEAR_PA_LIST */
    HCI_ULP_PERIODIC_SCAN_READ_PA_LIST_SIZE_PARAM_LEN,        /* 0x004A HCI_ULP_PERIODIC_SCAN_READ_PA_LIST_SIZE */
    0,                                                        /* 0x004B -- not used */
    0,                                                        /* 0x004C -- not used */
    0,                                                        /* 0x004D -- not used */
    HCI_ULP_SET_PRIVACY_MODE_PARAM_LEN,                       /* 0x004E HCI_ULP_SET_PRIVACY_MODE */
    0,                                                        /* 0x004F -- not used */
    0,                                                        /* 0x0050 -- not used */
    0,                                                        /* 0x0051 -- not used */
    0,                                                        /* 0x0052 -- not used */
    0,                                                        /* 0x0053 -- not used */
    0,                                                        /* 0x0054 -- not used */
    0,                                                        /* 0x0055 -- not used */
    0,                                                        /* 0x0056 -- not used */
    0,                                                        /* 0x0057 -- not used */
    0,                                                        /* 0x0058 -- not used */
    HCI_ULP_PERIODIC_SCAN_SYNC_REPORT_PERIODIC_PARAM_LEN,     /* 0x0059 HCI_ULP_PERIODIC_SCAN_SYNC_REPORT_PERIODIC */
    HCI_ULP_PERIODIC_ADV_SYNC_TRANSFER_PARAM_LEN,             /* 0x005A HCI_ULP_PERIODIC_ADV_SYNC_TRANSFER */
    HCI_ULP_PERIODIC_ADV_SET_INFO_TRANSFER_PARAM_LEN,         /* 0x005B HCI_ULP_PERIODIC_ADV_SET_INFO_TRANSFER */
    HCI_ULP_PERIODIC_ADV_SET_SYNC_TRANSFER_PARAMS_PARAM_LEN,  /* 0x005C HCI_ULP_PERIODIC_ADV_SET_SYNC_TRANSFER_PARAMS_PARAM_LEN */
    HCI_ULP_SET_DEFAULT_PA_SYNC_TRANSFER_PARAMS_PARAM_LEN,    /* 0x005D HCI_ULP_SET_DEFAULT_PA_SYNC_TRANSFER_PARAMS */
    0,                                                        /* 0x005E -- not used */
    0,                                                        /* 0x005F -- not used */
    0,                                                        /* 0x0060 -- not used */
    0,                                                        /* 0x0061 -- not used */
    HCI_ULP_SET_CIG_PARAMETERS_PARAM_LEN,                     /* 0x0062 HCI_ULP_SET_CIG_PARAMETERS */
    HCI_ULP_SET_CIG_PARAMETERS_TEST_PARAM_LEN,                /* 0x0063 HCI_ULP_SET_CIG_PARAMETERS_TEST */
    HCI_ULP_CREATE_CIS_PARAM_LEN,                             /* 0x0064 HCI_ULP_CREATE_CIS */
    HCI_ULP_REMOVE_CIG_PARAM_LEN,                             /* 0x0065 HCI_ULP_REMOVE_CIG */
    HCI_ULP_ACCEPT_CIS_REQUEST_PARAM_LEN,                     /* 0x0066 HCI_ULP_ACCEPT_CIS_REQUEST */
    HCI_ULP_REJECT_CIS_REQUEST_PARAM_LEN,                     /* 0x0067 HCI_ULP_REJECT_CIS_REQUEST */
    HCI_ULP_CREATE_BIG_PARAM_LEN,                             /* 0x0068 HCI_ULP_CREATE_BIG */
    HCI_ULP_CREATE_BIG_TEST_PARAM_LEN,                        /* 0x0069 HCI_ULP_CREATE_BIG_TEST */
    HCI_ULP_TERMINATE_BIG_PARAM_LEN,                          /* 0x006A HCI_ULP_TERMINATE_BIG */
    HCI_ULP_BIG_CREATE_SYNC_PARAM_LEN,                        /* 0x006B HCI_ULP_BIG_CREATE_SYNC */
    HCI_ULP_BIG_TERMINATE_SYNC_PARAM_LEN,                     /* 0x006C HCI_ULP_BIG_TERMINATE_SYNC */
    0,                                                        /* 0x006D -- not used */
    HCI_ULP_SETUP_ISO_DATA_PATH_PARAM_LEN,                    /* 0x006E HCI_ULP_SETUP_ISO_DATA_PATH */
    HCI_ULP_REMOVE_ISO_DATA_PATH_PARAM_LEN,                   /* 0x006F HCI_ULP_REMOVE_ISO_DATA_PATH */
    HCI_ULP_ISO_TRANSMIT_TEST_PARAM_LEN,                      /* 0x0070 HCI_ULP_ISO_TRANSMIT_TEST */
    HCI_ULP_ISO_RECEIVE_TEST_PARAM_LEN,                       /* 0x0071 HCI_ULP_ISO_RECEIVE_TEST */
    HCI_ULP_ISO_READ_TEST_COUNTERS_PARAM_LEN,                 /* 0x0072 HCI_ULP_ISO_READ_TEST_COUNTERS */
    0,                                                        /* 0x0073 -- not used */
    HCI_ULP_SET_HOST_FEATURE_PARAM_LEN,                       /* 0x0074 HCI_ULP_SET_HOST_FEATURE */
    HCI_ULP_READ_ISO_LINK_QUALITY_PARAM_LEN,                  /* 0x0075 HCI_ULP_READ_ISO_LINK_QUALITY */
    HCI_ULP_ENHANCED_READ_TRANSMIT_POWER_LEVEL_PARAM_LEN,     /* 0x0076 HCI_ULP_ENHANCED_READ_TRANSMIT_POWER_LEVEL_LEN */
    HCI_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_PARAM_LEN,       /* 0x0077 HCI_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL */
    HCI_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_PARAM_LEN,     /* 0x0078 HCI_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS */
    HCI_ULP_SET_PATH_LOSS_REPORTING_ENABLE_PARAM_LEN,         /* 0x0079 HCI_ULP_SET_PATH_LOSS_REPORTING_ENABLE */
    HCI_ULP_SET_TRANSMIT_POWER_REPORTING_ENABLE_PARAM_LEN,    /* 0x007A HCI_ULP_SET_TRANSMIT_POWER_REPORTING_ENABLE */
    0,                                                        /* 0x007B -- not used */
    HCI_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_PARAM_LEN,       /* 0x007C HCI_ULP_SET_DATA_RELATED_ADDRESS_CHANGES */
    HCI_ULP_SET_DEFAULT_SUBRATE_PARAM_LEN,                    /* 0x007D HCI_ULP_SET_DEFAULT_SUBRATE */
    HCI_ULP_SUBRATE_REQUEST_PARAM_LEN,                        /* 0x007E HCI_ULP_SUBRATE_REQUEST */
    HCI_ULP_EXT_ADV_SET_PARAMS_V2_PARAM_LEN                   /* 0x007F HCI_ULP_EXT_ADV_SET_PARAMS_V2 */
};
#endif

void CsrBtHcishimInsertLengthByOpcode(HCI_UPRIM_T *pv_hci_uprim)
{
    HCI_UPRIM_T *p_prim = (HCI_UPRIM_T *) pv_hci_uprim;
    hci_op_code_t opcode;

    opcode = p_prim->op_code & HCI_OPCODE_MASK;
    switch (p_prim->op_code & HCI_OPCODE_GROUP_MASK)
    {
#ifdef CSR_BT_LE_ENABLE
        case HCI_ULP:
            p_prim->hci_cmd.length = csrBtHcishimUlpLengths[opcode];
            break;
#endif

        default:
            /* This is probably a variable-length command, or
             * something went wrong */
            break;
    }

    /* Some commands have variable length, handle these separately */
    switch (p_prim->op_code)
    {
#ifdef INSTALL_ADVERTISING_EXTENSIONS
        case HCI_ULP_EXT_ADV_SET_DATA:
            {
                p_prim->hci_cmd.length = 4 + p_prim->hci_ulp_set_ea_data.adv_data_len;
            }
            break;
        case HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA:
            {
                p_prim->hci_cmd.length = 4 + p_prim->hci_ulp_set_ea_scan_resp_data.scan_resp_data_len;
            }
            break;
        case HCI_ULP_EXT_ADV_ENABLE:
            {
                p_prim->hci_cmd.length = 2 + (p_prim->hci_ulp_ea_enable.num_of_sets * 4);
            }
            break;
        case HCI_ULP_EXT_SCAN_SET_PARAMS:
            {
                uint8_t bit_pos;
                uint8_t count;

                /* Test how many bits set (e.g. how many scanning phys in message) */
                for (bit_pos = 0, count = 0; bit_pos < 8; bit_pos++)
                {
                    if ((1 << bit_pos) & p_prim->hci_ulp_set_es_params.scanning_phys)
                    {
                        count++;
                    }
                }

                p_prim->hci_cmd.length = 3 + (count * 5);
            }
            break;

#endif /* INSTALL_ADVERTISING_EXTENSIONS */

    }
}
