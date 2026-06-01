set(GATT_C_FLAGS "-Wno-error -Wno-tautological-compare -Oz")
set(GATT_CXX_FLAGS "-Wno-error -std=c++17 -Wno-tautological-compare -Oz -fno-exceptions")

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_compile_options(${ZEPHYR_APP_NAME} PRIVATE -std=c++20)

#target_link_libraries(${ZEPHYR_APP_NAME} stdc++)

target_include_directories(${ZEPHYR_APP_NAME} PRIVATE app_delegates ${CMAKE_CURRENT_SOURCE_DIR})

# list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
# include(nanopb)
 
# zephyr_nanopb_sources(${ZEPHYR_APP_NAME}
#     ${CMAKE_CURRENT_LIST_DIR}/gatt_mgr/lpai_bt_gatt.proto
# )
 
# target_include_directories(${ZEPHYR_APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gatt_mgr)

target_sources(${ZEPHYR_APP_NAME} PRIVATE 
	# gatt_mgr/app_mgr_rpc.c
	# gatt_mgr/gatt_mgr_proto.c
	# gatt_mgr/gatt_managers.cpp
	# # gatt_mgr/gatt_mgr_rpc.c
	# gatt_mgr/lpai_bt_app_mgr.c
)
