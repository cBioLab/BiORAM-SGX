/*!
 *
 * App.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 * Some function are released from Intel Corporation.
 * LIENSE: https://github.com/intel/linux-sgx/blob/master/License.txt
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <dirent.h>

using namespace std;


/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
  {
    SGX_ERROR_UNEXPECTED,
    "Unexpected error occurred.",
    NULL
  },
  {
    SGX_ERROR_INVALID_PARAMETER,
    "Invalid parameter.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_MEMORY,
    "Out of memory.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_LOST,
    "Power transition occurred.",
    "Please refer to the sample \"PowerTransition\" for details."
  },
  {
    SGX_ERROR_INVALID_ENCLAVE,
    "Invalid enclave image.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ENCLAVE_ID,
    "Invalid enclave identification.",
    NULL
  },
  {
    SGX_ERROR_INVALID_SIGNATURE,
    "Invalid enclave signature.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_EPC,
    "Out of EPC memory.",
    NULL
  },
  {
    SGX_ERROR_NO_DEVICE,
    "Invalid SGX device.",
    "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
  },
  {
    SGX_ERROR_MEMORY_MAP_CONFLICT,
    "Memory map conflicted.",
    NULL
  },
  {
    SGX_ERROR_INVALID_METADATA,
    "Invalid enclave metadata.",
    NULL
  },
  {
    SGX_ERROR_DEVICE_BUSY,
    "SGX device was busy.",
    NULL
  },
  {
    SGX_ERROR_INVALID_VERSION,
    "Enclave version was invalid.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ATTRIBUTE,
    "Enclave was not authorized.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_FILE_ACCESS,
    "Can't open enclave file.",
    NULL
  },
};

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

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  
  /* Call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    return -1;
  }
  
  return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
  /* Proxy/Bridge will check the length and null-terminate 
   * the input string to prevent buffer overflow. 
   */
  printf("%s", str);
}

// ==============================================================================


void OCALL_print(const char* str){
  cout << "[OCALL] ";
  cout << str << endl;
	
  return;
}


void OCALL_print_uint8_t(uint8_t* str, size_t len){
  cout << "[OCALL] ";

  for(size_t i=0; i<len; i++)
    cout << str[i];

  cout << endl;
  

  return;
}


// ディレクトリ内のファイル数を出力する関数．
void GetFileNum(const char* path, string extension, vector<string>& filepaths){
  DIR *dp;       // ディレクトリへのポインタ
  dirent* entry; // readdir() で返されるエントリーポイント


  dp = opendir(path);
  if(dp==NULL){
    cerr << "file do not exist..." << endl;
    exit(1);
  }


  while((entry = readdir(dp))){
    string filename = entry->d_name;

    if(entry != NULL && filename.find(extension) != string::npos)
      filepaths.push_back(path + filename);
  }
  // cout << file_num << endl;

  closedir(dp);


  // ファイルが昇順で取得できないので，ソートする．
  sort(filepaths.begin(), filepaths.end());

  return;
}


// ファイルを読み出す関数．
void OCALL_LoadFile(const char* filename, size_t filename_len, uint8_t* data, size_t data_len){

  ifstream ifs(filename, ios::binary);

  if(!ifs){
    cerr << "[OCALL_LoadFile] file cannot open..." << endl;
    exit(-1);
  }


  ifs.read((char*)data, data_len);
  
  ifs.close();
}


// データをファイルに新規で書き込む関数．
void OCALL_SaveFile(const char* filename, size_t filename_len, uint8_t* data, size_t data_len){
  ofstream ofs(filename, ios::binary);

  if(!ofs){
    cerr << "[OCALL_SaveFile] file cannot open..." << endl;
    exit(-1);
  }

  
  ofs.write((const char*)data, data_len);

  ofs.close();
}






/* Application entry */
int SGX_CDECL main(int argc, char *argv[]){
  if(argc != 5){
    cerr << "ERROR: Exec operation is not correct." << endl;
    cerr << "USAGE: ./app [vcf filepath] [chrID] [nation] [split size]" << endl;
    return -1;
  }
  
  
  /* Initialize the enclave */
  if(initialize_enclave() < 0){
    printf("Enter a character before exit ...\n");
    getchar();
    return -1; 
  }

  /*
  // Utilize edger8r attributes
  edger8r_array_attributes();
  edger8r_pointer_attributes();
  edger8r_type_attributes();
  edger8r_function_attributes();
  
  // Utilize trusted libraries
  ecall_libc_functions();
  ecall_libcxx_functions();
  ecall_thread_functions();
  */



  // Initialization.
  sgx_status_t status;
  
  string dir_path  = argv[1];
  string chrID     = argv[2];
  string nation    = argv[3];
  int FILE_SIZE    = atoi(argv[4]);
  string filepath  = "";
  string extension = ".vcf";
  vector<string> filepath_vec;

  string AES_filename = "chr" + chrID + "_" + nation + ".key";
  // cout << "directiry path: " << dir_path << endl;


  
  // 暗号化したいファイル群のファイル名を取得する．
  GetFileNum(dir_path.c_str(), extension, filepath_vec);

  for(size_t i=0; i<filepath_vec.size(); i++)
    filepath += filepath_vec[i] + ",";

  
  // 暗号化処理を行う．
  status = ECALL_VCFEncryption(global_eid, dir_path.c_str(), filepath.c_str(), AES_filename.c_str(), FILE_SIZE);
  


  
  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid);

  printf("\n\nInfo: SampleEnclave successfully returned.\n");

  return 0;
}

