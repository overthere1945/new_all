target_include_directories(${ZEPHYR_APP_NAME} PUBLIC inc)


target_sources(${ZEPHYR_APP_NAME} PRIVATE 
   src/pb_common.c
   src/pb_decode.c
   src/pb_encode.c
)