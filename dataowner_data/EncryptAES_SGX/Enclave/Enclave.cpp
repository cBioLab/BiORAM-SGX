/*!
 *
 * Enclave.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 * Some function are released from Intel Corporation.
 * LIENSE: https://github.com/intel/linux-sgx/blob/master/License.txt
 */


#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;


/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...)
{
  char buf[BUFSIZ] = { '\0' };
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

// =========================================================================


// 文字列をdel 毎に分割する関数．
void SplitString(vector<string>& result, string str, char del){
  int first   = 0;
  int last    = str.find_first_of(del);
  
  while(first < str.size()){
    string split_str(str, first, last - first);
    
    result.push_back(split_str);

    
    first = last + 1;
    last = str.find_first_of(del, first);

    
    if(last == string::npos)
      last = str.size();
  }
  

  return;
}


void swap_int(int* a, int* b){
  int t = *a;
  *a = *b;
  *b = t;
}


// 乱数生成する関数．
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


// SGX 環境で利用できる AES 秘密鍵を生成する関数．
void CreateAESSecretKey(sgx_key_128bit_t& AES_KEY){  
  for(int i=0; i<16; i++)
    AES_KEY[i] = (uint8_t)UniformDistribution_int(0, 255);

    
  return;
}




void ECALL_VCFEncryption(const char* dirpath, const char* filepath, const char* AES_filename, int FILE_SIZE){

  // Initialization.
  sgx_status_t status;
  int NONCE_SIZE = 12;
  
  sgx_key_128bit_t AES_SK;
  sgx_aes_gcm_128bit_tag_t AES_TAG;

  uint8_t* vcf_data = new uint8_t[FILE_SIZE];
  uint8_t* enc_data = new uint8_t[FILE_SIZE];
  uint8_t* nonce    = new uint8_t[NONCE_SIZE];
  
  vector<string> filepath_vec;
  string dir_path(dirpath);
  // string SK_filename    = dir_path + AES_filename;
  string SK_filename    = dir_path + "AES_SK.key";
  string half_filename  = "";
  string nonce_filename = "";
  string MAC_filename   = "";
  string cout = "";
  
  
  
  // 暗号化したいファイル名を取得する．
  SplitString(filepath_vec, filepath, ',');
  cout = "filesize: " + to_string(filepath_vec.size());
  OCALL_print(cout.c_str());
  // for(size_t i=0; i<filepath_vec.size(); i++)  OCALL_print(filepath_vec[i].c_str());


  // AES Secret key を生成する．
  cout = "Create AES Secretkey...";
  OCALL_print(cout.c_str());
  CreateAESSecretKey(AES_SK);


  // AES 秘密鍵を保存する．
  status = OCALL_SaveFile(SK_filename.c_str(), SK_filename.length() + 1, AES_SK, 16);

  
  for(size_t i=0; i<filepath_vec.size(); i++){
    cout = to_string(i + 1) + "th AES Encryption...";
    // OCALL_print(cout.c_str());

    
    // vcf ファイルをロードする．ファイルサイズは既知(= FILE_SIZE) とする．
    status = OCALL_LoadFile(filepath_vec[i].c_str(), filepath_vec[i].length() + 1, vcf_data, FILE_SIZE);
    // OCALL_print_uint8_t(vcf_data, FILE_SIZE);


    // シードを生成する．
    for(int j=0; j<NONCE_SIZE; j++)
      nonce[j] = (uint8_t)UniformDistribution_int(0, 255);


    // vcf データを暗号化する．
    status = sgx_rijndael128GCM_encrypt(&AES_SK, vcf_data, FILE_SIZE, enc_data, nonce, NONCE_SIZE, NULL, 0, &AES_TAG);
    

    // vcf ファイル名から.vcf を取った文字列を取得する．
    half_filename = filepath_vec[i].substr(0, filepath_vec[i].find_last_of("."));
    // OCALL_print(half_filename.c_str());


    // nonce, MAC のファイル名を決定する．
    nonce_filename = half_filename + ".nonce";
    MAC_filename   = half_filename + ".tag";


    // ファイルを保存する．vcf ファイルは暗号化データで上書きする．
    status = OCALL_SaveFile(nonce_filename.c_str(),  nonce_filename.length() + 1,  nonce,    NONCE_SIZE);
    status = OCALL_SaveFile(MAC_filename.c_str(),    MAC_filename.length() + 1,    AES_TAG,  16);
    status = OCALL_SaveFile(filepath_vec[i].c_str(), filepath_vec[i].length() + 1, enc_data, FILE_SIZE);
  }
  

  // メモリ開放．
  delete[] vcf_data;
  delete[] enc_data;
  delete[] nonce;

  vector<string>().swap(filepath_vec);
  string().swap(dir_path);
  string().swap(SK_filename);
  string().swap(half_filename);
  string().swap(nonce_filename);
  string().swap(MAC_filename);
  string().swap(cout);
  
  return;
}
