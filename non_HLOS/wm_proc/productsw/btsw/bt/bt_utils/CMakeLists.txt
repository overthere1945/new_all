target_include_directories(${ZEPHYR_APP_NAME} PRIVATE ${BTSW_ROOT}/../../modules/nanopb)

target_include_directories(${ZEPHYR_APP_NAME} PRIVATE inc)

target_sources(${ZEPHYR_APP_NAME} PRIVATE 
   src/lpai_bt_heap.c
   src/bt_utilities.c
   src/lpai_bt_npa.c  
)

