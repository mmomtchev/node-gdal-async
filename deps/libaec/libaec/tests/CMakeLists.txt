add_library(check_aec STATIC check_aec.c)
target_link_libraries(check_aec PUBLIC aec)

add_executable(check_code_options check_code_options.c)
target_link_libraries(check_code_options PUBLIC check_aec aec)
add_test(NAME check_code_options COMMAND check_code_options)

add_executable(check_buffer_sizes check_buffer_sizes.c)
target_link_libraries(check_buffer_sizes PUBLIC check_aec aec)
add_test(NAME check_buffer_sizes COMMAND check_buffer_sizes)
add_executable(check_seeking check_seeking.c)
target_link_libraries(check_seeking PUBLIC check_aec aec)
add_test(NAME check_seeking COMMAND check_seeking)
add_executable(check_long_fs check_long_fs.c)
target_link_libraries(check_long_fs PUBLIC check_aec aec)
add_test(NAME check_long_fs COMMAND check_long_fs)

add_executable(check_szcomp check_szcomp.c)
target_link_libraries(check_szcomp PUBLIC check_aec sz)
add_test(NAME check_szcomp
  COMMAND check_szcomp ${PROJECT_SOURCE_DIR}/data/121B2TestData/ExtendedParameters/sar32bit.dat)

add_executable(check_rsi_block_access check_rsi_block_access.c)
target_link_libraries(check_rsi_block_access PUBLIC check_aec aec)
add_test(NAME check_rsi_block_access COMMAND check_rsi_block_access)

if(UNIX)
  add_test(
    NAME sampledata.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sampledata.sh
    ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  set(SAMPLE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../data")
  set(SAMPLE_DATA_NAME "121B2TestData")
  set(SAMPLE_DATA_URL "https://cwe.ccsds.org/sls/docs/SLS-DC/BB121B2TestData/121B2TestData.zip")
  add_custom_target(
    update-sampledata
    WORKING_DIRECTORY ${SAMPLE_DATA_DIR}
    COMMAND rm -rf ${SAMPLE_DATA_NAME}
    COMMAND curl ${SAMPLE_DATA_URL} -O
    COMMAND unzip "${SAMPLE_DATA_NAME}.zip"
    COMMAND rm -f "${SAMPLE_DATA_NAME}.zip"
    COMMAND mv -f "${SAMPLE_DATA_NAME}/AllOptions/test_P512n22.dat"
    "${SAMPLE_DATA_NAME}/AllOptions/test_p512n22.dat")
endif(UNIX)
