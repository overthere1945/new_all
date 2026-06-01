target_include_directories(${ZEPHYR_APP_NAME} PRIVATE inc)

if(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD)
    target_include_directories(${ZEPHYR_APP_NAME} PRIVATE gatt/inc)
endif()

target_include_directories(${ZEPHYR_APP_NAME} PRIVATE audio_tone/inc)
target_include_directories(${ZEPHYR_APP_NAME} PRIVATE audio_tone/src)
if(CONFIG_QC_M55_DISPLAY_ENABLE)
		target_link_libraries(${ZEPHYR_APP_NAME} PRIVATE ${DISPLAY_SS_PUBLIC_INTERFACE_LIB})
endif()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

if(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD) 
    zephyr_nanopb_sources(${ZEPHYR_APP_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/gatt/pb/lpai_bt_gatt.proto
    )
endif()

if(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD)
    target_include_directories(${ZEPHYR_APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gatt/pb)
endif()

target_sources(${ZEPHYR_APP_NAME} PRIVATE 
   src/lpai_bt_lecoc_app.c
   src/lpai_bt_rfcomm_app.c
   src/lpai_bt_le_scan.c
   src/lpai_bt_le_adv.c
   src/lpai_bt_ble_demo_app.c # add hyungchul: Add a demo for sending car_img.h JPEG data via a LECoC socket offloaded from HLOS
)

if(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD)
    target_sources(${ZEPHYR_APP_NAME} PRIVATE 
    gatt/src/lpai_bt_gatt_common.c
    gatt/src/lpai_bt_gatt_client_app.c
    gatt/src/lpai_bt_gatt_server_app.c
    )
endif()

if(CONFIG_PDTSW_AUDIO_SERVICE_ENABLE)
target_sources(${ZEPHYR_APP_NAME} PRIVATE 
    audio_tone/src/lpai_bt_audio_tone.c)
endif()
