target_compile_options(${ZEPHYR_APP_NAME} PRIVATE -std=c++20)
target_include_directories(${ZEPHYR_APP_NAME} PRIVATE gatt/inc inc)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
	set(PROTO_DIR     ${CMAKE_CURRENT_LIST_DIR}/gatt/inc)
	set(PROTO_FILE    ${PROTO_DIR}/lpai_bt_gatt.proto)
endif()

set(PROTO_CTXHUB_DIR ${CMAKE_CURRENT_LIST_DIR}/pb)
set(PROTO_CTXHUB_FILE ${PROTO_CTXHUB_DIR}/contexthub_hal.proto)

if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
	if(NOT EXISTS "${PROTO_FILE}")
	message(FATAL_ERROR "Proto file not found: ${PROTO_FILE}")
	else()
	message(STATUS "Proto file exists: ${PROTO_FILE}")
	endif()
endif()

zephyr_library_named(gatt_proto)
zephyr_library_named(ctxhub_proto)  
if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
	zephyr_nanopb_sources(gatt_proto ${PROTO_FILE})
endif()

zephyr_nanopb_sources(ctxhub_proto ${PROTO_CTXHUB_FILE})  

if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
	target_link_libraries(${ZEPHYR_APP_NAME} PRIVATE gatt_proto)
endif()

target_link_libraries(${ZEPHYR_APP_NAME} PRIVATE ctxhub_proto)

message(STATUS "BINARYDIR: ${CMAKE_CURRENT_BINARY_DIR}")
if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
	target_include_directories(gatt_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gatt/inc)
endif()

target_include_directories(ctxhub_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/pb)

#target_include_directories(${ZEPHYR_APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pb)

target_sources(${ZEPHYR_APP_NAME} PRIVATE 
	src/app_mgr_rpc.c
	src/lpai_bt_app_mgr.c
	src/lpai_bt_state_mgr.c
	src/lpai_bt_hal_mgr.c
	src/lpai_bt_app_mgr_client_interface.c
	src/lpai_bt_app_mgr_ctxhub_handler.c
)


if (CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
target_sources(${ZEPHYR_APP_NAME} PRIVATE 
    gatt/src/lpai_bt_gatt_managers.cpp
	gatt/src/lpai_bt_gatt_proto.c
	src/gatt_client_test_app.cpp
	src/ancs_micro_app.cpp
	src/lpai_bt_ancs_profile.cpp)
endif()