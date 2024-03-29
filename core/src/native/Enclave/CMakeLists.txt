cmake_minimum_required(VERSION 2.8)

project(QShieldEnclaveTrusted)

set(QSHIELD_AC "1")
set(QSHIELD_TP "0")
message("${QSHIELD_AC}")
message("${QSHIELD_TP}")
configure_file("${CMAKE_SOURCE_DIR}/Enclave/qdebug.h.in" "${CMAKE_CURRENT_BINARY_DIR}/qdebug.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(SOURCES
  Aggregate.cpp
  Crypto.cpp
  Enclave.cpp
  Filter.cpp
  Flatbuffers.cpp
  FlatbuffersReaders.cpp
  FlatbuffersWriters.cpp
  Join.cpp
  Project.cpp
  Sort.cpp
  sgxaes.cpp
  sgxaes_asm.S
  util.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/Enclave_t.c
  ACPolicyApplied.cpp
  ResPrepared.cpp
  QProject.cpp
  QFilter.cpp
  QAggregate.cpp
  QSort.cpp
  QJoin.cpp
  QFlatbuffersReaders.cpp
  QFlatbuffersWriters.cpp
  QCrypto.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)

add_custom_command(
  COMMAND ${SGX_EDGER8R} --trusted ${CMAKE_SOURCE_DIR}/Enclave/Enclave.edl --search-path ${CMAKE_SOURCE_DIR}/Enclave --search-path "$ENV{SGX_SDK}/include"
  DEPENDS ${CMAKE_SOURCE_DIR}/Enclave/Enclave.edl
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Enclave_t.c)

add_custom_command(
  COMMAND ../../../../owner/target/native/keygen ${CMAKE_CURRENT_BINARY_DIR}/key.cpp
  DEPENDS ../../../../owner/target/native/keygen
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)

include_directories(SYSTEM "$ENV{SGX_SDK}/include/libcxx")
include_directories(SYSTEM "$ENV{SGX_SDK}/include/tlibc")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc -fvisibility=hidden -fpie -fstack-protector")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -nostdinc++ -Wall -Wextra -Werror=unused-variable")
set(ENCLAVE_LINK_FLAGS "-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined -Wl,-pie,-eenclave_entry -Wl,--export-dynamic -Wl,--defsym,__ImageBase=0 -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/Enclave.lds")

add_library(enclave_trusted SHARED ${SOURCES})
set_property(TARGET enclave_trusted PROPERTY POSITION_INDEPENDENT_CODE OFF)
set_target_properties(enclave_trusted PROPERTIES LINK_FLAGS ${ENCLAVE_LINK_FLAGS})

find_library(TRTS_LIB sgx_trts)
find_library(TRTS_SIM_LIB sgx_trts_sim)
find_library(TSTDC_LIB sgx_tstdc)
find_library(TSTDCXX_LIB sgx_tcxx)
find_library(TKEY_EXCHANGE_LIB sgx_tkey_exchange)
find_library(TCRYPTO_LIB sgx_tcrypto)
find_library(SERVICE_LIB sgx_tservice)
find_library(SERVICE_SIM_LIB sgx_tservice_sim)
find_library(TESCHEME_LIB sgx_tescheme)
find_library(TPBC_LIB sgx_tpbc)
find_library(TGMP_LIB sgx_tgmp)

if(NOT "$ENV{SGX_MODE}" STREQUAL "HW")
  message(STATUS "Building for simulated SGX")
  set(Trts_Library_Path "${TRTS_SIM_LIB}")
  set(Service_Library_Path "${SERVICE_SIM_LIB}")
else()
  message(STATUS "Building for SGX hardware")
  set(Trts_Library_Path "${TRTS_LIB}")
  set(Service_Library_Path "${SERVICE_LIB}")
endif()

target_link_libraries(enclave_trusted -Wl,--whole-archive "${Trts_Library_Path}" -Wl,--no-whole-archive -Wl,--start-group "${TSTDC_LIB}" "${TSTDCXX_LIB}"
  "${TKEY_EXCHANGE_LIB}" "${TCRYPTO_LIB}" "${Service_Library_Path}" "${TESCHEME_LIB}" "${TPBC_LIB}" "${TGMP_LIB}" -Wl,--end-group)

add_custom_command(
  COMMAND ${SGX_ENCLAVE_SIGNER} sign -key "$ENV{SGX_SDK}/SampleCode/SampleEnclave/Enclave/Enclave_private_test.pem" -enclave $<TARGET_FILE:enclave_trusted> -out libenclave_trusted_signed.so -config ${CMAKE_CURRENT_SOURCE_DIR}/Enclave.config.xml
  DEPENDS enclave_trusted ${CMAKE_CURRENT_SOURCE_DIR}/Enclave.config.xml
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libenclave_trusted_signed.so)
add_custom_target(enclave_trusted_signed_target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libenclave_trusted_signed.so)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libenclave_trusted_signed.so DESTINATION lib)
