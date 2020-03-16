/*!
 *
 * SGX_Measurement.hpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 */


#ifndef _SGX_Measurement_hpp
#define SGX_Measurement_hpp

#pragma once

#include "Enclave_u.h"
#include <iostream>
#include <chrono>


using namespace std;


void OCALL_chrono_start();

double OCALL_chrono_end();

void OCALL_chrono_Total_start();

void OCALL_chrono_Total_end();

void OCALL_chrono_AESenc_start();

void OCALL_chrono_AESenc_end();

void OCALL_chrono_AESdec_start();

void OCALL_chrono_AESdec_end();

void OCALL_chrono_FileIn_start();

void OCALL_chrono_FileIn_end();

void OCALL_chrono_FileOut_start();

void OCALL_chrono_FileOut_end();

void OCALL_chrono_GetPath_start();

void OCALL_chrono_GetPath_end();

void OCALL_chrono_WritePath_start();

void OCALL_chrono_WritePath_end();

void OCALL_chrono_Check_start();

void OCALL_chrono_Check_end();

void OCALL_ShowMeasuredTime();


void OCALL_FS_start();

void OCALL_FS_end();

void OCALL_ML_start();

void OCALL_ML_end();

void OCALL_res_start();

void OCALL_res_end();

void OCALL_ExpResult();



#endif
