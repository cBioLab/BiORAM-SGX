/*!
 *
 * client.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 * Some function are released from Intel Corporation.
 * LIENSE: https://github.com/intel/sgx-ra-sample/blob/master/LICENSE
 */


#include "RA_client.hpp"
#include <bitset>
#include <cassert>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>


// ref1: https://github.com/intel/linux-sgx/blob/master/sdk/tlibcrypto/sgxssl/sgx_aes_gcm.cpp#L121
// ref2: https://github.com/intel/linux-sgx/blob/master/sdk/sample_libcrypto/sample_libcrypto.cpp#L270
sample_status_t sample_rijndael128GCM_decrypt(const sample_aes_gcm_128bit_key_t *p_key,
					      const uint8_t *p_src, uint32_t src_len, uint8_t *p_dst,
					      const uint8_t *p_iv, uint32_t iv_len,
					      const uint8_t *p_aad, uint32_t aad_len, sample_aes_gcm_128bit_tag_t *p_in_mac)
{
  uint8_t l_tag[SAMPLE_AESGCM_MAC_SIZE];

  if ((src_len >= INT_MAX) || (aad_len >= INT_MAX) || (p_key == NULL) || ((src_len > 0) && (p_dst == NULL)) || ((src_len > 0) && (p_src == NULL))
                || (p_in_mac == NULL) || (iv_len != SGX_AESGCM_IV_SIZE) || ((aad_len > 0) && (p_aad == NULL))
		|| (p_iv == NULL) || ((p_src == NULL) && (p_aad == NULL)))
    {
      return SAMPLE_ERROR_INVALID_PARAMETER;
    }
  int len = 0;
  sample_status_t ret = SAMPLE_ERROR_UNEXPECTED;
  EVP_CIPHER_CTX * pState = NULL;

  // Autenthication Tag returned by Decrypt to be compared with Tag created during seal
  memset(&l_tag, 0, SAMPLE_AESGCM_MAC_SIZE);   // memset_s(&l_tag, SAMPLE_AESGCM_MAC_SIZE, 0, SAMPLE_AESGCM_MAC_SIZE);
  memcpy(l_tag, p_in_mac, SAMPLE_AESGCM_MAC_SIZE);

  do {
    // Create and initialise the context
    if (!(pState = EVP_CIPHER_CTX_new())) {
      ret = SAMPLE_ERROR_OUT_OF_MEMORY;
      break;
    }

    // Initialise decrypt, key and IV
    if (!EVP_DecryptInit_ex(pState, EVP_aes_128_gcm(), NULL, (unsigned char*)p_key, p_iv)) {
      break;
    }

    // Provide AAD data if exist
    if (NULL != p_aad) {
      if (!EVP_DecryptUpdate(pState, NULL, &len, p_aad, aad_len)) {
	break;
      }
    }

    // Decrypt message, obtain the plaintext output
    if (!EVP_DecryptUpdate(pState, p_dst, &len, p_src, src_len)) {
      break;
    }

    // Update expected tag value
    if (!EVP_CIPHER_CTX_ctrl(pState, EVP_CTRL_GCM_SET_TAG, SAMPLE_AESGCM_MAC_SIZE, l_tag)) {
      break;
    }
    
    // Finalise the decryption. A positive return value indicates success,
    // anything else is a failure - the plaintext is not trustworthy.
    if (EVP_DecryptFinal_ex(pState, p_dst + len, &len) <= 0) {
      ret = SAMPLE_ERROR_UNEXPECTED;  // ret = SGX_ERROR_MAC_MISMATCH;
      break;
    }

    ret = SAMPLE_SUCCESS;    
  } while (0);


  // Clean up and return
  if (pState != NULL) {
    EVP_CIPHER_CTX_free(pState);
  }
  memset(&l_tag, 0, SAMPLE_AESGCM_MAC_SIZE);  // memset_s(&l_tag, SGX_AESGCM_MAC_SIZE, 0, SGX_AESGCM_MAC_SIZE);
  

  return ret;
}


int ReadFile(string& filename, string& data)
{
  ifstream ifs(filename, ios::binary);

  if(!ifs){
    cerr << "File cannot open..." << endl;
    cerr << "filename: " << filename << endl;

    ifs.close();
    return -1;
  }


  // Read File.
  stringstream strstream;
  strstream << ifs.rdbuf();
  ifs.close();

  data = strstream.str();
  // cout << data << endl;
  
  return 0;
}


int WriteFile(string& filename, string& data)
{
  ofstream ofs(filename, ios::binary);

  if(!ofs){
    cerr << "File cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    return -1;
  }


  assert(data != "");
  ofs << data;

  
  ofs.close();
}


void CreateRandomCharacter(uint8_t *character, int size)
{
  random_device random;
  mt19937 mt(random());
  uniform_int_distribution<> rand(0, 255);

  for(int i=0; i<size; i++)
   character[i] = rand(mt);

  
  return;
}


/*
 * convert type: int -> uint8_t*.
 * < input >
 * [int]   from: variable to convert.
 * [uint8_t] to: converted variable (should be prepared 4 bit). 
 */
void ConvertIntToUint8_t(int from, uint8_t* to){
  bitset<32> bit_num(from);
  int ret = 0;
  
  
  for(size_t i=0; i<bit_num.size(); i++){
    ret += bit_num[i] * pow(2, i % 8);

    // 1[byte] ごとにuint8_t に変換する．
    if(i % 8 == 7){
      to[(int)i / 8] = (char)ret;

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


int DoActionFromMessage(ra_session_t session, uint8_t *get_msg_all)
{
  // Get length of message.
  uint8_t get_msg_len_uint8t[4] = {};
  for(int i=0; i<4; i++)
    get_msg_len_uint8t[i] = get_msg_all[i];
  
  int get_msg_len;
  ConvertUint8_tToInt(get_msg_len_uint8t, get_msg_len);
  // cout << get_msg_len << endl;
  
  
  // Get iv, tag and ciphertext.
  sample_aes_gcm_128bit_tag_t get_tag;
  uint8_t get_iv[12] = {};  
  uint8_t get_enc_msg[get_msg_len] = {};

  memcpy(get_iv,      &get_msg_all[4],  12);
  memcpy(get_tag,     &get_msg_all[16], 16);
  memcpy(get_enc_msg, &get_msg_all[32], get_msg_len);
  
  
  // Decrypt.
  sample_status_t dec_status;
  uint8_t dec_msg[get_msg_len] = {};
  dec_status = sample_rijndael128GCM_decrypt(&session.sk, get_enc_msg, get_msg_len, dec_msg, get_iv, 12, NULL, 0, &get_tag);
  
  
  // Convert type: uint8_t -> string.
  string message(get_msg_len, '\0');
  for(int i=0; i<get_msg_len; i++){
    message[i] = dec_msg[i];
  }
  
  
  string action = message.substr(0, message.find("\n"));
  if(action == "FinishConnecting"){
    return 1;
  }
  else if(action == "STDIO"){
    message = message.substr(message.find("\n") + 1);
    cout << message << endl;
  }
  else if(action == "STDERR"){
    message = message.substr(message.find("\n") + 1);
    cout << message << endl;

    sleep(1);

    return -1;
  }
  else if(action == "FILESAVE"){
    message = message.substr(message.find("\n") + 1);
    
    string filename = message.substr(0, message.find("\n"));
    string content  = message.substr(message.find("\n") + 1);

    WriteFile(filename, content);
  }
  else if(action == "OK"){
    cout << "User verification is succeeded!!" << endl;
    return 1;
  }
  else if(action == "NG"){
    cout << "User verification is failed..." << endl;
    return -1;
  }
  

  return 0;
}


void SendMessage(ra_session_t session, string& context_str, bool sleep_flg)
{
  // Convert type: string -> uint8_t.
  size_t context_len = context_str.length();
  uint8_t *context = new uint8_t[context_len]();
  for(size_t i=0; i<context_len; i++){
    context[i] = context_str[i];
  }


  // Convert type: size_t -> uint8_t.
  uint8_t context_len_uint8t[4] = {};
  ConvertIntToUint8_t((int)context_len, context_len_uint8t);

  
  // Create seed.
  sample_aes_gcm_128bit_tag_t tag;
  uint8_t iv[12] = {};
  CreateRandomCharacter(iv, 12);
  // uint8_t iv[12] = {65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76};


  // Encrypt context.
  uint8_t* enc_context = new uint8_t[context_len]();

  sample_status_t status;
  status = sample_rijndael128GCM_encrypt(&session.sk, context, context_len, enc_context, iv, 12, NULL, 0, &tag);
  


  // create send_msg.
  uint8_t* send_msg = new uint8_t[context_len + 32]();
  memcpy(send_msg, context_len_uint8t, 4);
  memcpy(&send_msg[4],  iv,  12);
  memcpy(&send_msg[16], tag, 16);
  memcpy(&send_msg[32], enc_context, context_len);


  // Send message.
  msgio->send((void *) send_msg, context_len + 32);

  if(sleep_flg == true){
    sleep(1);
  }


  delete(context);
  delete(enc_context);
  delete(send_msg);

  return;
}




int main(int argc, char *argv[])
{
  // ***** ↓↓↓ Remote Attestation ↓↓↓ *****
   
  
  // Initialization.
  int retval;
  ra_session_t session;

  
  retval = handshake_RA(argc, argv, session);  // -2: Error during RA, -1: abend, 0: invalid input, 1: normal.

  if(msgio == NULL){
    if(retval == -1){
      cerr << "Abnormal End..." << endl;
      return 1;
    }
    else if(retval == 0){
      cerr << "Invalid Input..." << endl;
      return 0;
    }
    else if(retval == -2){
      cerr << "Error during Remote Attestation..." << endl;

      crypto_destroy();
      return 1;
    }
  }
  

  // ***** ↑↑↑ Remote Attestation ↑↑↑ *****

  cout << "\n\n---- ↑↑↑ Remote Attestation Handshake ↑↑↑ ----------------------------------\n\n" << endl;  
  

  // Select Operations.
  string userID = "";
  string passwd = "";
  string key_filename = "";
  string js_filename  = "";

  
  // Input userID, password.
  cout << "Input your user ID: ";
  cin >> userID;

  cout << "Input your ID's password: ";
  cin >> passwd;

  cout << "(If you do not have key, push ENTER only.)" << endl;
  cout << "Input your SK filename(): ";
  cin.ignore();  getline(cin, key_filename);
    
  cout << "Input your JavaScript code: ";
  cin >> js_filename;
  

  
  // Send ID & passwd
  SendMessage(session, userID, true);
  SendMessage(session, passwd, false);


  // Receive result of user verification.
  uint8_t *user_check = NULL;
  msgio->read((void **) &user_check, NULL);

  if(DoActionFromMessage(session, user_check) < 0){
    crypto_destroy();
    return 1;
  }
  

  // Get and send Javascript code.
  string jscode;
  ReadFile(js_filename, jscode);

  SendMessage(session, jscode, true);


  // Get and send AES secretkey.
  string AESkey;
  if(key_filename != ""){
    ReadFile(key_filename, AESkey);
  }
  else{
    // do not use key on server, but need to send it for security reason.
    AESkey = "0000000000000000";
  }
  
  SendMessage(session, AESkey, true);

  
  // Receive messages.
  int conn_flag = 0;
  while(conn_flag == 0){
    uint8_t *get_msg_all = NULL;
    msgio->read((void **) &get_msg_all, NULL);


    // Do Actions in accordance with message.
    conn_flag = DoActionFromMessage(session, get_msg_all);
  }
  
  

  if(conn_flag > 0){
    cout << "All procedure successfully!!" << endl;
  }
    
  crypto_destroy();
  
  return 0;
}
