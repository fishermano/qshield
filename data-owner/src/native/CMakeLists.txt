cmake_minimum_required(VERSION 2.8)

project(dataOwner)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH} $ENV{SGX_SDK}/lib")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "$ENV{SGX_SDK}/lib64")
endif()

include_directories(Include)
include_directories(Common)
include_directories("$ENV{SGX_SDK}/include")

set(SOURCES
  KeyGen.cpp
  ServiceProvider.cpp
  ServiceProviderJNI.cpp
  agent_wget.cpp
  base64.c
  crypto.cpp
  ecp.cpp
  iasrequest.cpp
  sp_crypto.cpp)

set(QSOURCES
    QServiceProvider.cpp
    ServiceProvider.cpp
    ServiceProviderJNI.cpp
    QServiceProviderJNI.cpp
    agent_wget.cpp
    base64.c
    crypto.cpp
    ecp.cpp
    iasrequest.cpp
    sp_crypto.cpp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-attributes")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")

add_library(ra_jni SHARED ${QSOURCES})
set_property(TARGET ra_jni PROPERTY POSITION_INDEPENDENT_CODE OFF)
find_library(CRYPTO_LIB crypto)
find_library(SSL_LIB ssl)
find_library(TSTDC_LIB sgx_tstdc)
find_library(TSTDCXX_LIB sgx_tcxx)
find_library(TKEY_EXCHANGE_LIB sgx_tkey_exchange)
find_library(TCRYPTO_LIB sgx_tcrypto)
find_library(TESCHEME_LIB sgx_tescheme)

target_link_libraries(ra_jni "${CRYPTO_LIB}" "${SSL_LIB}" -Wl,--no-whole-archive
  -Wl,--start-group "${TKEY_EXCHANGE_LIB}" "${TCRYPTO_LIB}" "${TESCHEME_LIB}" -Wl,--end-group)

add_executable(keygen ${SOURCES})
target_link_libraries(keygen "${CRYPTO_LIB}" "${SSL_LIB}")

install(TARGETS ra_jni DESTINATION lib)
