target_include_directories(${ZEPHYR_APP_NAME} PRIVATE inc)

if(CONFIG_LPAI_BTSW_BTTEST_ENABLE)
    target_sources(${ZEPHYR_APP_NAME} PRIVATE 
    src/bt_test.c)
endif()