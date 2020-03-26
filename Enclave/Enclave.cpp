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
 * LIENSE: https://github.com/intel/sgx-ra-sample/blob/master/LICENSE
 */

#ifndef _WIN32
#include "../config.h"
#endif
#include "Enclave_t.h"
#include <string.h>
#include <sgx_utils.h>
#include <sgx_tae_service.h>
#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>


// ***** ↓↓↓ EDIT HERE ↓↓↓ *****

// #define sgx_spinlock_t volatile uint32_t

#include "ObliviousRAM_SGX.hpp"
#include "MLfunc_SGX.hpp"
#include "SGX_Math.hpp"
#include "SGX_Sort.hpp"

#include "sgx_trts.h"
#include "Enclave.h"
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include <sstream>

// should be same for APP.cpp
#define MAX_JS_FILE_SIZE 10240
#define MAX_JS_FILE_NAME_LENGTH 256


char *mystrdup(const char* input)
{
  char *out = new char[strlen(input)+1];
  for(int i=0; i<strlen(input)+1; i++){
    out[i] = input[i];
  }
  //  strcpy(out, input);
  return out;
}
#define strdup mystrdup

/*
class myostringstream
{
public:
  string str(){ return "NOT_VALID";}

  myostringstream& operator<< (const void *test1){return *this;}
  myostringstream& operator<< (const string& test2){return *this;}
  myostringstream& operator<< (const char *test2){return *this;}
  myostringstream& operator<< (const int test2){return *this;}
};
#define ostringstream myostringstream
*/
class myostringstream
{
  string str_value;
  
public:
  string str(){
    return this->str_value;
  }
 
  myostringstream& operator<< (const void *text){
    const char* s = (const char*)text;
    this->str_value.append(s);
  }
  myostringstream& operator<< (const string& s){
    this->str_value.append(s);
  }
  myostringstream& operator<< (const char *s){
    this->str_value.append(s);
  }
  myostringstream& operator<< (char c){
    this->str_value.push_back(c);
  }
  myostringstream& operator<< (const int num){
    this->str_value.append(to_string(num));
  }
};
#define ostringstream myostringstream


/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...)
{
  char buf[256] = { '\0' };
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
  return (int)strnlen(buf, BUFSIZ - 1) + 1;
}


//#ifdef __GNUC__
//#define vsprintf_s vsnprintf
#define sprintf_s snprintf
#define _strdup strdup
//#endif

#include "TinyJS.hpp"
#include "TinyJS_MathFunctions.hpp"
#include "TinyJS_Functions.hpp"

// ***** ↑↑↑ EDIT HERE ↑↑↑ *****

static const sgx_ec256_public_t def_service_public_key = {
  {
    0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
    0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
    0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
    0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
  },
  {
    0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
    0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
    0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
    0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
  }
  
};

#define PSE_RETRIES	5	/* Arbitrary. Not too long, not too short. */

/*----------------------------------------------------------------------
 * WARNING
 *----------------------------------------------------------------------
 *
 * End developers should not normally be calling these functions
 * directly when doing remote attestation:
 *
 *    sgx_get_ps_sec_prop()
 *    sgx_get_quote()
 *    sgx_get_quote_size()
 *    sgx_get_report()
 *    sgx_init_quote()
 *
 * These functions short-circuits the RA process in order
 * to generate an enclave quote directly!
 *
 * The high-level functions provided for remote attestation take
 * care of the low-level details of quote generation for you:
 *
 *   sgx_ra_init()
 *   sgx_ra_get_msg1
 *   sgx_ra_proc_msg2
 *
 *----------------------------------------------------------------------
 */

/*
 * This doesn't really need to be a C++ source file, but a bug in 
 * 2.1.3 and earlier implementations of the SGX SDK left a stray
 * C++ symbol in libsgx_tkey_exchange.so so it won't link without
 * a C++ compiler. Just making the source C++ was the easiest way
 * to deal with that.
 */

sgx_status_t get_report(sgx_report_t *report, sgx_target_info_t *target_info)
{
#ifdef SGX_HW_SIM
  return sgx_create_report(NULL, NULL, report);
#else
  return sgx_create_report(target_info, NULL, report);
#endif
}


size_t get_pse_manifest_size ()
{
  return sizeof(sgx_ps_sec_prop_desc_t);
}


sgx_status_t get_pse_manifest(char *buf, size_t sz)
{
  sgx_ps_sec_prop_desc_t ps_sec_prop_desc;
  sgx_status_t status= SGX_ERROR_SERVICE_UNAVAILABLE;
  int retries= PSE_RETRIES;
  
  do {
    status= sgx_create_pse_session();
    if ( status != SGX_SUCCESS ) return status;
  } while (status == SGX_ERROR_BUSY && retries--);
  if ( status != SGX_SUCCESS ) return status;
  
  status= sgx_get_ps_sec_prop(&ps_sec_prop_desc);
  if ( status != SGX_SUCCESS ) return status;
  
  memcpy(buf, &ps_sec_prop_desc, sizeof(ps_sec_prop_desc));
  
  sgx_close_pse_session();
  
  return status;
}


sgx_status_t enclave_ra_init(sgx_ec256_public_t key, int b_pse,
			     sgx_ra_context_t *ctx, sgx_status_t *pse_status)
{
  sgx_status_t ra_status;
  
  /*
   * If we want platform services, we must create a PSE session 
   * before calling sgx_ra_init()
   */
  
  if ( b_pse ) {
    int retries= PSE_RETRIES;
    do {
      *pse_status= sgx_create_pse_session();
      if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
    } while (*pse_status == SGX_ERROR_BUSY && retries--);
    if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
  }
  
  ra_status= sgx_ra_init(&key, b_pse, ctx);
  
  if ( b_pse ) {
    int retries= PSE_RETRIES;
    do {
      *pse_status= sgx_close_pse_session();
      if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
    } while (*pse_status == SGX_ERROR_BUSY && retries--);
    if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
  }
  
  return ra_status;
}


sgx_status_t enclave_ra_init_def(int b_pse, sgx_ra_context_t *ctx,
				 sgx_status_t *pse_status)
{
	return enclave_ra_init(def_service_public_key, b_pse, ctx, pse_status);
}


/*
 * Return a SHA256 hash of the requested key. KEYS SHOULD NEVER BE
 * SENT OUTSIDE THE ENCLAVE IN PLAIN TEXT. This function let's us
 * get proof of possession of the key without exposing it to untrusted
 * memory.
 */
sgx_status_t enclave_ra_get_key_hash(sgx_status_t *get_keys_ret,
	sgx_ra_context_t ctx, sgx_ra_key_type_t type, sgx_sha256_hash_t *hash)
{
	sgx_status_t sha_ret;
	sgx_ra_key_128_t k;

	// First get the requested key which is one of:
	//  * SGX_RA_KEY_MK 
	//  * SGX_RA_KEY_SK
	// per sgx_ra_get_keys().

	*get_keys_ret= sgx_ra_get_keys(ctx, type, &k);
	if ( *get_keys_ret != SGX_SUCCESS ) return *get_keys_ret;

	/* Now generate a SHA hash */

	sha_ret= sgx_sha256_msg((const uint8_t *) &k, sizeof(k), 
		(sgx_sha256_hash_t *) hash); // Sigh.

	/* Let's be thorough */

	memset(k, 0, sizeof(k));

	return sha_ret;
}


//KS add
void print_hexstring(unsigned char* test, int size){

  const char _hextable[]= "0123456789abcdef";
  for(int i=0; i<size; i++){
    printf("%c",_hextable[test[i]>>4]);
    printf("%c",_hextable[test[i]&0xf]);
  }
  printf("\n");
}


sgx_status_t enclave_get_user_key(unsigned char *enc_user_key, size_t enc_user_key_len,
				  unsigned char *iv, size_t iv_len, sgx_aes_gcm_128bit_tag_t *tag)
{
  sgx_status_t ctx;
  sgx_ra_key_128_t k;
  
  sgx_status_t get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &k);
  if ( get_keys_ret != SGX_SUCCESS ) return get_keys_ret;
  
  unsigned char *user_key = (unsigned char*)malloc(16);
  sgx_status_t ret;
  
  //decrypt user's key that was used for encrypting source & data.
  ret = sgx_rijndael128GCM_decrypt(&k, enc_user_key, enc_user_key_len, user_key, iv, 12, NULL, 0, tag);
  
  print_hexstring(enc_user_key,16);
  print_hexstring(iv,12);
  print_hexstring((unsigned char*)tag,16);
  print_hexstring(user_key,16);
  
  //for security reason
  memset(user_key, 0x00, 16);
  free(user_key);
  
  return ret;
}
//KS add end


sgx_status_t enclave_ra_close(sgx_ra_context_t ctx)
{
  sgx_status_t ret;
  ret = sgx_ra_close(ctx);
  return ret;
}





// ***** ↓↓↓ EDIT HERE ↓↓↓ *****


void GetDirectoryInfo(string& in_topdir, string& out_topdir)
{
  string filename_str = "./BiORAM_settings";
  size_t filename_len = filename_str.length();

  uint8_t filename[filename_len] = {};
  for(size_t i=0; i<filename_len; i++){
    filename[i] = filename_str[i];
  }

  string().swap(filename_str);


  int file_len;
  OCALL_GetFileLength(&file_len, filename, filename_len);

  uint8_t *dir_info_uint8t = new uint8_t[file_len]();
  OCALL_LoadFile(filename, filename_len, dir_info_uint8t, file_len);

  string dir_info(file_len, '\0');
  for(int i=0; i<file_len; i++){
    dir_info[i] = dir_info_uint8t[i];
  }

  delete(dir_info_uint8t);
  // OCALL_print(dir_info.c_str());


  
  while(dir_info.length() > 0){
    string line = dir_info.substr(0, dir_info.find("\n"));
    dir_info = dir_info.substr(dir_info.find("\n") + 1);


    // Erase blanks.
    line.erase(remove(line.begin(), line.end(),' '), line.end());

    // If line starts '#', use line as comment.
    if(line[0] == '#'){
      continue;
    }
    
    
    string factor = line.substr(0, line.find("="));
    string dir = line.substr(line.find("=") + 1);


    if(factor == "ENC_VCF_TOPDIR"){
      in_topdir = dir;
    }
    else if(factor == "ORAMTREE_TOPDIR"){
      out_topdir = dir;
    } 
  }

  
  string().swap(dir_info);  
  
  return;
}


void ORAM_init(CScriptVar *c, void* key)
{
  // input from javascript.
  int Z_blocks  = scGetInt("Z_blocks");
  string in_dirname  = c->getParameter("in_dirname")->getString();
  string out_dirname = c->getParameter("out_dirname")->getString();


  uint8_t* user_aes_key = (uint8_t*)key;
  // print_hexstring(user_aes_key, 16);

  string in_topdir, out_topdir;
  GetDirectoryInfo(in_topdir, out_topdir);

  
  ORAM_initialization(Z_blocks, in_topdir, in_dirname,
		      out_topdir, out_dirname, user_aes_key);

  
  // for security reason
  memset(user_aes_key, 0x00, 16);

  OCALL_DeleteFilepaths();
  string().swap(in_dirname);
  string().swap(out_dirname);
  string().swap(in_topdir);
  string().swap(out_topdir);
}


// For checking.
void ORAM_search(CScriptVar *c, void *)
{
  // input from javascript.
  string dirname   = c->getParameter("dirname")->getString();
  string chrID_str = c->getParameter("chrID")->getString();
  string nation    = c->getParameter("nation")->getString();
  int position     = scGetInt("position");

  
  int chrID;
  if(chrID_str == "X")
    chrID = 88;
  else if(chrID_str == "Y")
    chrID = 89;
  else
    chrID = stoi(chrID_str.c_str());

  // OCALL_print_int(chrID);

  
  ORAM_FileSearch(dirname, chrID, nation, position);

  
  string().swap(dirname);
  string().swap(chrID_str);
  string().swap(nation);
}


void ML_Fisher(CScriptVar *c, void*)
{
  // input from javascript.
  int position     = scGetInt("position");
  string chrID_str = c->getParameter("chrID")->getString();
  string nation_1  = c->getParameter("nation_1")->getString();
  string nation_2  = c->getParameter("nation_2")->getString();
  string dirname_1 = c->getParameter("dirname_1")->getString();
  string dirname_2 = c->getParameter("dirname_2")->getString();

  int chrID;
  if(chrID_str == "X")
    chrID = 88;
  else if(chrID_str == "Y")
    chrID = 89;
  else
    chrID = stoi(chrID_str.c_str());

  string in_topdir, topdir;
  GetDirectoryInfo(in_topdir, topdir);
    
  
  // Initialization.
  vector< vector<int> > ContingencyTable(2, vector<int>(3, 0));
  vector<double> p_value(2, 0.0);  // // p_value = p_value[0] * 10^p_value[1].
  string cout = "";


  // Get data from ORAM Tree.
  ORAM_GetData_Fisher(chrID, nation_1, topdir, dirname_1, position, ContingencyTable[0]);
  ORAM_GetData_Fisher(chrID, nation_2, topdir, dirname_2, position, ContingencyTable[1]);


  // Calculate Fisher's exact test.
  FisherExactTest(ContingencyTable, p_value);
  

  double ret_p_value = p_value[0] * pow(10, p_value[1]);  
  c->getReturnVar()->setDouble(ret_p_value);


  string().swap(chrID_str);
  string().swap(cout);
  string().swap(nation_1);
  string().swap(nation_2);
  string().swap(dirname_1);
  string().swap(dirname_2);
  string().swap(in_topdir);
  string().swap(topdir);
  vector< vector<int> >().swap(ContingencyTable);
  vector<double>().swap(p_value);
}


void ML_LogisticRegression(CScriptVar *c, void*)
{
  // input from javascript.
  string chrID_str     = c->getParameter("chrID")->getString();
  string nation_1      = c->getParameter("nat_1")->getString();
  string nation_2      = c->getParameter("nat_2")->getString();
  string dirname_1     = c->getParameter("dirname_1")->getString();
  string dirname_2     = c->getParameter("dirname_2")->getString();
  string position_list = c->getParameter("pos")->getString();
  int iteration        = scGetInt("iter");
  int regularization   = scGetInt("regularize");

  int chrID;
  if(chrID_str == "X")
    chrID = 88;
  else if(chrID_str == "Y")
    chrID = 89;
  else
    chrID = stoi(chrID_str.c_str());

  string in_topdir, topdir;
  GetDirectoryInfo(in_topdir, topdir);

  
  // Initialization
  vector<int> position;
  SplitStringInt(position, position_list, ',');

  
  int M = position.size() + 1;  // row: position size.  col: data num.
  vector< vector<double> > x;
  vector<int> y;
  vector<double> theta;
  string cout = "";


  // Get data from ORAM Tree.
  ORAM_GetData_LR(chrID, nation_1, dirname_1, nation_2, dirname_2, topdir, position, x, y);
  
  
  for(size_t i=0; i<x.size()-1; i++)  assert(x[i].size() == x[i + 1].size());
  assert(x[0].size() == y.size());

  
  // Calculate Logistic Regression.
  LogisticRegression(x, y, theta, iteration, regularization);

  // return theta as js retval.
  CScriptVar *result = c->getReturnVar();
  result->setArray();
  int length = 0;

  for(int i=0; i<theta.size(); i++)
    result->setArrayIndex(length++, new CScriptVar(to_string(theta[i])));
  

  
  string().swap(chrID_str);  
  string().swap(cout);
  string().swap(nation_1);
  string().swap(nation_2);
  string().swap(dirname_1);
  string().swap(dirname_2);
  string().swap(position_list);
  string().swap(in_topdir);
  string().swap(topdir);
  vector<int>().swap(position);
  vector< vector<double> >().swap(x);
  vector<int>().swap(y);
  vector<double>().swap(theta);
}


void ML_PCA(CScriptVar *c, void*)
{
  // input from javascript.
  string chrID_str     = c->getParameter("chrID")->getString();
  string nation        = c->getParameter("nation")->getString();
  string dirname       = c->getParameter("dirname")->getString();
  string position_list = c->getParameter("pos")->getString();
  int n_component      = scGetInt("n_component");

  int chrID;
  if(chrID_str == "X")
    chrID = 88;
  else if(chrID_str == "Y")
    chrID = 89;
  else
    chrID = stoi(chrID_str.c_str());

  string in_topdir, topdir;
  GetDirectoryInfo(in_topdir, topdir);  
  

  // Initialization
  vector<int> position;
  SplitStringInt(position, position_list, ',');

  // [x] row: position size.  col: data num.
  vector< vector<double> > x;
  string cout = "";
  


  // Get data from ORAM Tree.
  ORAM_GetData_PCA(chrID, nation, topdir, dirname, position, x);

  assert((int)x.size() > n_component);
  for(size_t i=0; i<x.size()-1; i++)  assert(x[i].size() == x[i + 1].size());
  
  
  // [PCA_data] row: data num.  col: position size.
  // !! CAUTION !!: PCA_data is opposite of "x" as row and col.
  vector< vector<double> > PCA_data(x[0].size());

  
  // Calculate PCA.
  PCA(x, PCA_data, n_component);

  
  // get retval.
  int row_length = 0;
  int col_length = 0;
  CScriptVar *result = c->getReturnVar();
  result->setArray();

  for(int i=0; i<PCA_data.size(); i++){
    CScriptVar *result_item = new CScriptVar();
    result_item->setArray();
    
    for(int j=0; j<PCA_data[0].size(); j++){
      result_item->setArrayIndex(col_length++, new CScriptVar(to_string(PCA_data[i][j])));
    }

    
    result->setArrayIndex(row_length++, result_item);    
    col_length = 0;
  }

    
  string().swap(chrID_str);
  string().swap(cout);
  string().swap(nation);
  string().swap(dirname);
  string().swap(position_list);
  string().swap(in_topdir);
  string().swap(topdir);
  vector<int>().swap(position);
  vector< vector<double> >().swap(x);
  vector< vector<double> >().swap(PCA_data);
}


void SortListFromTheta(CScriptVar *c, void *)
{
  // input from javascript.
  int num = scGetInt("num");
  string pos_str = c->getParameter("this")->getString();
  CScriptVarLink *theta_item = c->getParameter("theta")->firstChild;


  // Initialization
  vector<int> position;
  SplitStringInt(position, pos_str, ',');
  position.push_back(-1);  // for memory allocation.

  
  vector<double> theta;
  while(theta_item){
    string tmp = theta_item->var->getString();
    theta.push_back(stod(tmp));
    
    theta_item = theta_item->nextSibling;
  }
  theta.push_back(-1.0);  // for memory allocation.

  assert(position.size() + 1 == theta.size());
  assert(num <= position.size() - 1);


  size_t size = position.size() - 1;
  QuickSort_LRresult(&theta[1], &theta[1 + size], &position[0], &position[size]);
  
  // for(size_t i=0; i<theta.size(); i++)     OCALL_print_double(theta[i]);
  // for(size_t i=0; i<position.size(); i++)  OCALL_print_int(position[i]);
  

  // get positions.
  pos_str = "";
  for(int i=0; i<num; i++)
    pos_str += to_string(position[size-1 - i]) + ",";
  
  
  c->getReturnVar()->setString(pos_str);


  
  string().swap(pos_str);
  vector<int>().swap(position);
  vector<double>().swap(theta);
}


void js_print(CScriptVar *c, void *userdata)
{
  // Get session key.
  sgx_status_t ctx, get_keys_ret;
  sgx_ra_key_128_t session_key;
  
  get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &session_key);


  // Get text according to type.
  string text = "";
  
  if(c->getParameter("text")->isNumeric()){
    text = c->getParameter("text")->getParsableString();
  }
  else if(c->getParameter("text")->isString()){
    text = c->getParameter("text")->getString();
  }
  else if(c->getParameter("text")->isArray()){
    CScriptVar *data = c->getParameter("text");

    if(data->getArrayIndex(0)->isArray()){  // 2 dimensions array.
      
      text += "[";

      for(int i=0; i<data->getArrayLength(); i++){
	CScriptVarLink *item = data->getArrayIndex(i)->firstChild;

	if(i == 0){
	  text += "[ ";
	}
	else{
	  text += " [ ";
	}
	
	while(item){
	  text += item->var->getString();
	  item =  item->nextSibling;
	  
	  if(item)
	    text += ", ";
	}

	if(i != data->getArrayLength() - 1){
	  text += " ],\n";
	}
	else{
	  text += " ]";
	}
      }

      text += " ]";
    }
    else{ // 1 dimension array.
      CScriptVarLink *item = c->getParameter("text")->firstChild;
      text += "[ ";
    
      while(item){
	text += item->var->getString();
	item  = item->nextSibling;

	if(item){
	  text += ", ";
	}  
      }

      text += " ]";
    }
  }
  /*
  else if(c->getParameter("text")->isFunction()){
    OCALL_print("isFunction");
  }
  else if(c->getParameter("text")->isObject()){
    OCALL_print("isObject");
  }
  else if(c->getParameter("text")->isNative()){
    OCALL_print("isNative");
  }
  else if(c->getParameter("text")->isUndefined()){
    OCALL_print("isUndefined");
  }
  else if(c->getParameter("text")->isNull()){
    OCALL_print("isNull");
  }
  else if(c->getParameter("text")->isBasic()){
    OCALL_print("isBasic");
  }
  */
  else{
    text = "> Undefined.";
  }


  // Generate ret_txt(uint8_t).
  string ret_txt_str = "STDIO\n" + text;
  string().swap(text);

  int ret_txt_len = ret_txt_str.length();
  uint8_t* ret_txt = new uint8_t[ret_txt_len]();
  for(int i=0; i<ret_txt_len; i++){
    ret_txt[i] = ret_txt_str[i];
  }

  string().swap(ret_txt_str);


  // Initialization for encryption.
  uint8_t ret_txt_len_uint8t[4] = {};
  ConvertIntToUint8_t(ret_txt_len, ret_txt_len_uint8t);

  uint8_t* ret_enc_txt = new uint8_t[ret_txt_len]();
  uint8_t* ret_msg = new uint8_t[ret_txt_len + 32]();
  uint8_t ret_iv[12] = {};
  sgx_aes_gcm_128bit_tag_t ret_tag;
  
  
  // Generate IV.
  for(int i=0; i<NONCE_SIZE; i++){
    ret_iv[i] = (uint8_t)UniformDistribution_int(0, 255);
  }
  

  // Encrypt ret_txt.
  sgx_status_t status;
  status = sgx_rijndael128GCM_encrypt(&session_key, ret_txt, ret_txt_len, ret_enc_txt, ret_iv, NONCE_SIZE, NULL, 0, &ret_tag);

  delete ret_txt;


  // Generate ret_msg.
  for(int i=0; i<ret_txt_len + 32; i++){
    if(i < 4){
      ret_msg[i] = ret_txt_len_uint8t[i];
    }
    else if(i < 16){
      ret_msg[i] = ret_iv[i - 4];
    }
    else if(i < 32){
      ret_msg[i] = ret_tag[i - 16];
    }
    else{
      ret_msg[i] = ret_enc_txt[i - 32];
    }
  }

  delete ret_enc_txt;

  
  OCALL_SendMessage(ret_msg, ret_txt_len + 32 + 1);

  delete ret_msg;
}


void js_fwrite(CScriptVar *c, void *)
{
  // Get session key.
  sgx_status_t ctx, get_keys_ret;
  sgx_ra_key_128_t session_key;
  get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &session_key);

  if(get_keys_ret != SGX_SUCCESS)  return;
  
  
  // input from javascript.
  string filename  = c->getParameter("filename")->getString();


  // Get data according to type.
  string text = "";

  if(c->getParameter("data")->isString()){
    text = c->getParameter("data")->getString();
  }
  else if(c->getParameter("data")->isArray()){
    CScriptVar *data = c->getParameter("data");

    if(data->getArrayIndex(0)->isArray()){  // 2 dimensions array.
      
      for(int i=0; i<data->getArrayLength(); i++){
	CScriptVarLink *item = data->getArrayIndex(i)->firstChild;
	
	while(item){
	  text += item->var->getString();
	  item =  item->nextSibling;
	  
	  if(item)
	    text += ",";
	}

	text += "\n";
      }
    }
    else{ // 1 dimension array.
      CScriptVarLink *item = c->getParameter("data")->firstChild;
    
      while(item){
	text += item->var->getString();
	item  = item->nextSibling;

	if(item){
	  text += ",";
	}  
      }

      text += "\n";
    }
  }
  /*
  else if(c->getParameter("text")->isFunction()){
    OCALL_print("isFunction");
  }
  else if(c->getParameter("text")->isObject()){
    OCALL_print("isObject");
  }
  else if(c->getParameter("text")->isNative()){
    OCALL_print("isNative");
  }
  else if(c->getParameter("text")->isUndefined()){
    OCALL_print("isUndefined");
  }
  else if(c->getParameter("text")->isNull()){
    OCALL_print("isNull");
  }
  else if(c->getParameter("text")->isBasic()){
    OCALL_print("isBasic");
  }
  */
  else{
    text = "Undefined.\n";
  }
  
  
  // Create data for client.
  text = "FILESAVE\n" + filename + "\n" + text + "\n";
  
  
  // change type of data(string -> uint8_t).
  int result_len  = text.length();
  uint8_t* result = new uint8_t[result_len];

  for(int i=0; i<result_len; i++)
    result[i] = text[i];

  string().swap(filename);
  string().swap(text);
  

  // Encrypt data.
  uint8_t* enc_result = new uint8_t[result_len];
  sgx_aes_gcm_128bit_tag_t tag;
  uint8_t* iv = new uint8_t[NONCE_SIZE];

  for(int i=0; i<NONCE_SIZE; i++)
    iv[i] = (uint8_t)UniformDistribution_int(0, 255);

  
  sgx_status_t status;
  status = sgx_rijndael128GCM_encrypt(&session_key, result, result_len, enc_result, iv, NONCE_SIZE, NULL, 0, &tag);

  delete result;
  

  // Concat data and seed into one.
  uint8_t result_len_uint8t[4] = {};
  ConvertIntToUint8_t(result_len, result_len_uint8t);

  uint8_t* ret_msg = new uint8_t[result_len + 32]();
  for(int i=0; i<result_len + 32; i++){
    if(i < 4){
      ret_msg[i] = result_len_uint8t[i];
    }
    else if(i < 16){
      ret_msg[i] = iv[i - 4];
    }
    else if(i < 32){
      ret_msg[i] = tag[i - 16];
    }
    else{
      ret_msg[i] = enc_result[i - 32];
    }
  }


  OCALL_SendMessage(ret_msg, result_len + 32 + 1);


  delete enc_result;
  delete iv;
  delete ret_msg;
}





void registerBiORAMFunctions(CTinyJS *tinyJS, uint8_t *user_aes_key)
{
  // ORAM related.
  tinyJS->addNative("function BiORAM.ORAM.init(Z_blocks, in_dirname, out_dirname)", ORAM_init, user_aes_key);
  // tinyJS->addNative("function BiORAM.ORAM.search(dirname, chrID, nation, position)", ORAM_search, 0);

  // ML related.
  tinyJS->addNative("function BiORAM.ML.Fisher(chrID, nation_1, dirname_1, nation_2, dirname_2, position)", ML_Fisher, 0);
  tinyJS->addNative("function BiORAM.ML.LogisticRegression(chrID, nat_1, dirname_1, nat_2, dirname_2, pos, iter, regularize)",
		    ML_LogisticRegression, 0);
  tinyJS->addNative("function BiORAM.ML.PCA(chrID, nation, dirname, pos, n_component)", ML_PCA, 0);
  tinyJS->addNative("function String.SortListFromTheta(theta, num)", SortListFromTheta, 0);

  // stdio related.
  tinyJS->addNative("function BiORAM.console.log(text)", js_print, 0);
  tinyJS->addNative("function BiORAM.fwrite(filename, data)", js_fwrite, 0);
}


/*
void js_dump(CScriptVar *v, void *userdata)
{
  CTinyJS *js = (CTinyJS*)userdata;
  js->root->trace(">  ");
}
*/


void cp_source(void *ptr, size_t len)
{
  std::string sc = (const char*)ptr;
  printf("%s", sc.c_str());
}


sgx_status_t JSinterpreter(uint8_t *jscode_clump, int jscode_clump_len, uint8_t *AESkey_clump, int AESkey_clump_len)
{
  // Get session key.
  sgx_status_t ctx, get_keys_ret;
  sgx_ra_key_128_t session_key;
  
  get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &session_key);

  if(get_keys_ret != SGX_SUCCESS){
    return get_keys_ret;
  }


  // Split jscode_clumps.
  int jscode_len = jscode_clump_len - 32;
  uint8_t* enc_jscode = new uint8_t[jscode_len]();
  uint8_t iv_jscode[12] = {};
  sgx_aes_gcm_128bit_tag_t tag_jscode;

  memcpy(iv_jscode,  &jscode_clump[4],  12);
  memcpy(tag_jscode, &jscode_clump[16], 16);
  memcpy(enc_jscode, &jscode_clump[32], jscode_len);


  // Split passwd_clumps.
  int AESkey_len = AESkey_clump_len - 32;  // AESkey_len should be 16 byte.
  uint8_t enc_AESkey[AESkey_len] = {};
  uint8_t iv_AESkey[12] = {};
  sgx_aes_gcm_128bit_tag_t tag_AESkey;

  memcpy(iv_AESkey,  &AESkey_clump[4],  12);
  memcpy(tag_AESkey, &AESkey_clump[16], 16);
  memcpy(enc_AESkey, &AESkey_clump[32], AESkey_len);


  // Decryption.
  sgx_status_t status;
  uint8_t* jscode_uint8t = new uint8_t[jscode_len]();
  uint8_t *AESkey        = new uint8_t[AESkey_len]();
  
  status = sgx_rijndael128GCM_decrypt(&session_key, enc_jscode, jscode_len, jscode_uint8t, iv_jscode, 12, NULL, 0, &tag_jscode);
  status = sgx_rijndael128GCM_decrypt(&session_key, enc_AESkey, AESkey_len, AESkey,        iv_AESkey, 12, NULL, 0, &tag_AESkey);

  delete enc_jscode;


  // Convert type jscode: uint8_t -> string.
  string jscode(jscode_len, '\0');
  for(int i=0; i<jscode_len; i++)
    jscode[i] = jscode_uint8t[i];

  delete jscode_uint8t;


  // Preparation for JS interpreter.
  CTinyJS *js = new CTinyJS();  
  registerFunctions(js);
  registerMathFunctions(js);
  registerBiORAMFunctions(js, AESkey);
  

  
  // Execute JS.
  int line_num = 0;

  try{
    js->execute(jscode.c_str());
  }
  catch(CScriptException *e){
    string cerr = "STDERR\n-----\nERROR: " + e->text;
    SendMessage(cerr.c_str(), cerr.length());

      
    string().swap(cerr);

    // for security reason
    memset(AESkey, 0x00, AESkey_len);
    delete AESkey;
      
    string().swap(jscode);
    
    return SGX_ERROR_UNEXPECTED;
  }

  
  // for security reason
  memset(AESkey, 0x00, AESkey_len);
  delete AESkey;

  string().swap(jscode);

  
  return SGX_SUCCESS;
}


sgx_status_t GetUserID_pwhash(uint8_t *userID_clump, int userID_clump_len, uint8_t *userID, int userID_len,
			      uint8_t *passwd_clump, int passwd_clump_len, uint8_t *pwhash)
{
  // Get session key.
  sgx_status_t ctx, get_keys_ret;
  sgx_ra_key_128_t session_key;
  
  get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &session_key);

  if(get_keys_ret != SGX_SUCCESS){
    return get_keys_ret;
  }


  // Split userID_clumps.
  uint8_t enc_userID[userID_len] = {};
  uint8_t iv_userID[12] = {};
  sgx_aes_gcm_128bit_tag_t tag_userID;

  memcpy(iv_userID,  &userID_clump[4],  12);
  memcpy(tag_userID, &userID_clump[16], 16);
  memcpy(enc_userID, &userID_clump[32], userID_len);


  // Split passwd_clumps.
  int passwd_len = passwd_clump_len - 32;
  uint8_t passwd[passwd_len] = {};

  uint8_t enc_passwd[passwd_len] = {};
  uint8_t iv_passwd[12] = {};
  sgx_aes_gcm_128bit_tag_t tag_passwd;

  memcpy(iv_passwd,  &passwd_clump[4],  12);
  memcpy(tag_passwd, &passwd_clump[16], 16);
  memcpy(enc_passwd, &passwd_clump[32], passwd_len);


  // Decryption.
  sgx_status_t status;
  status = sgx_rijndael128GCM_decrypt(&session_key, enc_userID, userID_len, userID, iv_userID, 12, NULL, 0, &tag_userID);
  status = sgx_rijndael128GCM_decrypt(&session_key, enc_passwd, passwd_len, passwd, iv_passwd, 12, NULL, 0, &tag_passwd);


  // convert passwd -> SHA256 hash.
  status = sgx_sha256_msg(passwd, passwd_len, (sgx_sha256_hash_t*)pwhash);

  
  return SGX_SUCCESS;
}


sgx_status_t SendMessage(const char* msg_str, int msg_len)
{
  // Get session key.
  sgx_status_t ctx, get_keys_ret;
  sgx_ra_key_128_t session_key;
  get_keys_ret = sgx_ra_get_keys(ctx, SGX_RA_KEY_SK, &session_key);
  
  if(get_keys_ret != SGX_SUCCESS)  return get_keys_ret;
  

  // Generate IV.
  uint8_t iv[NONCE_SIZE] = {};  
  for(int i=0; i<NONCE_SIZE; i++){
    iv[i] = (uint8_t)UniformDistribution_int(0, 255);
  }


  // Convert type: const char* -> uint8_t.
  uint8_t msg[msg_len] = {};
  for(int i=0; i<msg_len; i++){
    msg[i] = msg_str[i];
  }


  // Encrypt.
  uint8_t enc_msg[msg_len] = {};
  sgx_aes_gcm_128bit_tag_t tag;
  
  sgx_status_t status;
  status = sgx_rijndael128GCM_encrypt(&session_key, msg, msg_len, enc_msg, iv, NONCE_SIZE, NULL, 0, &tag);


  // Convert type: int -> uint8_t.
  uint8_t msg_len_uint8t[4] = {};
  ConvertIntToUint8_t(msg_len, msg_len_uint8t);
  
  
  // Generate send_msg.
  uint8_t send_msg[msg_len + 32] = {};
  memcpy(send_msg, msg_len_uint8t, 4);
  memcpy(&send_msg[4],  iv,  NONCE_SIZE);
  memcpy(&send_msg[16], tag, MAC_SIZE);
  memcpy(&send_msg[32], enc_msg, msg_len);

  
  OCALL_SendMessage(send_msg, msg_len + 32 + 1);
  

  return SGX_SUCCESS;
}



// ***** ↑↑↑ EDIT HERE ↑↑↑ *****
