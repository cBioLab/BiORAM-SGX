/*!
 *
 * client.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */

#ifndef _SGX_Sort_hpp
#define SGX_Sort_hpp

#pragma once


#include "Enclave_t.h"
#include <sgx_trts.h>

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>

#include "SGX_Math.hpp"

using namespace std;


// ref: http://kivantium.hateblo.jp/entry/2015/11/03/212211
void QuickSort_LRresult(double *first, double *last, int *sub_first, int *sub_last);


#endif
