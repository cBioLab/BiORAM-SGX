/*!
 *
 * SGX_Math.hpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#ifndef _SGX_Math_hpp
#define SGX_Math_hpp

#pragma once


#include "Enclave_t.h"
#include <sgx_trts.h>
#include <bitset>
#include <cmath>


using namespace std;


void ConvertIntToUint8_t(int from, uint8_t* to);

void ConvertUint8_tToInt(uint8_t* from, int& to);

int UniformDistribution_int(int min, int max);

void swap_double(double* a, double* b);

void swap_int(int* a, int* b);


#endif
