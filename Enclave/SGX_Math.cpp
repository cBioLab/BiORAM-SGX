/*!
 *
 * SGX_Math.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#include "SGX_Math.hpp"


/*
 * convert type: int -> uint8_t*.
 * < input >
 * [int]   from: variable to convert.
 * [uint8_t] to: converted variable (should be prepared 4 bit).
 */

void ConvertIntToUint8_t(int from, uint8_t* to)
{
  bitset<32> bit_num(from);
  int ret = 0;
  
  
  for(size_t i=0; i<bit_num.size(); i++){
    // OCALL_print_int(bit_num[i]);
    
    ret += bit_num[i] * pow(2, i % 8);

    // 1[byte] ごとにuint8_t に変換する．
    if(i % 8 == 7){
      to[(int)i / 8] = (char)ret;
      // OCALL_print_int(ret);

      ret = 0;
    }
  }


  return;
}


/*
 * convert type: uint8_t* -> int.
 * [uint8_t] from: variable to convert (should be prepared 4 bit).
 * [int]       to: converted variable.
 */
void ConvertUint8_tToInt(uint8_t* from, int& to)
{
  bool comp_flag = false;
  to = 0;


  for(int i=0; i<sizeof(int); i++){
    bitset<8> bit_sep((int)from[i]);


    if(i == 3){
      for(int j=0; j<7; j++)
	to += bit_sep[j] * pow(2, 8*i + j);


      if(bit_sep[7] == 1)
	comp_flag = true;
    }
    else{
      for(int j=0; j<8; j++)
	to += bit_sep[j] * pow(2, 8*i + j);
    }
  }

  
  if(comp_flag){
    to -= INT_MAX;
    to -= 1;
  }
  

  return;
}


/*
 * Random Generation Function.
 * ref: https://software.intel.com/en-us/forums/intel-software-guard-extensions-intel-sgx/topic/671569
 */
int UniformDistribution_int(int min, int max)
{
  if(min == max){
    return min;
  }
  else if(min > max){
    swap_int(&min, &max);
  }
 

  // Get random number.
  uint32_t val;
  sgx_read_rand((unsigned char *)&val, 4);
  

  // Convert range: min <= ret_val <= max.
  int range = max - min + 1;
  int ret_val = abs((int32_t)val) % range + min;
  // OCALL_print_int(ret_val);
  
  return ret_val;
}


void swap_double(double* a, double* b){
  double t = *a;
  *a = *b;
  *b = t;
}


void swap_int(int* a, int* b){
  int t = *a;
  *a = *b;
  *b = t;
}
