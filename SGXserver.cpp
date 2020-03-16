/*!
 *
 * SGXserver.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 * Some function are released from Intel Corporation.
 * LIENSE: https://github.com/intel/sgx-ra-sample/blob/master/LICENSE
 */


#include "RA_SGXserver.hpp"
#include "SGX_Fileio.hpp"
#include "SGX_Measurement.hpp"
#include "SGX_Print.hpp"
#include <bitset>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>


#define MAX_PATH FILENAME_MAX
#define MAX_JS_FILE_SIZE 10240
#define MAX_JS_FILE_NAME_LENGTH 256

char gbuf[MAX_JS_FILE_NAME_LENGTH];

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

uint32_t ret_status;
MsgIO *msgio;



void set_JS_Fname(const char *ptr)
{
  if(strlen(ptr)+1 > MAX_JS_FILE_NAME_LENGTH){
    fprintf(stderr, "%s", "JS file name length is too long\n");
    exit(1);
  }
  sprintf(gbuf, "%s", ptr);
}


void cp_source_ocall(void *sc, size_t size)
{
  FILE *fp;
  char *fname = gbuf;
  fprintf(stderr,"%s is going to be read.\n", fname);
  if((fp = fopen(fname, "r")) == NULL){
    fprintf(stderr, "JS File open error\n");
    exit(1);
  }
  char c;
  int i=0;
  char *out = (char *)sc;
  while( (c=fgetc(fp))!=EOF){
    out[i++] = c;
    if(i == size){
      printf("The file size exceeds %d bytes.\n", size);
      exit(1);
    }
  }
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


void OCALL_SendMessage(uint8_t* ret_msg, int msg_len)
{
  msgio->send((void *) ret_msg, msg_len - 1);
  sleep(1);  
}


bool CheckUserInfo(MsgIO *msgio, sgx_enclave_id_t eid, unsigned char* userID_clump, unsigned char* passwd_clump)
{
  // Get length of infos from clump.
  uint8_t userID_len_uint8t[4] = {};
  uint8_t passwd_len_uint8t[4] = {};
  memcpy(userID_len_uint8t, userID_clump, 4);
  memcpy(passwd_len_uint8t, passwd_clump, 4);
  
  int userID_len;
  int passwd_len;
  ConvertUint8_tToInt(userID_len_uint8t, userID_len);
  ConvertUint8_tToInt(passwd_len_uint8t, passwd_len);
  // cout << userID_len << endl;
  // cout << passwd_len << endl;
  
  
  // Get userID(dec) and pwhash(dec + SHA256).
  sgx_status_t status;
  uint8_t userID[userID_len] = {};
  uint8_t pwhash[32]         = {}; // pwhash should be 256 bit (32 byte).

  GetUserID_pwhash(eid, &status, userID_clump, userID_len + 32, userID, userID_len, passwd_clump, passwd_len + 32, pwhash);


  // Convert type: uint8_t -> string.
  string userID_str(userID_len, '\0');
  for(int i=0; i<userID_len; i++){
    userID_str[i] = userID[i];
  }

  string pwhash_str = hexstring(pwhash, 32);
  // cout << userID_str << ", " << pwhash_str << endl;


  // Check userID and pwhash on SQlite3.
  chkdb(userID_str, pwhash_str);

  if(cbflg != 0){
    cerr << "Password check is failed..." << endl;
    return false;
  }

  
  return true;
}





int main(int argc, char *argv[])
{
  // ***** ↓↓↓ Remote Attestation ↓↓↓ *****
  
  // Initialization
  // MsgIO *msgio;
  int ra_status;
  config_t config;
  sgx_status_t sgxrv;
  sgx_enclave_id_t eid = 0;
  sgx_ra_context_t ra_ctx = 0xdeadbeef;
  

  ra_status = init_RA(argc, argv, eid, config);
  
  if(ra_status < 0){
    cerr << "Fail to connect with SGX server..." << endl;
    return 1;
  }
  else if(ra_status == 2){
    cout << "exec do_quite()..." << endl;

    close_logfile(fplog);
    return 0;
  }

  
  msgio = prepare_MsgIO(&config);

  if(msgio == NULL){
    cerr << "Fail to prepare MsgIO socket..." << endl;

    delete msgio;
    return 1;
  }  
  
  
  while(msgio->server_loop()){
    msgio = handshake_RA(eid, &config, ra_ctx, msgio);

    if(msgio == NULL){
      delete msgio;
      return 1;
    }


    // ***** ↑↑↑ Remote Attestation ↑↑↑ *****


    // verify userID, password.
    uint8_t *enc_userID = NULL;
    uint8_t *enc_passwd = NULL;
    msgio->read((void **) &enc_userID, NULL);
    msgio->read((void **) &enc_passwd, NULL);


    bool user_check = CheckUserInfo(msgio, eid, enc_userID, enc_passwd);

    string msg;
    if(user_check == true){
      msg = "OK";
    }
    else{
      msg = "NG";
    }

    sgx_status_t status;
    SendMessage(eid, &status, msg.c_str(), msg.length());
    

    
    // Receive enc_jscode and enc_AESkey.
    uint8_t *enc_jscode = NULL;
    uint8_t *enc_AESkey = NULL;
    
    msgio->read((void **) &enc_jscode, NULL);
    msgio->read((void **) &enc_AESkey, NULL);


    // Get data length from clump.
    uint8_t jscode_len_uint8t[4] = {};
    uint8_t AESkey_len_uint8t[4] = {};
    memcpy(jscode_len_uint8t, enc_jscode, 4);
    memcpy(AESkey_len_uint8t, enc_AESkey, 4);
    
    int jscode_len;
    int AESkey_len;
    ConvertUint8_tToInt(jscode_len_uint8t, jscode_len);
    ConvertUint8_tToInt(AESkey_len_uint8t, AESkey_len);


    // interpret JS code.
    string boundary = "STDIO\n\n\n--------------- ↓↓↓ Execute JavaScript Code in Enclave ↓↓↓ ---------------\n";
    SendMessage(eid, &status, boundary.c_str(), boundary.length() + 1);

    sgx_status_t js_status;
    JSinterpreter(eid, &status, enc_jscode, jscode_len + 32, enc_AESkey, AESkey_len + 32);


    if(js_status == SGX_SUCCESS){
      boundary = "STDIO\n\n--------------- ↑↑↑ Execute JavaScript Code in Enclave ↑↑↑ ---------------\n\n";
      SendMessage(eid, &status, boundary.c_str(), boundary.length() + 1);
    

      // Send message to tell client to finish connecting.
      string finish_msg = "FinishConnecting";
      SendMessage(eid, &status, finish_msg.c_str(), finish_msg.length());
    }
    
    enclave_ra_close(eid, &sgxrv, ra_ctx);    
    
    msgio->disconnect();
  }
  
  
  delete msgio;
  close_logfile(fplog);

  return 0;
}
