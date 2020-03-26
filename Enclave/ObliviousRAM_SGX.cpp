/*!
 *
 * Oblivious_RAM.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#include "ObliviousRAM_SGX.hpp"


ORAM_Tree* tree_item;
ORAM_Tree* enc_item;
uint8_t* filename;
uint8_t* nonce;
uint8_t* enc_ORAMTreeItem;
sgx_aes_gcm_128bit_tag_t AES_TAG;
sgx_aes_gcm_128bit_tag_t AES_TAG_block;
sgx_aes_gcm_128bit_tag_t AES_TAG_content;
sgx_aes_gcm_128bit_tag_t AES_TAG_LeafID;
sgx_aes_gcm_128bit_tag_t AES_TAG_TFflag;





/* 
 * Function to find the minimum power of 2 over a specified number.
 * ref: https://qiita.com/NotFounds/items/4648a3793d1b2a4f11d5
 * < input >
 * [int] n: specified number.
 *
 * < output >
 * [int]: the minimum power of 2 over "n".
 */
int NextPow2(int n)
{
  // If n <= 0, retval is 0.
  if(n <= 0)
    return 0;


  // If (n & (n - 1)) == 0, retval is "n".
  if((n & (n - 1)) == 0)
    return n;

  // Get power of 2 using bit shift.
  int ret = 1;
  while(n > 0){
    ret <<= 1;
    n >>= 1;
  }


  return ret;
}


/*
 * Function to split string.
 * < input >
 * [vector<string>] result: vector to hold each splitted string.
 * [string]            str: original string.
 * [char]              del: deliminator (ex. ',').
 */
void SplitString(vector<string>& result, string str, char del)
{
  int first   = 0;
  int last    = str.find_first_of(del);
  int vec_num = 0;
  
  while(first < str.size()){
    string split_str(str, first, last - first);
    
    result.push_back(split_str);
    vec_num++;

    
    first = last + 1;
    last = str.find_first_of(del, first);

    
    if(last == string::npos)
      last = str.size();
  }
  

  return;
}





// =============== ORAM Support Functions ===============


/*
 * Function to find vacant stash in ascending order. 
 * < input >
 * [ORAM_Stash*]  stash: stash.
 * [int]     stash_size: stash size.
 * [int&] stash_vec_num: start number to search stash.
 *
 * < output >
 * [int] stash_vec_num: vec number of vacant stash firstly.
 */
int GetVacantStashNumber_ascend(ORAM_Stash* stash, int stash_size, int& stash_vec_num)
{  
  while(1){

    // If not find vacant stash, it failure.
    if(stash_vec_num > stash_size - 1){
      string cerr = "Stash Table is already full. Stash FAILURE.";
      OCALL_cerror(cerr.c_str());
    }
    
    if(stash[stash_vec_num].valid[0] == '0')
      break;
    
    
    stash_vec_num++;
  }


  return stash_vec_num;
}


/*
 * Function to find vacant stash in descending order. 
 * < input >
 * [ORAM_Stash*]  stash: stash.
 * [int]     stash_size: stash size.
 * [int&] stash_vec_num: start number tob search stash.
 * 
 * < output >
 * [int] stash_vec_num: vec number of vacant stash firstly.
 */
int GetVacantStashNumber_descend(ORAM_Stash* stash, int stash_size, int& stash_vec_num)
{  
  while(1){

    // If not find vacant stash, it failure.
    if(stash_vec_num <= 0){
      string cerr = "Stash Table is already full. Stash FAILURE.";
      OCALL_cerror(cerr.c_str());
    }
    
    if(stash[stash_vec_num].valid[0] == '0')
      break;
    
    
    stash_vec_num--;
  }

  
  return stash_vec_num;
}


/*
 * Get vec number that correspond to LeafID.
 * < input >
 * [int] L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int] Z_blocks: num of blocks in each node.
 * [int]   LeafID: Leaf ID. Count from 0 from left to right.
 *
 * < output >
 * [int] vec number that correspond to LeafID.
 */
int GetLeafVectorNumber(int L_height, int Z_blocks, int LeafID)
{
  return Z_blocks * ( pow(2, L_height) - 1 + LeafID );
}


/*
 * Get node number that correspond to LeafID.
 * < input >
 * [int] L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]   LeafID: Leaf ID. Count from 0 from left to right.
 *
 * < output >
 * [int] node number that correspond to LeafID.
 */
int GetLeafNodeNumber(int L_height, int LeafID)
{
  return pow(2, L_height) + LeafID;
}


/*
 * Function to get vec number that correspond to node number.
 * < input >
 * [int] node_num: node number.
 * [int] Z_blocks: num of blocks in each node.
 */
int GetVectorNumberFromNode(int node_num, int Z_blocks)
{
  return (node_num - 1) * Z_blocks;
}


/*
 * Function to get minimum LeafID that correspond to node.
 * < input >
 * [int] node_num: node number.
 * [int] L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]   L_diff: height of ORAM Tree between node_num and leaf.
 *
 * < output >
 * [int] minimum LeafID.
 */
int GetLeafID_start(int node_num, int L_height, int L_diff)
{
  return node_num * pow(2, L_diff) - pow(2, L_height);
}


/*
 * Function to get maximum LeafID that correspond to node.
 * < input >
 * [int] node_num: node number.
 * [int] L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]   L_diff: height of ORAM Tree between node_num and leaf.
 *
 * < output >
 * [int] maximum LeafID.
 */
int GetLeafID_end(int node_num, int L_height, int L_diff)
{
  return (node_num + 1) * pow(2, L_diff) - pow(2, L_height) - 1;
}


// ======================================================





// =============== ORAM Table Initialize / Clean / Load / Save ===============


/*
 * Function to initialize elements of ORAM_Tree type.
 * < input >
 * [ORAM_Tree] *tree_item.
 */
void InitializeORAMTreeItem(ORAM_Tree *tree_item)
{
  tree_item->block         = new uint8_t[MAX_BLOCK_SIZE]();
  tree_item->block_nonce   = new uint8_t[NONCE_SIZE]();
  tree_item->content       = new uint8_t[CONTENT_SIZE]();
  tree_item->content_nonce = new uint8_t[NONCE_SIZE]();
  tree_item->LeafID        = new int[1]();
  tree_item->LeafID_nonce  = new uint8_t[NONCE_SIZE]();
  tree_item->TFflag        = new uint8_t[FLAG_SIZE]();
  tree_item->TFflag_nonce  = new uint8_t[NONCE_SIZE]();


  return;
}


/*
 * Function to free elements of ORAM_Tree type.
 * < input >
 * [ORAM_Tree] *tree_item.
 */
void CleanORAMTreeItem(ORAM_Tree *tree_item)
{
  delete[] tree_item->block;
  delete[] tree_item->block_nonce;
  delete[] tree_item->content;
  delete[] tree_item->content_nonce;
  delete[] tree_item->LeafID;
  delete[] tree_item->LeafID_nonce;
  delete[] tree_item->TFflag;
  delete[] tree_item->TFflag_nonce;
  delete[] tree_item;

  return;
}


/*
 * Function to initialize global variables. call this function in each program at first.
 */
void InitializeGlobalVariable()
{
  tree_item = new ORAM_Tree[1]();
  enc_item  = new ORAM_Tree[1]();
  InitializeORAMTreeItem(tree_item);
  InitializeORAMTreeItem(enc_item);

  filename         = new uint8_t[256]();
  nonce            = new uint8_t[NONCE_SIZE]();
  enc_ORAMTreeItem = new uint8_t[ORAMTreeItem_len]();
}


/*
 * Function to free global variables. call this function in each program finally.
 */
void CleanGlobalVariable()
{
  CleanORAMTreeItem(tree_item);
  CleanORAMTreeItem(enc_item);

  delete[] filename;
  delete[] nonce;
  delete[] enc_ORAMTreeItem;
}


/*
 * Function to initialize PositionMap.
 * < input >
 * [ORAM_PositionMap*] p_map.
 * [int]           pmap_size: p_map size.
 */
void InitializePositionMap(ORAM_PositionMap* p_map, int pmap_size)
{
  for(int i=0; i<pmap_size; i++){
    p_map[i].block  = new uint8_t[MAX_BLOCK_SIZE]();
    p_map[i].nation = new uint8_t[NATION_SIZE]();
  }


  return;
}


/*
 * Function to free PositionMap.
 * < input >
 * [ORAM_PositionMap*] p_map.
 * [int]           pmap_size: p_map size.
 */
void CleanPositionMap(ORAM_PositionMap* p_map, int pmap_size)
{
  for(int i=0; i<pmap_size; i++){
    delete[] p_map[i].block;
    delete[] p_map[i].nation;
  }


  return;
}


/*
 * Function to initialize Stash.
 * < input >
 * [ORAM_Stash*] stash.
 * [int]    stash_size: stash size.
 */
void InitializeStash(ORAM_Stash* stash, int stash_size)
{
  // define/delete stash.content in each time because the size of stash.content is too big.
  for(int i=0; i<stash_size; i++){
    stash[i].block = new uint8_t[MAX_BLOCK_SIZE]();
    stash[i].valid = new uint8_t[FLAG_SIZE]();
  }


  return;
}


/*
 * Function to free Stash.
 * < input >
 * [ORAM_Stash*] stash.
 * [int]    stash_size: stash size.
 */
void CleanStash(ORAM_Stash* stash, int stash_size)
{
  for(int i=0; i<stash_size; i++){
    delete[] stash[i].block;
    delete[] stash[i].valid;
  }


  return;
}


/*
 * Function to initialize ORAM_Search.
 * < input >
 * [ORAM_Search*] search.
 */
void InitializeORAMSearch(ORAM_Search& search)
{
  search.block   = new uint8_t[MAX_BLOCK_SIZE]();
  search.content = new uint8_t[CONTENT_SIZE]();
  search.LeafID  = -1;


  return;
}


/*
 * Function to free ORAM_Search.
 * < input >
 * [ORAM_Search*] search.
 */
void CleanORAMSearch(ORAM_Search& search)
{
  delete[] search.block;
  delete[] search.content;


  return;
}


/*
 * Function to load PositionMap.
 * <input>
 * [string]              path: path to PositionMap.
 * [string]        p_map_file: filename of PositionMap.
 * [ORAM_PositionMap*]  p_map.
 * [int]             map_size: p_map size.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void LoadPositionMap(string path, string p_map_file, ORAM_PositionMap* p_map, int pmap_size, sgx_key_128bit_t& AES_SK)
{
  sgx_status_t status;

  
  // Initialization for AES.
  uint8_t* LeafID_bit   = new uint8_t[4]();
  uint8_t* chrID_bit    = new uint8_t[4]();
  uint8_t* position_bit = new uint8_t[4]();

  int data_len      = pmap_size * (MAX_BLOCK_SIZE + 3 * sizeof(int) + NATION_SIZE);
  uint8_t* data     = new uint8_t[data_len]();
  uint8_t* enc_data = new uint8_t[data_len]();

  
  string filename_str = path + p_map_file + TABLE_extension;
  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }
  
  string().swap(filename_str);
  
  
#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_start();
#endif

  /*
  int saved_len = 0;
  status        = OCALL_GetFileLength(&saved_len, filename, filename_len);  

  assert(saved_len == data_len + NONCE_SIZE + MAC_SIZE);
  */

  // Load data.
  size_t saved_len    = data_len + NONCE_SIZE + MAC_SIZE;
  uint8_t *saved_data = new uint8_t[saved_len]();
  status              = OCALL_LoadFile(filename, filename_len, saved_data, saved_len);
  
  
#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_end();
#endif


  // split data to ciphertext and seed.
  int phase_1 = NONCE_SIZE;
  int phase_2 = phase_1 + MAC_SIZE;

  memcpy(nonce,    saved_data,           NONCE_SIZE);
  memcpy(AES_TAG,  &saved_data[phase_1], MAC_SIZE);
  memcpy(enc_data, &saved_data[phase_2], data_len);
  
  delete[] saved_data;

    
#ifdef MEASURE_AESdec
  OCALL_chrono_AESdec_start();
#endif

  // decrypt data.
  status = sgx_rijndael128GCM_decrypt(&AES_SK, enc_data, data_len, data, nonce, NONCE_SIZE, NULL, 0, &AES_TAG);

  delete[] enc_data;
  
#ifdef MEASURE_AESdec
  OCALL_chrono_AESdec_end();
#endif  


  // split data to elements of PositionMap.
  int pmap_item = MAX_BLOCK_SIZE + 3 * sizeof(int) + NATION_SIZE;
  phase_1     = MAX_BLOCK_SIZE;
  phase_2     = phase_1 + sizeof(int);
  int phase_3 = phase_2 + sizeof(int);
  int phase_4 = phase_3 + sizeof(int);

  
  for(size_t i=0; i<pmap_size; i++){
    memcpy(p_map[i].block,  &data[i * pmap_item],           MAX_BLOCK_SIZE);
    memcpy(LeafID_bit,      &data[i * pmap_item + phase_1], sizeof(int));
    memcpy(chrID_bit,       &data[i * pmap_item + phase_2], sizeof(int));
    memcpy(position_bit,    &data[i * pmap_item + phase_3], sizeof(int));
    memcpy(p_map[i].nation, &data[i * pmap_item + phase_4], NATION_SIZE);

    // Convert typr: uint8_t -> int.
    ConvertUint8_tToInt(LeafID_bit,   p_map[i].LeafID);
    ConvertUint8_tToInt(chrID_bit,    p_map[i].chrID);
    ConvertUint8_tToInt(position_bit, p_map[i].position);
  }


  
  delete[] LeafID_bit;
  delete[] chrID_bit;
  delete[] position_bit;
  delete[] data;
  
  return;
}


/*
 * Function to save PositionMap.
 * <input>
 * [string]              path: path to PositionMap.
 * [string]        p_map_file: filename of PositionMap.
 * [ORAM_PositionMap*]  p_map.
 * [int]             map_size: p_map size.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void SavePositionMap(string path, string p_map_file, ORAM_PositionMap* p_map, int pmap_size, sgx_key_128bit_t& AES_SK)
{
  sgx_status_t status;

  
  uint8_t* LeafID_bit   = new uint8_t[4]();
  uint8_t* chrID_bit    = new uint8_t[4]();
  uint8_t* position_bit = new uint8_t[4]();
  
  int pmap_item = MAX_BLOCK_SIZE + 3 * sizeof(int) + NATION_SIZE;
  int phase_1 = MAX_BLOCK_SIZE;
  int phase_2 = phase_1 + sizeof(int);
  int phase_3 = phase_2 + sizeof(int);
  int phase_4 = phase_3 + sizeof(int);

  // Initialization for AES.
  size_t data_len   = pmap_size * pmap_item;
  uint8_t* data     = new uint8_t[data_len]();
  uint8_t* enc_data = new uint8_t[data_len]();

  
  // Combine PositionMap into one variable.
  for(size_t i=0; i<pmap_size; i++){
    // Convert type: int -> uint8_t.
    ConvertIntToUint8_t(p_map[i].LeafID,   LeafID_bit);
    ConvertIntToUint8_t(p_map[i].chrID,    chrID_bit);
    ConvertIntToUint8_t(p_map[i].position, position_bit);
    
    memcpy(&data[i * pmap_item],           p_map[i].block,  MAX_BLOCK_SIZE);
    memcpy(&data[i * pmap_item + phase_1], LeafID_bit,      sizeof(int));
    memcpy(&data[i * pmap_item + phase_2], chrID_bit,       sizeof(int));
    memcpy(&data[i * pmap_item + phase_3], position_bit,    sizeof(int));
    memcpy(&data[i * pmap_item + phase_4], p_map[i].nation, NATION_SIZE);
  }

  delete[] LeafID_bit;
  delete[] chrID_bit;
  delete[] position_bit;
  

  string filename_str = path + p_map_file + TABLE_extension;
  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }

  string().swap(filename_str);

  
  // Create seed.
  for(int i=0; i<NONCE_SIZE; i++)
    nonce[i] = (uint8_t)UniformDistribution_int(0, 255);

  
  // Encrypt data uing AES-GCM.
#ifdef MEASURE_AESenc
  OCALL_chrono_AESenc_start();
#endif
  
  status = sgx_rijndael128GCM_encrypt(&AES_SK, data, data_len, enc_data, nonce, NONCE_SIZE, NULL, 0, &AES_TAG);

#ifdef MEASURE_AESenc
  OCALL_chrono_AESenc_end();
#endif
  
  delete[] data;
  

  // Save ciphertext and seed.
#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_start();
#endif
  
  status = OCALL_SaveFile(filename,     filename_len, nonce,    NONCE_SIZE);
  status = OCALL_SaveFile_add(filename, filename_len, AES_TAG,  MAC_SIZE);
  status = OCALL_SaveFile_add(filename, filename_len, enc_data, data_len);

#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_end();
#endif

  

  delete[] enc_data;
  
  return;
}


/*
 * Function to load Stash.
 * <input>
 * [string]              path: path to stash.
 * [vector<ORAM_Stash>] stash.
 * [int]           stash_size: stash size.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void LoadStash(string path, ORAM_Stash* stash, int stash_size, sgx_key_128bit_t& AES_SK)
{
  sgx_status_t status;
  string filename_str = "";

  // Initialization for AES.
  int data_len        = MAX_BLOCK_SIZE + sizeof(int) + FLAG_SIZE + CONTENT_SIZE;
  uint8_t* data       = new uint8_t[data_len]();
  uint8_t* enc_data   = new uint8_t[data_len]();
  uint8_t* LeafID_bit = new uint8_t[4]();

  int saved_len       = data_len + NONCE_SIZE + MAC_SIZE;
  uint8_t *saved_data = new uint8_t[saved_len]();
  

  for(int i=0; i<stash_size; i++){

    filename_str        = path + to_string(i) + TABLE_extension;
    size_t filename_len = filename_str.length() + 1;
    for(size_t j=0; j<filename_len; j++){
      if(j == filename_len - 1)
	filename[j] = '\0';
      else
	filename[j] = filename_str[j];
    }
    
    
#ifdef MEASURE_FileIn
    OCALL_chrono_FileIn_start();
#endif

    /*
    int saved_len = 0;
    status        = OCALL_GetFileLength(&saved_len, filename, filename_len);

    assert(saved_len == data_len + NONCE_SIZE + MAC_SIZE);
    */

    
    // Load data.
    status = OCALL_LoadFile(filename, filename_len, saved_data, saved_len);
    
#ifdef MEASURE_FileIn
    OCALL_chrono_FileIn_end();
#endif

    
    // split data into ciphertext and seed.
    int phase_1 = NONCE_SIZE;
    int phase_2 = phase_1 + MAC_SIZE;

    memcpy(nonce,    saved_data, NONCE_SIZE);
    memcpy(AES_TAG,  &saved_data[phase_1], MAC_SIZE);
    memcpy(enc_data, &saved_data[phase_2], data_len);
    
    
#ifdef MEASURE_AESdec
    OCALL_chrono_AESdec_start();
#endif

    // decrypt data.
    status = sgx_rijndael128GCM_decrypt(&AES_SK, enc_data, data_len, data, nonce, NONCE_SIZE, NULL, 0, &AES_TAG);
    
    
#ifdef MEASURE_AESdec
    OCALL_chrono_AESdec_end();
#endif
    
    
    // split data into elements of Stash.
    phase_1     = MAX_BLOCK_SIZE;
    phase_2     = phase_1 + sizeof(int);
    int phase_3 = phase_2 + FLAG_SIZE;


    // Define stash.content in each time.
    stash[i].content = new uint8_t[CONTENT_SIZE]();


    memcpy(stash[i].block,   &data,          MAX_BLOCK_SIZE);
    memcpy(LeafID_bit,       &data[phase_1], sizeof(int));
    memcpy(stash[i].valid,   &data[phase_2], FLAG_SIZE);
    memcpy(stash[i].content, &data[phase_3], CONTENT_SIZE);

    ConvertUint8_tToInt(LeafID_bit, stash[i].LeafID);
    
    
    if(stash[i].valid[0] == '0'){
      memset(stash[i].block, '\0', MAX_BLOCK_SIZE);
      delete[] stash[i].content;
    }
  }



  string().swap(filename_str);
  delete[] saved_data;
  delete[] enc_data;
  delete[] data;
  delete[] LeafID_bit;
  
  return;
}


/*
 * Function to save Stash.
 * <input>
 * [string]              path: path to stash.
 * [vector<ORAM_Stash>] stash.
 * [int]           stash_size: stash size.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void SaveStash(string path, ORAM_Stash* stash, int stash_size, sgx_key_128bit_t& AES_SK)
{
  sgx_status_t status;
  string filename_str = "";
  string cout         = "";

  // Initialization for AES.
  int data_len = MAX_BLOCK_SIZE + sizeof(int) + FLAG_SIZE + CONTENT_SIZE;
  uint8_t* data       = new uint8_t[data_len]();
  uint8_t* enc_data   = new uint8_t[data_len]();
  uint8_t* LeafID_bit = new uint8_t[4]();

  
  for(int i=0; i<stash_size; i++){
    filename_str        = path + to_string(i) + TABLE_extension;
    size_t filename_len = filename_str.length() + 1;
    for(size_t j=0; j<filename_len; j++){
      if(j == filename_len - 1)
	filename[j] = '\0';
      else
	filename[j] = filename_str[j];
    }
    // OCALL_print_uint8_t(filename, filename_len);
    

    // Create seed.
    for(int j=0; j<NONCE_SIZE; j++)
      nonce[j] = (uint8_t)UniformDistribution_int(0, 255);

    
    // if stash does not hold data, do padding.
    if(stash[i].valid[0] == '0'){
      memset(stash[i].block, '0', MAX_BLOCK_SIZE);
      stash[i].content = new uint8_t[CONTENT_SIZE]();
    }
    
    
    // Combine Stash into one variable.
    int phase_1 = MAX_BLOCK_SIZE;
    int phase_2 = phase_1 + sizeof(int);
    int phase_3 = phase_2 + FLAG_SIZE;
    int phase_4 = phase_3 + CONTENT_SIZE;

    // Convert type: int -> uint8_t.
    ConvertIntToUint8_t(stash[i].LeafID, LeafID_bit);

    memcpy(&data[0],       stash[i].block,   MAX_BLOCK_SIZE);
    memcpy(&data[phase_1], LeafID_bit,       sizeof(int));
    memcpy(&data[phase_2], stash[i].valid,   FLAG_SIZE);
    memcpy(&data[phase_3], stash[i].content, CONTENT_SIZE);
    
    
#ifdef MEASURE_AESenc
    OCALL_chrono_AESenc_start();
#endif

    // Encrypt data uing AES-GCM.
    status = sgx_rijndael128GCM_encrypt(&AES_SK, data, data_len, enc_data, nonce, NONCE_SIZE, NULL, 0, &AES_TAG);
    
    
#ifdef MEASURE_AESenc
    OCALL_chrono_AESenc_end();
#endif
    
    
#ifdef MEASURE_FileOut
    OCALL_chrono_FileOut_start();
#endif

    // Save ciphertext and seed.
    status = OCALL_SaveFile(filename,     filename_len, nonce,    NONCE_SIZE);
    status = OCALL_SaveFile_add(filename, filename_len, AES_TAG,  MAC_SIZE);
    status = OCALL_SaveFile_add(filename, filename_len, enc_data, data_len);

#ifdef MEASURE_FileOut
    OCALL_chrono_FileOut_end();
#endif


    delete[] stash[i].content;
  }
  

  
  string().swap(filename_str);
  string().swap(cout);
  delete[] enc_data;
  delete[] data;
  delete[] LeafID_bit;
  
  return;
}


/*
 * Function to load vec_num's ORAM Tree item.
 * <input>
 * [ORAM_Tree*]     tree_item: item of ORAM Tree.
 * [ORAM_Tree*]      enc_item: item of ORAM Tree (for ciphertext).
 * [int]              vec_num: number of ORAM Tree's array.
 * [string]              path: path to ORAM Tree.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void LoadORAMTreeItem(ORAM_Tree *tree_item, ORAM_Tree *enc_item, int vec_num, string path, sgx_key_128bit_t& AES_SK)
{
  string filename_str = path + to_string(vec_num) + ITEM_extension;
  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }

  string().swap(filename_str);
  
  
  // Initialization for AES.
  sgx_status_t status;

  uint8_t* LeafID_bit = new uint8_t[4]();
  uint8_t* LeafID_enc = new uint8_t[4]();


#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_start();
#endif

  /*
  int data_len = 0;
  status       = OCALL_GetFileLength(&data_len, filename, filename_len);

  assert(data_len == ORAMTreeItem_len);
  */

  
  // Load data.
  status = OCALL_LoadFile(filename, filename_len, enc_ORAMTreeItem, ORAMTreeItem_len);

#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_end();
#endif
  


  // Split data into elements for ORAM Tree item.
  int phase_1  = MAX_BLOCK_SIZE;
  int phase_2  = phase_1  + NONCE_SIZE;
  int phase_3  = phase_2  + MAC_SIZE;
  int phase_4  = phase_3  + CONTENT_SIZE;
  int phase_5  = phase_4  + NONCE_SIZE;
  int phase_6  = phase_5  + MAC_SIZE;
  int phase_7  = phase_6  + sizeof(int);
  int phase_8  = phase_7  + NONCE_SIZE;
  int phase_9  = phase_8  + MAC_SIZE;
  int phase_10 = phase_9  + FLAG_SIZE;
  int phase_11 = phase_10 + NONCE_SIZE;

  memcpy(enc_item->block,          &enc_ORAMTreeItem[0],        MAX_BLOCK_SIZE);
  memcpy(tree_item->block_nonce,   &enc_ORAMTreeItem[phase_1],  NONCE_SIZE);
  memcpy(AES_TAG_block,            &enc_ORAMTreeItem[phase_2],  MAC_SIZE);
  memcpy(enc_item->content,        &enc_ORAMTreeItem[phase_3],  CONTENT_SIZE);
  memcpy(tree_item->content_nonce, &enc_ORAMTreeItem[phase_4],  NONCE_SIZE);
  memcpy(AES_TAG_content,          &enc_ORAMTreeItem[phase_5],  MAC_SIZE);
  memcpy(LeafID_enc,               &enc_ORAMTreeItem[phase_6],  sizeof(int));
  memcpy(tree_item->LeafID_nonce,  &enc_ORAMTreeItem[phase_7],  NONCE_SIZE);
  memcpy(AES_TAG_LeafID,           &enc_ORAMTreeItem[phase_8],  MAC_SIZE);
  memcpy(enc_item->TFflag,         &enc_ORAMTreeItem[phase_9],  FLAG_SIZE);
  memcpy(tree_item->TFflag_nonce,  &enc_ORAMTreeItem[phase_10], NONCE_SIZE);
  memcpy(AES_TAG_TFflag,           &enc_ORAMTreeItem[phase_11], MAC_SIZE);
  
  
#ifdef MEASURE_AESdec
  OCALL_chrono_AESdec_start();
#endif


  // Decrypt enc_item->TFflag using AES-GCM.
  status = sgx_rijndael128GCM_decrypt(&AES_SK, enc_item->TFflag, FLAG_SIZE, tree_item->TFflag,
				      tree_item->TFflag_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_TFflag);
  
  
#ifdef MEASURE_AESdec
  OCALL_chrono_AESdec_end();
#endif  

  // If enc_item->TFflag is true, decrypt block, content and LeafID.
  if(tree_item->TFflag[0] == '1'){

    
#ifdef MEASURE_AESdec
    OCALL_chrono_AESdec_start();
#endif

    status = sgx_rijndael128GCM_decrypt(&AES_SK, enc_item->block, MAX_BLOCK_SIZE, tree_item->block,
					tree_item->block_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_block);

    status = sgx_rijndael128GCM_decrypt(&AES_SK, enc_item->content, CONTENT_SIZE, tree_item->content,
					tree_item->content_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_content);

    status = sgx_rijndael128GCM_decrypt(&AES_SK, LeafID_enc, 4, LeafID_bit,
					tree_item->LeafID_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_LeafID);
    ConvertUint8_tToInt(LeafID_bit, tree_item->LeafID[0]);

#ifdef MEASURE_AESdec
    OCALL_chrono_AESdec_end();
#endif

  }



  delete[] LeafID_enc;
  delete[] LeafID_bit;
  
  return;
}


/*
 * Function to save vec_num's ORAM Tree item.
 * <input>
 * [ORAM_Tree*]     tree_item: item of ORAM Tree.
 * [ORAM_Tree*]      enc_item: item of ORAM Tree (for ciphertext).
 * [int]              vec_num: number of ORAM Tree's array.
 * [string]              path: path to ORAM Tree.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void SaveORAMTreeItem(ORAM_Tree *tree_item, ORAM_Tree *enc_item, int vec_num, string path, sgx_key_128bit_t& AES_SK)
{
  string filename_str = path + to_string(vec_num) + ITEM_extension;
  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }
  
  string().swap(filename_str);
  

  // Initilization for AES.
  sgx_status_t status;

  uint8_t* LeafID_bit = new uint8_t[4]();
  uint8_t* LeafID_enc = new uint8_t[4]();


  // Encrypt elements using AES-GCM.
#ifdef MEASURE_AESenc
  OCALL_chrono_AESenc_start();
#endif

  status = sgx_rijndael128GCM_encrypt(&AES_SK, tree_item->block, MAX_BLOCK_SIZE, enc_item->block,
				      tree_item->block_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_block);

  status = sgx_rijndael128GCM_encrypt(&AES_SK, tree_item->content, CONTENT_SIZE, enc_item->content,
				      tree_item->content_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_content);

  ConvertIntToUint8_t(tree_item->LeafID[0], LeafID_bit);  
  status = sgx_rijndael128GCM_encrypt(&AES_SK, LeafID_bit, 4, LeafID_enc,
				      tree_item->LeafID_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_LeafID);
  delete[] LeafID_bit;

  
  status = sgx_rijndael128GCM_encrypt(&AES_SK, tree_item->TFflag, FLAG_SIZE, enc_item->TFflag,
				      tree_item->TFflag_nonce, NONCE_SIZE, NULL, 0, &AES_TAG_TFflag);
#ifdef MEASURE_AESenc
  OCALL_chrono_AESenc_end();
#endif
  
  
  // Combine elements of ORAMTreeItem into one variable.
  int phase_1  = MAX_BLOCK_SIZE;
  int phase_2  = phase_1  + NONCE_SIZE;
  int phase_3  = phase_2  + MAC_SIZE;
  int phase_4  = phase_3  + CONTENT_SIZE;
  int phase_5  = phase_4  + NONCE_SIZE;
  int phase_6  = phase_5  + MAC_SIZE;
  int phase_7  = phase_6  + sizeof(int);
  int phase_8  = phase_7  + NONCE_SIZE;
  int phase_9  = phase_8  + MAC_SIZE;
  int phase_10 = phase_9  + FLAG_SIZE;
  int phase_11 = phase_10 + NONCE_SIZE;
  
  memcpy(&enc_ORAMTreeItem[0],        enc_item->block,          MAX_BLOCK_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_1],  tree_item->block_nonce,   NONCE_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_2],  AES_TAG_block,            MAC_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_3],  enc_item->content,        CONTENT_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_4],  tree_item->content_nonce, NONCE_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_5],  AES_TAG_content,          MAC_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_6],  LeafID_enc,               sizeof(int));
  memcpy(&enc_ORAMTreeItem[phase_7],  tree_item->LeafID_nonce,  NONCE_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_8],  AES_TAG_LeafID,           MAC_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_9],  enc_item->TFflag,         FLAG_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_10], tree_item->TFflag_nonce,  NONCE_SIZE);
  memcpy(&enc_ORAMTreeItem[phase_11], AES_TAG_TFflag,           MAC_SIZE);

  delete[] LeafID_enc;
  
  
#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_start();
#endif

  // Save ciphertext.
  status = OCALL_SaveFile(filename, filename_len, enc_ORAMTreeItem, ORAMTreeItem_len);

#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_end();
#endif

    
  return;
}


// ===========================================================================





// ==================== ORAM Exection Function ====================


/*
 * Function to create dummy ORAM Tree item.
 * < input >
 * [ORAM_Tree*] tree_item.
 * [int]             mode: 1: create all elements (dummy). 2: create only nonce.
 */
void CreateDummyORAMTreeItem(ORAM_Tree *tree_item, int mode)
{
  if(mode != 1 && mode != 2){
    string cerr = "Invalid mode...";
    OCALL_cerror(cerr.c_str());
  }

  
  if(mode == 1){
    memset(tree_item->block,   '0', MAX_BLOCK_SIZE);
    memset(tree_item->content, '0', CONTENT_SIZE);
    memset(tree_item->TFflag,  '0', FLAG_SIZE);
    tree_item->LeafID[0] = -1;
  }


  for(int i=0; i<NONCE_SIZE; i++){      
    tree_item->block_nonce[i]   = (uint8_t)UniformDistribution_int(0, 255);
    tree_item->content_nonce[i] = (uint8_t)UniformDistribution_int(0, 255);
    tree_item->LeafID_nonce[i]  = (uint8_t)UniformDistribution_int(0, 255);
    tree_item->TFflag_nonce[i]  = (uint8_t)UniformDistribution_int(0, 255);
  }
  

  return;
}


int GetFirstPositionFromVCF(uint8_t* data)
{
  int phase    = 1;
  int position = 0;
  string pos_str;
    
  for(int j=0; j<CONTENT_SIZE; j++){
      
    if(phase == 1 && data[j] == '\n')
      phase = 2;
    
    else if(phase == 2 && data[j] == '\t')
      phase = 3;
    
    else if(phase == 3 && data[j] != '\t')
      pos_str.push_back(data[j]);
    
    else if(phase == 3 && data[j] == '\t')
      break;
  }
  
  position = atoi(pos_str.c_str());


  string().swap(pos_str);

  return position;
}


/*
 * Function to get searching content in Stash.
 * <input>
 * [ORAM_Stash*]   stash.
 * [int]      stash_size: stash size.
 * [ORAM_Search&] search: searching file.
 */
void GetDataFromStash(ORAM_Stash* stash, int stash_size, ORAM_Search& search, int position){
  
  // Search Stash to find content.
  for(size_t i=0; i<stash_size; i++){
    // Temporary hold content in ORAM_Search type.
    if(memcmp(search.block, stash[i].block, MAX_BLOCK_SIZE) == 0){
      memcpy(search.content, stash[i].content, CONTENT_SIZE);
      
      break;
    }
  }

  
  // Search specific position in search.content.
  int phase = 1;
  string pos_str;
  bool find_flag = false;
  
  for(int i=0; i<CONTENT_SIZE; i++){
    if(phase == 1 && search.content[i] == '\n')
      phase = 2;
    
    else if(phase == 2 && search.content[i] == '\t')
      phase = 3;
    
    else if(phase == 3 && search.content[i] != '\t')
      pos_str.push_back(search.content[i]);
    
    else if(phase == 3 && search.content[i] == '\t'){
      // OCALL_print(pos_str.c_str());
      
      if(atoi(pos_str.c_str()) == position){
	find_flag = true;
	break;
      }

      phase = 1;
      pos_str.clear();
    }
  }


  if(find_flag == false){
    string cerr = "Position: " + to_string(position) + " does not exist on ORAM Tree...";
    OCALL_cerror(cerr.c_str());
  }



  string().swap(pos_str);
  
  return;
}


/*
 * Function to get searching content in Stash. For Fisher's Exact Test.
 * <input>
 * [ORAM_Stash*]            stash.
 * [int]               stash_size: stash size.
 * [ORAM_Search&]          search: searching file.
 * [vactor<int>] ContingencyTable: contingency table. 1 dimension.
 */
void GetDataFromStash_Fisher(ORAM_Stash* stash, int stash_size, ORAM_Search& search,
			     int chrID, int position, vector<int>& ContingencyTable)
{
  int store_row = 0;
  
  // Search Stash to find content.
  for(size_t i=0; i<stash_size; i++){
    // Temporary hold content in ORAM_Search type.
    if(memcmp(search.block, stash[i].block, MAX_BLOCK_SIZE) == 0){
      break;
    }

    store_row++;
  }
  
  
  // Search specific position in search.content.
  int phase = 1;
  int count = 0;
  string pos_str;
  string data_str;
  string cerr = "";
  bool find_flag = false;
  
  for(int i=0; i<CONTENT_SIZE; i++){
    if(phase == 1 && stash[store_row].content[i] == '\n')
      phase = 2;
    
    else if(phase == 2 && stash[store_row].content[i] == '\t')
      phase = 3;
    
    else if(phase == 3 && stash[store_row].content[i] != '\t')
      pos_str.push_back(stash[store_row].content[i]);
    
    else if(phase == 3 && stash[store_row].content[i] == '\t'){
      // OCALL_print(pos_str.c_str());

      if(atoi(pos_str.c_str()) == position){
	find_flag = true;
	phase = 4;
      }

      pos_str.clear();
    }
    
    else if(phase == 4 && stash[store_row].content[i] != '\t'){
      if(stash[store_row].content[i] == '\n')
	break;
      
      data_str.push_back(stash[store_row].content[i]);
    }

    else if(phase == 4 && stash[store_row].content[i] == '\t'){
      if(chrID == 89){  // chrID == "Y"
	if(data_str == "0"){
	  ContingencyTable[0]++;
	}
	else if(data_str == "1"){
	  ContingencyTable[1]++;
	}
	else{
	  if(count > 6){
	    find_flag = false;
	    
	    cerr = "Unexpexted data(" + data_str + ") exists in position: " + to_string(position);
	    OCALL_print(cerr.c_str());
	  }
	}
      }
      else{
	if(data_str == "0|0")
	  ContingencyTable[2]++;
	
	else if(data_str == "0|1" || data_str == "1|0")
	  ContingencyTable[1]++;
	
	else if(data_str == "1|1")
	  ContingencyTable[0]++;
	
	else{
	  if(count > 6){
	    find_flag = false;
	    
	    cerr = "Unexpexted data(" + data_str + ") exists in position: " + to_string(position);
	    OCALL_print(cerr.c_str());
	  }
	}
      }

      count++;      
      data_str.clear();
    }
  }
  

  if(find_flag == false){
    cerr = "Position: " + to_string(position) + " does not exist on ORAM Tree...";
    OCALL_cerror(cerr.c_str());
  }


  string().swap(cerr);
  string().swap(pos_str);
  string().swap(data_str);
  
  return;
}


/*
 * Function to get searching content in Stash. For LR/PCA.
 * <input>
 * [ORAM_Stash*]   stash.
 * [int]      stash_size: stash size.
 * [ORAM_Search&] search: searching file.
 * [int] chrID, position.
 * [vactor<double>] data: variable for content.
 */
void GetDataFromStash_ML(ORAM_Stash* stash, int stash_size, ORAM_Search& search,
			 int chrID, int position, vector<double>& data)
{
  int store_row = 0;
  
  // Search Stash to find content.
  for(size_t i=0; i<stash_size; i++){
    // Temporary hold content in ORAM_Search type.
    if(memcmp(search.block, stash[i].block, MAX_BLOCK_SIZE) == 0)
      break;

    store_row++;
  }

  
  // Search specific position in search.content.
  int phase = 1;
  int count = 0;
  string pos_str;
  string data_str;
  string cerr = "";
  bool find_flag = false;

  
  for(int i=0; i<CONTENT_SIZE; i++){
    if(phase == 1 && stash[store_row].content[i] == '\n')
      phase = 2;
    
    else if(phase == 2 && stash[store_row].content[i] == '\t')
      phase = 3;
    
    else if(phase == 3 && stash[store_row].content[i] != '\t')
      pos_str.push_back(stash[store_row].content[i]);
    
    else if(phase == 3 && stash[store_row].content[i] == '\t'){
      
      if(atoi(pos_str.c_str()) == position){
	find_flag = true;
	phase = 4;
      }

      pos_str.clear();
    }
    
    else if(phase == 4 && stash[store_row].content[i] != '\t'){
      if(stash[store_row].content[i] == '\n')
	break;

      // If x > 1 in x|x, x = 1. (use data as "exist mutation").
      // CAUTION!!: assume that 0 <= x < 10.
      if((int)'1' < (int)stash[store_row].content[i] && (int)stash[store_row].content[i] < (int)':')
	data_str.push_back('1');

      else
	data_str.push_back(stash[store_row].content[i]);
    }

    else if(phase == 4 && stash[store_row].content[i] == '\t'){
      // OCALL_print(data_str.c_str());

      if(chrID == 89){  // chrID == "Y"
	if(data_str == "0"){
	  data.push_back(0.0);
	}
	else if(data_str == "1"){
	  data.push_back(1.0);
	}
	else{
	  if(count > 6){
	    find_flag = false;
	    
	    cerr = "Unexpexted data(" + data_str + ") exists in position: " + to_string(position);
	    OCALL_print(cerr.c_str());
	  }
	}
      }
      else{
	if(data_str == "0|0")
	  data.push_back(2.0);
      
	else if(data_str == "0|1" || data_str == "1|0")
	  data.push_back(1.0);
	
	else if(data_str == "1|1")
	  data.push_back(0.0);
	
	else{
	  if(count > 6){
	    find_flag = false;
	    
	    cerr = "Unexpexted data(" + data_str + ") exists in position: " + to_string(position);
	    OCALL_print(cerr.c_str());
	  }
	}
      }


      count++;
      data_str.clear();
    }
  }

  
  if(find_flag == false){
    data.clear();
    
    cerr = "[position: " + to_string(position) + "] Data cannnot use in ML, so skipped...";
    OCALL_print(cerr.c_str());
  }


  
  string().swap(cerr);
  string().swap(pos_str);
  string().swap(data_str);
  
  return;
}


/*
 * Function to get block and LeafID from PositionMap using chrID, position and nation.
 * < input >
 * [ORAM_PositionMap] p_map.
 * [int]          pmap_size: p_map size.
 * [ORAM_Search&]    search: variable to hold block and LeafID.
 * [int&]       path_LeafID: LeafID before updating.
 * [int]           L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]              chrID.
 * [int]           position.
 * [string]          nation.
 */
void GetElementsFromPositionMap(ORAM_PositionMap* p_map, int pmap_size, ORAM_Search& search,
				int& path_LeafID, int L_height, int chrID, int position, string nation)
{
  int nearest_diff = MAX_INT;
  int pos_diff     = 0;
  size_t vec_num   = 0;
  bool find_flag   = false;


  for(size_t i=0; i<pmap_size; i++){
    if(p_map[i].chrID == chrID){
      if(p_map[i].nation[0] == nation[0] && p_map[i].nation[1] == nation[1] && p_map[i].nation[2] == nation[2]){
	pos_diff = position - p_map[i].position;
    
	// Temporary hold vec_num if distance between pos_diff and nearest_diff is nearest.
	if(0 <= pos_diff && pos_diff < nearest_diff){
	  nearest_diff = pos_diff;
	  vec_num = i;
	}
      }
    }
  }

  
  // OCALL_print(nation.c_str());
  // OCALL_print_uint8_t(p_map[vec_num].nation, NATION_SIZE);
  // OCALL_print("\n");
  
  // Hold prev LeafID, then update.
  path_LeafID           = p_map[vec_num].LeafID;
  p_map[vec_num].LeafID = UniformDistribution_int(0, pow(2, L_height) - 1);
  

  // Hold elements of PositionMap in ORAM_Search.
  memcpy(search.block, p_map[vec_num].block, MAX_BLOCK_SIZE);
  search.LeafID = p_map[vec_num].LeafID;

  
  return;
}


/*
 * Function to trace ORAM Tree from LeafID to root, and hold true data in Stash.
 * <input>
 * [ORAM_Stash*]        stash: Stash.
 * [int]           stash_size: stash size.
 * [ORAM_Search&]      search: vriable to hold block.
 * [int]               LeafID.
 * [int]             L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]             Z_blocks: num of blocks in each node.
 * [string]              path: path to ORAM Tree.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void GetPathFromTreeToStash(ORAM_Stash* stash, int stash_size, ORAM_Search& search,
			    int LeafID, int L_height, int Z_blocks, string path, sgx_key_128bit_t& AES_SK)
{
#ifdef MEASURE_GetPath
  OCALL_chrono_GetPath_start();
#endif
  
  // Get info to trace ORAM Tree.
  int node_vec_num  = GetLeafVectorNumber(L_height, Z_blocks, LeafID);
  int node_num      = GetLeafNodeNumber(L_height, LeafID);
  int stash_vec_num = stash_size - 1;
  string cout = "";
  
  for(int i=0; i<L_height+1; i++){
    
    for(int j=0; j<Z_blocks; j++){
      LoadORAMTreeItem(tree_item, enc_item, node_vec_num + j, path, AES_SK);
      
      
      // Temporary hold true data in ORAM Tree to Stash.
      if(tree_item->TFflag[0] == '1'){
	// Find vacant Stash. If exist, do following procedure.
	GetVacantStashNumber_descend(stash, stash_size, stash_vec_num);
	
	
	// Store data in stash.
	stash[stash_vec_num].valid[0] = '1';
	memcpy(stash[stash_vec_num].block, tree_item->block, MAX_BLOCK_SIZE);

	stash[stash_vec_num].content = new uint8_t[CONTENT_SIZE]();
	memcpy(stash[stash_vec_num].content, tree_item->content, CONTENT_SIZE);
        
	// Store LeafID (change procedure whether LeafID is changed or not).
	if(memcmp(search.block, tree_item->block, MAX_BLOCK_SIZE) == 0)
	  stash[stash_vec_num].LeafID = search.LeafID;
	else
	  stash[stash_vec_num].LeafID = tree_item->LeafID[0];
      }
    }
    
    // Update node_num and node_vec_num to trace ORAM Tree.
    node_num    /= 2;
    node_vec_num = GetVectorNumberFromNode(node_num, Z_blocks);
  }

#ifdef MEASURE_GetPath
  OCALL_chrono_GetPath_end();
#endif
  
  return;
}


/*
 * Function to trace ORAM Tree from LeafID to root, and rewrite true data in ORAM Tree from Stash.
 * <input>
 * [ORAM_Stash*]        stash: Stash.
 * [int]           stash_size: stash size.
 * [int]               LeafID.
 * [int]             L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]             Z_blocks: num of blocks in each node.
 * [string]              path: path to ORAM Tree.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void WritePathFromStashToTree(ORAM_Stash* stash, int stash_size, 
			      int LeafID, int L_height, int Z_blocks, string path, sgx_key_128bit_t& AES_SK)
{
#ifdef MEASURE_WritePath
  OCALL_chrono_WritePath_start();
#endif
  
  // Get info to trace ORAM Tree.
  int node_vec_num = GetLeafVectorNumber(L_height, Z_blocks, LeafID);
  int node_num     = GetLeafNodeNumber(L_height, LeafID);
  int LeafID_start = 0, LeafID_end = 0;
  bool  stash_flag = true;
  bool vacant_flag = true;
  string cout      = "";


  
  for(int i=0; i<L_height+1; i++){
    LeafID_start = GetLeafID_start(node_num, L_height, i);
    LeafID_end   = GetLeafID_end(node_num, L_height, i);
    stash_flag   = true;
    
    for(int j=0; j<Z_blocks; j++){
      vacant_flag = true;
      
      if(stash_flag){
	for(size_t k=stash_size; k>0; k--){
	  // Search element that can rewrite data in ORAM Tree from Stash.
	  if(LeafID_start <= stash[k - 1].LeafID && stash[k - 1].LeafID <= LeafID_end && stash[k - 1].valid[0] == '1'){

	    // Store data in ORAM Tree.
	    memcpy(tree_item->block,   stash[k - 1].block,   MAX_BLOCK_SIZE);
	    memcpy(tree_item->content, stash[k - 1].content, CONTENT_SIZE);
	    tree_item->LeafID[0] = stash[k - 1].LeafID;
	    tree_item->TFflag[0] = '1';

	    
	    // Delete info in Stash.
	    stash[k - 1].valid[0] = '0';
	    stash[k - 1].LeafID   = -1;
	    memset(stash[k - 1].block, '\0', MAX_BLOCK_SIZE);

	    // free stash.content.
	    delete[] stash[k - 1].content;
	    

	    vacant_flag = false;
	    
	    break;
	  }

	  
	  // If it finish searching Stash, there does not need following search.
	  if(k == 1){
	    stash_flag = false;
	  }
	}
      }

      
      if(vacant_flag == true){  // If it cannot store data from Stash, store dummy data in ORAM Tree.
	CreateDummyORAMTreeItem(tree_item, 1);
      }
      else{                     // If it can store data from Stash, create only seed.
	CreateDummyORAMTreeItem(tree_item, 2);
      }
      
      
      SaveORAMTreeItem(tree_item, enc_item, node_vec_num + j, path, AES_SK);
    }	
    
    // Update node_num and node_vec_num to trace ORAM Tree.
    node_num     /= 2;
    node_vec_num  = GetVectorNumberFromNode(node_num, Z_blocks);
  }


#ifdef MEASURE_WritePath
  OCALL_chrono_WritePath_end();
#endif


  
  string().swap(cout);
  
  return;
}


string GetFilename(int num)
{
  int filename_len  = 0;
  OCALL_GetFilenameLength(&filename_len, num);
  uint8_t* fname = new uint8_t[filename_len + 1]();
  OCALL_GetFilename(fname, filename_len + 1, num);


  string filename_str(filename_len, '\0');
  for(size_t i=0; i<filename_len; i++)
    filename_str[i] = fname[i];

  delete[] fname;
  // OCALL_print(filename_str.c_str());
  
  
  return filename_str;
}


/*
 * Function to create Position Map.
 * <input>
 * [ORAM_PositionMap*]  p_map.
 * [int]            pmap_size: p_map size.
 * [int]             L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 */
void CreatePositionMap(ORAM_PositionMap* p_map, int pmap_size, int L_height)
{  
  // Initialization.
  int max_LeafID   = pow(2, L_height) - 1;
  string filename  = "";
  string chrID_str = "";
  string nation    = "";
  int chrID;  // X: 88, Y: 89 
  

  for(int i=0; i<pmap_size; i++){
    filename = GetFilename(i);
    filename = filename.substr(filename.find_last_of("/") + 1, filename.length());
    // OCALL_print(filename.c_str());

    // Each filename of .vcf data is "chr[chrID]_[nation]_xxx.vcf".
    chrID_str = filename.substr(3, filename.find_first_of("_") - 3);
    nation    = filename.substr(filename.find_first_of("_") + 1, 3);


    assert(chrID_str.length() < 3);  // chrID_str is less than 2 (0~22, X, Y).
    if(chrID_str == "X" || chrID_str == "Y")
      chrID = chrID_str[0];
    else
      chrID = stoi(chrID_str.c_str());
    

    // Store elements in PositionMap.
    // There need decryption to get p_map.position, so get it later(CreateORAMTree()).
    ConvertIntToUint8_t(i, p_map[i].block);
    
    for(int j=0; j<NATION_SIZE; j++)
      p_map[i].nation[j] = nation[j];
    
    p_map[i].LeafID = UniformDistribution_int(0, max_LeafID);
    p_map[i].chrID  = chrID;
  }

  
  return;
}


/*
 * Function to create Stash.
 * <input>
 * [ORAM_Stash*] stash.
 * [int]    stash_size: stash size.
 */
void CreateStash(ORAM_Stash* stash, int stash_size)
{
  for(size_t i=0; i<stash_size; i++){
    stash[i].LeafID   = -1;
    stash[i].valid[0] = '0';
  }


  return;
}


/*
 * Function to create ORAM Tree.
 * <input>
 * [ORAM_PositionMap*]  p_map.
 * [int]            pmap_size: p_map size.
 * [ORAM_Stash*]        stash.
 * [int]           stash_size: stash size.
 * [int]               N_node: node num of ORAM Tree.
 * [int]             L_height: height of ORAM Tree. !! CAUTION !!: L_height start from 0.
 * [int]             Z_blocks: num of blocks in each node.
 * [string]              path: path to ORAM Tree.
 * [sgx_key_128bit_t&] AES_SK: AES secret key.
 */
void CreateORAMTree(ORAM_PositionMap* p_map, int pmap_size, ORAM_Stash* stash, int stash_size,
		    int N_node, int L_height, int Z_blocks, string path, uint8_t* user_aes_key, sgx_key_128bit_t& AES_SK)
{
  sgx_status_t status;

  int stash_vec_num = 0;
  int leaf_node_num = 0;
  int node_vec_num  = 0;
  int node_num      = 0;
  int LeafID        = 0;
  int max_LeafID    = pow(2, L_height) - 1;
  int LeafID_start  = 0;
  int LeafID_end    = 0;

  sgx_key_128bit_t prev_SK;

  string filename_str        = "";
  string half_filename       = "";
  string prev_filepath       = "";
  string prev_AES_filename   = "";
  string prev_nonce_filename = "";
  string prev_MAC_filename   = "";
  string cout                = "";

  // use it in GetPath().
  ORAM_Search search;
  InitializeORAMSearch(search);
   
    
  // Creaate dummy ORAM Tree at first.
  for(int i=0; i<N_node * Z_blocks; i++){
    cout = "Create " + to_string((int)i + 1) + "th ORAM tree...";
    // OCALL_print(cout.c_str());

    
    CreateDummyORAMTreeItem(tree_item, 1);

    SaveORAMTreeItem(tree_item, enc_item, i, path, AES_SK);    
  }
  
  
  for(size_t i=0; i<pmap_size; i++){
    cout = to_string((int)i + 1) + "th position map.";
    // if(i % 10000 == 0)  OCALL_print(cout.c_str());
    
    // Find vacant Stash. If found, do following procedure.
    stash_vec_num = 0;
    GetVacantStashNumber_ascend(stash, stash_size, stash_vec_num);

    	
    // Update Position Map.
    LeafID           = p_map[i].LeafID;
    p_map[i].LeafID  = UniformDistribution_int(0, max_LeafID);
    
    
    // Store data in Stash.
    stash[stash_vec_num].valid[0] = '1';
    stash[stash_vec_num].LeafID   = p_map[i].LeafID;
    memcpy(stash[stash_vec_num].block, p_map[i].block, MAX_BLOCK_SIZE);


    // Get filename of data and seed.
    filename_str  = GetFilename(i);
    half_filename = filename_str.substr(0, filename_str.find_last_of("."));
    prev_nonce_filename = half_filename + ".nonce";
    prev_MAC_filename   = half_filename + ".tag";


    // Load ciphertext and seed.
    status = OCALL_LoadFile_char(filename_str.c_str(),        filename_str.length() + 1,        enc_item->content, CONTENT_SIZE);
    status = OCALL_LoadFile_char(prev_nonce_filename.c_str(), prev_nonce_filename.length() + 1, nonce,             NONCE_SIZE);
    status = OCALL_LoadFile_char(prev_MAC_filename.c_str(),   prev_MAC_filename.length() + 1,   AES_TAG,           MAC_SIZE);
    

    // Load AES_SK.
    for(int j=0; j<AES_KEY_SIZE; j++)
      prev_SK[j] = user_aes_key[j];

    
    // Decrypt data(size: CONTENT_SIZE) and store it in Stash.
    stash[stash_vec_num].content = new uint8_t[CONTENT_SIZE]();
    status = sgx_rijndael128GCM_decrypt(&prev_SK, enc_item->content, CONTENT_SIZE, stash[stash_vec_num].content,
					nonce, NONCE_SIZE, NULL, 0, &AES_TAG);
    

    
    // Store first position in data to PositionMap.
    p_map[i].position = GetFirstPositionFromVCF(stash[stash_vec_num].content);

    
    // Use it in GetPath().
    memcpy(search.block, p_map[i].block, MAX_BLOCK_SIZE);
    search.LeafID = p_map[i].LeafID;
    
    

    // Function to trace ORAM Tree from LeafID to root, and hold true data in Stash. 
    GetPathFromTreeToStash(stash, stash_size, search, LeafID, L_height, Z_blocks, path, AES_SK);
    
    
    // Function to trace ORAM Tree from LeafID to root, and rewrite true data in ORAM Tree from Stash.
    WritePathFromStashToTree(stash, stash_size, LeafID, L_height, Z_blocks, path, AES_SK);
  }


  CleanORAMSearch(search);
  string().swap(filename_str);
  string().swap(half_filename);
  string().swap(prev_filepath);
  string().swap(prev_AES_filename);
  string().swap(prev_nonce_filename);
  string().swap(prev_MAC_filename);
  string().swap(cout);
  
  return;
}


// ================================================================





// ================= AES FUNCTIONS =================


/*
 * Function to create AES SK.
 * < input >
 * [sgx_key_128bit_t] AES_KEY: AES secret key.
 */
void CreateAESSecretKey(sgx_key_128bit_t& AES_KEY)
{  
  for(int i=0; i<AES_KEY_SIZE; i++)
    AES_KEY[i] = (uint8_t)UniformDistribution_int(0, 255);

    
  return;
}


/*
 * Function to load and unseal AES SK.
 * < input >
 * [sgx_key_128bit_t] AES_KEY: AES SK.
 * [string]      filename_str: filename.
 */
void LoadAESSecretKey(sgx_key_128bit_t& AES_KEY, string filename_str)
{
  sgx_status_t status;

  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }
  
  
#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_start();
#endif

  // Get file length.
  int sealed_len = 0;
  status = OCALL_GetFileLength(&sealed_len, filename, filename_len);


  // Load data.
  uint8_t *sealed_data = new uint8_t[sealed_len]();
  status = OCALL_LoadFile(filename, filename_len, sealed_data, sealed_len);
  
#ifdef MEASURE_FileIn
  OCALL_chrono_FileIn_end();
#endif

  // Unseal AES SK.
  // int unsealed_len    = sgx_get_encrypt_txt_len((sgx_sealed_data_t*)sealed_data);
  int unsealed_len       = AES_KEY_SIZE;
  uint8_t *unsealed_data = new uint8_t[unsealed_len]();
  status = sgx_unseal_data((sgx_sealed_data_t*)sealed_data, NULL, 0, unsealed_data, (uint32_t*)&unsealed_len);
  

  // Convert type: uint8_t -> sgx_key_128bit_t
  memcpy(AES_KEY, unsealed_data, AES_KEY_SIZE);
  


  delete[] sealed_data;
  delete[] unsealed_data;
  
  return;
}


/*
 * Function to seal and save AES SK.
 * < input >
 * [sgx_key_128bit_t] AES_KEY: AES secret key.
 * [string]      filename_str: filename.
 */
void SaveAESSecretKey(sgx_key_128bit_t _AES_KEY, string filename_str)
{
  // Convert type: sgx_key_128bit_t -> uint8_t.
  uint8_t* AES_KEY = new uint8_t[AES_KEY_SIZE]();
  memcpy(AES_KEY, _AES_KEY, AES_KEY_SIZE);

  
  size_t filename_len   = filename_str.length() + 1;
  for(size_t j=0; j<filename_len; j++){
    if(j == filename_len - 1)
      filename[j] = '\0';
    else
      filename[j] = filename_str[j];
  }

  
  // Sealing
  // uint16_t key_policy = 0x0001;  // MRENCLAVE
  uint16_t key_policy = 0x0002;  // MRSIGNER
  sgx_status_t status;
  sgx_attributes_t attr;
  sgx_misc_select_t misc = 0xF0000000;
  
  attr.flags = 0xFF0000000000000B;
  attr.xfrm  = 0;

  
  uint32_t sealed_len     = sgx_calc_sealed_data_size(0, AES_KEY_SIZE);
  uint8_t* AES_KEY_sealed = new uint8_t[sealed_len]();
  
  status = sgx_seal_data_ex(key_policy, attr, misc, 0, NULL,
			    AES_KEY_SIZE, AES_KEY, sealed_len, (sgx_sealed_data_t*)AES_KEY_sealed);
  
  
#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_start();
#endif

  // Save data.
  status = OCALL_SaveFile(filename, filename_len, AES_KEY_sealed, sealed_len);

#ifdef MEASURE_FileOut
  OCALL_chrono_FileOut_end();
#endif

  
  delete[] AES_KEY;
  delete[] AES_KEY_sealed;
  
  return;
}


// =================================================





// ================ ORAM FUNCTION ================


void ORAM_initialization(int Z_blocks, string& in_topdir, string& in_dirname,
			 string& out_topdir, string& out_dirname, uint8_t* user_aes_key)
{
  // Check whether table_path exists or not. If it does not exist, create it.
  string table_path = out_topdir + out_dirname + "/";
  OCALL_DirectoryCheck(table_path.c_str(), table_path.length() + 1);

  
  // Initialization.
  int data_num;
  string extension = ".vcf";
  string path = in_topdir + in_dirname + "/";
  OCALL_GetFileNumber(&data_num, path.c_str(), path.length() + 1, extension.c_str(), extension.length() + 1);
  int N_node     = NextPow2(data_num) - 1;     // node num of ORAM Tree.
  int L_height   = log2(N_node + 1) - 1;       // height of ORAM Tree.
  int stash_size = (L_height + 1) * Z_blocks;  // stash size.
  
  string p_map_file    = "PositionMap";
  string AES_keyfile   = table_path + "AES_SK.key.sealed";
  string metadata_file = table_path + "metadata";
  string cout        = "";
  
  sgx_key_128bit_t AES_SK;
  ORAM_PositionMap p_map[data_num] = {};
  ORAM_Stash stash[stash_size] = {};
  

  /*
  cout = GREEN_START + "Init ORAM Tree: " + table_path + COLOR_END;
  OCALL_print(cout.c_str());
  cout = "Data Num: " + to_string(data_num) + ", Node: " + to_string(N_node) +
              ", Z: " + to_string(Z_blocks) +    ", L: " + to_string(L_height);
  OCALL_print(cout.c_str());
  */
  
  
  // Initialize variables.
  InitializeGlobalVariable();
  InitializePositionMap(p_map, data_num);
  InitializeStash(stash, stash_size);

  
  
  // Create AES secret key.
  CreateAESSecretKey(AES_SK);


   
  // Create PositionMap.
  CreatePositionMap(p_map, data_num, L_height);
  


  
  // Create Stash.
  CreateStash(stash, stash_size);



  
  // Create ORAM Tree from PositionMap.
  CreateORAMTree(p_map, data_num, stash, stash_size, N_node, L_height, Z_blocks, table_path, user_aes_key, AES_SK);



  // Store tables and AES SK.
  cout = "SUCCEEDED: Create (0)AES Secretkey,  (1)Position Map, (2)Stash Table, and (3)ORAM Tree...";
  // OCALL_print(cout.c_str());
  
  SavePositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  SaveStash(table_path, stash, stash_size, AES_SK);
  SaveAESSecretKey(AES_SK, AES_keyfile);
  OCALL_SaveMetadata(Z_blocks, data_num, metadata_file.c_str(), metadata_file.length() + 1);
  

  
  CleanGlobalVariable();
  CleanPositionMap(p_map, data_num);
  CleanStash(stash, stash_size);
  string().swap(table_path);
  string().swap(p_map_file);
  string().swap(AES_keyfile);
  string().swap(metadata_file);
  string().swap(cout);
}


void ORAM_FileSearch(string dirname, int chrID, string nation, int position)
{
  OCALL_chrono_Total_start();
  
  
  // Initialization.
  string topdir        = "/mnt/sdc/iwata/ORAM_table/";
  string table_path    = topdir + dirname + "/";  
  string p_map_file    = "PositionMap";
  string AES_keyfile   = table_path + "AES_SK.key.sealed";
  string metadata_file = table_path + "metadata";
  string cout          = "";

  int Z_blocks;
  int data_num;
  OCALL_LoadMetadata(&Z_blocks, &data_num, metadata_file.c_str(), metadata_file.length() + 1);

  int N_node     = NextPow2(data_num) - 1;     // node num of ORAM Tree.
  int L_height   = log2(N_node + 1) - 1;       // height of ORAM Tree.
  int stash_size = (L_height + 1) * Z_blocks;  // stash size.
  
  int path_LeafID;
  ORAM_Search search;
  
  sgx_key_128bit_t AES_SK;
  ORAM_PositionMap p_map[data_num] = {};
  ORAM_Stash stash[stash_size]     = {};
  
  
  cout = "Data Num: " + to_string(data_num) + ", Node: " + to_string(N_node) +
              ", Z: " + to_string(Z_blocks) +    ", L: " + to_string(L_height);
  OCALL_print(cout.c_str());


  
  // Initialize variables.
  InitializeGlobalVariable();
  InitializePositionMap(p_map, data_num);
  InitializeStash(stash, stash_size);
  InitializeORAMSearch(search);


  
  // Load AES SK.
  cout = BLUE_START + "***** Load AES Secretkey *****" + COLOR_END;
  OCALL_print(cout.c_str());
  LoadAESSecretKey(AES_SK, AES_keyfile);
  

  // Load Position Map.
  cout = BLUE_START + "***** Load Position Map *****" + COLOR_END;
  OCALL_print(cout.c_str());
  
  LoadPositionMap(table_path, p_map_file, p_map, data_num, AES_SK);


  // Load Stash.
  cout = BLUE_START + "***** Load Stash *****" + COLOR_END;
  OCALL_print(cout.c_str());
  
  LoadStash(table_path, stash, stash_size, AES_SK);
  
  
  cout = "End Initialization...";
  OCALL_print(cout.c_str());
  


  // Find file that correspond to chrID, position and nation.
  // In this function, it update PositionMap(LeafID).
  cout = BLUE_START + "***** Search Block from Position Map *****" + COLOR_END;
  OCALL_print(cout.c_str());
  

  GetElementsFromPositionMap(p_map, data_num, search, path_LeafID, L_height, chrID, position, nation);
  
  cout = "[Search] chrID: "  + to_string(chrID) + ", position: "       + to_string(position) + ", nation: " + nation
       + " -> Search File: " + GREEN_START      + (char*)search.block + COLOR_END;
  OCALL_print(cout.c_str());
 
  cout = "File exists the path of " + GREEN_START + "Leaf " + to_string(search.LeafID) + COLOR_END;
  OCALL_print(cout.c_str());
  


  cout = BLUE_START + "***** Check ORAM Tree Path and Save to Stash *****" + COLOR_END + "\nStart reading path of ORAM tree...";
  OCALL_print(cout.c_str());

  
  GetPathFromTreeToStash(stash, stash_size, search, path_LeafID, L_height, Z_blocks, table_path, AES_SK);


  GetDataFromStash(stash, stash_size, search, position);

  OCALL_print_uint8_t(search.content, CONTENT_SIZE);

  
  cout = "End reading path..." + GREEN_START;
  OCALL_print(cout.c_str());
  
  



  cout = BLUE_START + "***** Rewrite ORAM Tree Path *****" + COLOR_END;
  OCALL_print(cout.c_str());


  WritePathFromStashToTree(stash, stash_size, path_LeafID, L_height, Z_blocks, table_path, AES_SK);
  



  
  cout = "Save position map and stash...";
  OCALL_print(cout.c_str());

  SavePositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  SaveStash(table_path, stash, stash_size, AES_SK);
  


  CleanGlobalVariable();
  CleanPositionMap(p_map, data_num);
  CleanStash(stash, stash_size);
  CleanORAMSearch(search);
  string().swap(table_path);
  string().swap(p_map_file);
  string().swap(AES_keyfile);
  string().swap(metadata_file);
  string().swap(cout);
  string().swap(nation);
  

  OCALL_chrono_Total_end();

  OCALL_ShowMeasuredTime();
}


void ORAM_FileSearch_ML(int data_num, int Z_blocks, int chrID, string nation,
			string table_path, int position, vector<double>& data){
  // Initialization.
  string p_map_file  = "PositionMap";
  string AES_keyfile = table_path + "AES_SK.key.sealed";
  string cout        = "";

  int N_node     = NextPow2(data_num) - 1;     // node num of ORAM Tree.
  int L_height   = log2(N_node + 1) - 1;       // height of ORAM Tree.
  int stash_size = (L_height + 1) * Z_blocks;  // stash size.
  
  int path_LeafID;
  ORAM_Search search;
  
  sgx_key_128bit_t AES_SK;
  ORAM_PositionMap p_map[data_num] = {};
  ORAM_Stash stash[stash_size]     = {};
  


  /*
   *  1. Initialization.
   */
  InitializeGlobalVariable();
  InitializePositionMap(p_map, data_num);
  InitializeStash(stash, stash_size);
  InitializeORAMSearch(search);


  LoadAESSecretKey(AES_SK, AES_keyfile);
  LoadPositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  LoadStash(table_path, stash, stash_size, AES_SK);


  
  /*
   * 2. Read/Update Position Map.
   */
  GetElementsFromPositionMap(p_map, data_num, search, path_LeafID, L_height, chrID, position, nation);
  


  /*
   * 3. Temporary store data in Stash.
   */
  GetPathFromTreeToStash(stash, stash_size, search, path_LeafID, L_height, Z_blocks, table_path, AES_SK);
  GetDataFromStash_ML(stash, stash_size, search, chrID, position, data);
  


  /*
   * 4. Rewrite data from Stash to ORAM Tree.
   */
  WritePathFromStashToTree(stash, stash_size, path_LeafID, L_height, Z_blocks, table_path, AES_SK);
  

  SavePositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  SaveStash(table_path, stash, stash_size, AES_SK);

  

  CleanGlobalVariable();
  CleanPositionMap(p_map, data_num);
  CleanStash(stash, stash_size);
  CleanORAMSearch(search);
  string().swap(table_path);
  string().swap(p_map_file);
  string().swap(AES_keyfile);
  string().swap(cout);
  string().swap(nation);
}


void ORAM_GetData_Fisher(int chrID, string nation, string topdir, string dirname,
			 int position, vector<int>& ContingencyTable)
{
  // Initialization.
  string table_path    = topdir + dirname + "/";  
  string p_map_file    = "PositionMap";
  string AES_keyfile   = table_path + "AES_SK.key.sealed";
  string metadata_file = table_path + "metadata";
  string cout          = "";

  int Z_blocks;
  int data_num;
  OCALL_LoadMetadata(&Z_blocks, &data_num, metadata_file.c_str(), metadata_file.length() + 1);

  int N_node     = NextPow2(data_num) - 1;     // node num of ORAM Tree.
  int L_height   = log2(N_node + 1) - 1;       // height of ORAM Tree.
  int stash_size = (L_height + 1) * Z_blocks;  // stash size.
  
  int path_LeafID;
  ORAM_Search search;
  
  sgx_key_128bit_t AES_SK;
  ORAM_PositionMap p_map[data_num] = {};
  ORAM_Stash stash[stash_size]     = {};

  

  /*
   *  1. Initialization.
   */
  InitializeGlobalVariable();
  InitializePositionMap(p_map, data_num);
  InitializeStash(stash, stash_size);
  InitializeORAMSearch(search);

  LoadAESSecretKey(AES_SK, AES_keyfile);
  LoadPositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  LoadStash(table_path, stash, stash_size, AES_SK);



  /*
   * 2. Read/Update Position Map.
   */
  GetElementsFromPositionMap(p_map, data_num, search, path_LeafID, L_height, chrID, position, nation);
  



  /*
   * 3. Temporary store data in Stash.
   */
  GetPathFromTreeToStash(stash, stash_size, search, path_LeafID, L_height, Z_blocks, table_path, AES_SK);
  GetDataFromStash_Fisher(stash, stash_size, search, chrID, position, ContingencyTable);

  

  /*
   * 4. Rewrite data from Stash to ORAM Tree.
   */
  WritePathFromStashToTree(stash, stash_size, path_LeafID, L_height, Z_blocks, table_path, AES_SK);
  

  
  SavePositionMap(table_path, p_map_file, p_map, data_num, AES_SK);
  SaveStash(table_path, stash, stash_size, AES_SK);



  CleanGlobalVariable();
  CleanPositionMap(p_map, data_num);
  CleanStash(stash, stash_size);
  CleanORAMSearch(search);
  string().swap(table_path);
  string().swap(p_map_file);
  string().swap(AES_keyfile);
  string().swap(metadata_file);
  string().swap(cout);
  string().swap(nation);
}


void ORAM_GetData_LR(int chrID, string nation_1, string dirname_1, string nation_2, string dirname_2,
		     string topdir, vector<int>& position, vector< vector<double> >& x, vector<int>& y)
{
  // Initialization.
  vector<double> x_nation1;
  vector<double> x_nation2;
  bool first_flag = true;
  int count = 0;
  // string topdir = "/mnt/sdc/iwata/ORAM_table/";
  string cout = "";

  int Z_blocks_1;
  int data_num_1;
  
  string table_path_1 = topdir + dirname_1 + "/";  
  string metadata_1   = table_path_1 + "metadata";
  OCALL_LoadMetadata(&Z_blocks_1, &data_num_1, metadata_1.c_str(), metadata_1.length() + 1);

  int Z_blocks_2;
  int data_num_2;
  string table_path_2 = topdir + dirname_2 + "/";  
  string metadata_2   = table_path_2 + "metadata";
  OCALL_LoadMetadata(&Z_blocks_2, &data_num_2, metadata_2.c_str(), metadata_2.length() + 1);
  
    
  
  for(size_t i=0; i<position.size(); i++){
    if(i == 0){
      OCALL_chrono_start();
    }

    
    ORAM_FileSearch_ML(data_num_1, Z_blocks_1, chrID, nation_1, table_path_1, position[i], x_nation1);
    ORAM_FileSearch_ML(data_num_2, Z_blocks_2, chrID, nation_2, table_path_2, position[i], x_nation2);

    
    if(x_nation1.empty()){
      cout = "Unexpexted data exists in nation: " + nation_1 + ", position: " + to_string(position[i]);
      OCALL_print(cout.c_str());

      OCALL_cerror("For now, aborted...");
    }
    else if(x_nation2.empty()){
      cout = "Unexpexted data exists in nation: " + nation_2 + ", position: " + to_string(position[i]);
      OCALL_print(cout.c_str());

      OCALL_cerror("For now, aborted...");
    }
    else{
      count++;

      if(first_flag){
	first_flag = false;
	x.push_back(vector<double>());
	
	for(size_t j=0; j<x_nation1.size(); j++){
	  x[0].push_back(1.0);
	  y.push_back(0);
	}

        for(size_t j=0; j<x_nation2.size(); j++){
	  x[0].push_back(1.0);
	  y.push_back(1);
	}
      }

      x.push_back(vector<double>());
      for(size_t j=0; j<x_nation1.size(); j++)  x[count].push_back(x_nation1[j]);
      for(size_t j=0; j<x_nation2.size(); j++)  x[count].push_back(x_nation2[j]);
    }
    
    x_nation1.clear();
    x_nation2.clear();


    if(i == 0){
      double elapsed;  // unit: ms.
      OCALL_chrono_end(&elapsed);

      
      int apx_time = 0;
      apx_time = (int)(elapsed / 1000 * position.size()) * 2;

      if(apx_time == 0){
	apx_time = 3;
      }

      
      string msg = "STDIO\n[BiORAM.ML.LogisticRegression]: In this condition, it takes about " + to_string(apx_time) + " seconds...\n";
      sgx_status_t status = SendMessage(msg.c_str(), msg.length());
    }
  }


  string().swap(cout);
  string().swap(table_path_1);
  string().swap(table_path_2);
  string().swap(metadata_1);
  string().swap(metadata_2);
  vector<double>().swap(x_nation1);
  vector<double>().swap(x_nation2);
}


void ORAM_GetData_PCA(int chrID, string nation, string topdir, string dirname,
		      vector<int>& position, vector< vector<double> >& x)
{
  // Initialization.
  vector<double> x_item;

  int Z_blocks;
  int data_num;
  string cout = "";
  string table_path    = topdir + dirname + "/";  
  string metadata_file = table_path + "metadata";
  OCALL_LoadMetadata(&Z_blocks, &data_num, metadata_file.c_str(), metadata_file.length() + 1);
  
  
  for(size_t i=0; i<position.size(); i++){
    if(i == 0){
      OCALL_chrono_start();
    }

    ORAM_FileSearch_ML(data_num, Z_blocks, chrID, nation, table_path, position[i], x_item);
    
    
    if(!x_item.empty())
      x.push_back(x_item);

    else
      OCALL_cerror("For now, aborted...");
    
    x_item.clear();


    if(i == 0){
      double elapsed;  // unit: ms.
      OCALL_chrono_end(&elapsed);

      
      int apx_time = 0;
      apx_time = (int)(elapsed / 1000 * position.size()) * 2;

      if(apx_time == 0){
	apx_time = 3;
      }

      
      string msg = "STDIO\n[BiORAM.ML.PCA]: In this condition, it takes about " + to_string(apx_time) + " seconds...\n";
      sgx_status_t status = SendMessage(msg.c_str(), msg.length());
    }
  }


  vector<double>().swap(x_item);
  string().swap(cout);
}



// ===============================================
