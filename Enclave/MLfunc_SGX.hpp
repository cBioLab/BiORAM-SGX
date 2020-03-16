/*!
 *
 * MLfunc_SGX.hpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#ifndef _MLfunc_SGX_hpp
#define MLfunc_SGX_hpp

#pragma once

#include "Enclave_t.h"
#include "SGX_Math.hpp"
#include <sgx_trts.h>

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace std;


void FisherExactTest(vector< vector<int> >& ContingencyTable, vector<double>& p_value);

void SplitStringInt(vector<int>& result, string str, char del);

void LogisticRegression(vector< vector<double> >& x, vector<int>& y, vector<double>& theta,
                        int iteration, int regularization);

void PCA(vector< vector<double> >& x, vector< vector<double> >& PCA_data, int n_component);




#endif
