#!/bin/bash

git clone --branch mbedtls-2.24.0 --depth 1 https://github.com/Mbed-TLS/mbedtls.git mbedtls1
cd mbedtls1
patch -p1 < ../mbedtls.patch

