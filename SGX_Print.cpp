/*!
 *
 * SGX_Print.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 */

#include "SGX_Print.hpp"


/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if(ret == sgx_errlist[idx].err) {
      if(NULL != sgx_errlist[idx].sug)
	printf("Info: %s\n", sgx_errlist[idx].sug);
      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }
  
  if (idx == ttl)
    printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}


void ocall_print_string(const char *str)
{
  /* Proxy/Bridge will check the length and null-terminate 
   * the input string to prevent buffer overflow. 
   */
  printf("%s", str);
}


void OCALL_print(const char* str)
{
  cout << "[OCALL] " << str << endl;

  return;
}


void OCALL_cerror(const char* str)
{
  cerr << "[OCALL] " << str << endl;

  exit(-1);
}


void OCALL_print_uint8_t(uint8_t* str, size_t len)
{
  cout << "[OCALL] " << str << endl;

  return;
}


void OCALL_print_int(int val)
{
  cout << "[OCALL] " << val << endl;

  return;
}


void OCALL_print_double(double val)
{
  cout << "[OCALL] " << val << endl;

  return;
}
