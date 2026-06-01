# ------------------------------------------------------------------------------
#  Copyright (c) Qualcomm Technologies, Inc.
#  All rights reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
# ------------------------------------------------------------------------------

add_subdirectory_ifdef(CONFIG_LPAI_BTSW_ENABLE ${BTSW_ROOT}/bt lpai_bt_mgr)