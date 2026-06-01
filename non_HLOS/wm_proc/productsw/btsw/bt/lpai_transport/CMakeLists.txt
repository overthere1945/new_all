target_include_directories(${ZEPHYR_APP_NAME} PRIVATE inc)

target_sources(${ZEPHYR_APP_NAME} PRIVATE 
   src/lpai_transport.c
)