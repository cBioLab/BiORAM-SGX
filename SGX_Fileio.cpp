/*!
 *
 * SGX_Fileio.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 * 
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 */


#include "SGX_Fileio.hpp"


vector<string> filepaths;

/*
 * Function to get number of files in directory. 
 * < input >
 * [const char*]    path: directory path.
 * [string]    extension: file extension to count up.
 * [vector<string>] file: vector to save filenames (using crating PositionMap).
 *
 * < output >
 * [int]: number of files that exist in [path] directory.
 */
int getFileNum(const char* path, string extension, vector<string>& file)
{
  DIR *dp;
  dirent* entry;


  dp = opendir(path);
  if(dp==NULL){
    cerr << "[getFileNum] File do not exist..." << endl;
    exit(1);
  }


  while((entry = readdir(dp))){
    string filename = entry->d_name;

    if(entry != NULL && filename.find(extension) != string::npos)
      file.push_back(path + filename);
  }
  // cout << file_num << endl;

  closedir(dp);


  // sort filename.
  sort(file.begin(), file.end());

  return (int)file.size();
}


// Function to create directory if it does not exist.
void OCALL_DirectoryCheck(const char* filepath, size_t filepath_len)
{
  struct stat statbuf;


  if(stat(filepath, &statbuf) != 0)
    mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
  
  return;
};


int OCALL_GetLengthOfFilenames(const char* path, size_t path_len, const char* extension, size_t extension_len)
{
  string files = "";
  int data_num = getFileNum(path, extension, filepaths);

  
  for(size_t i=0; i<data_num; i++)
      files += filepaths[i] + ",";

  
  return files.length() + 1;
}


void OCALL_GetFilenames(uint8_t* filenames, size_t files_len, const char* path, size_t path_len,
			const char* extension, size_t extension_len)
{
  string files = "";
  

  for(size_t i=0; i<filepaths.size(); i++)
    files += filepaths[i] + ",";

  for(size_t i=0; i<files_len; i++)
    filenames[i] = files[i];


  return;
}


int OCALL_GetFileNumber(const char* path, size_t path_len, const char* extension, size_t extension_len)
{
  int data_num = getFileNum(path, extension, filepaths);
  
  return data_num;
}



int OCALL_GetFilenameLength(int num)
{
  return filepaths[num].length();
}


void OCALL_GetFilename(uint8_t* filename, size_t filename_len, int num)
{
  for(size_t i=0; i<filepaths[num].length(); i++)
    filename[i] = filepaths[num][i];

  
  return;
}


/*
 * Function to get length of file content.
 * < input >
 * [uint8_t]   *filename: filename.
 * [size_t] filename_len: length of filename.
 *
 * < output >
 * [int]: length of file content.
 */
int OCALL_GetFileLength(uint8_t *filename, size_t filename_len)
{
  string filename_str(filename_len, '\0');
  for(size_t i=0; i<filename_len; i++)
    filename_str[i] = filename[i];

  
  ifstream ifs(filename_str, ios::binary);

  if(!ifs){
    cerr << "ERROR: [OCALL_GetFileLength] Failed to open data..." << endl;
    cerr << "filename: " << filename_str << endl;
    exit(-1);
  }

  ifs.seekg(0, ios::end);
  int sealed_len = ifs.tellg();
  ifs.seekg(0, ios::beg);


  ifs.close();

  return sealed_len;
}


void OCALL_DeleteFilepaths()
{
  filepaths.resize(0);
}


/*
 * Function to get file.
 * < input >
 * [uint8_t]   *filename: filename.
 * [size_t] filename_len: length of filename.
 * [uint8_t]       *data: variable to hold file content.
 * [size_t]     data_len: length of file content. get variable using "OCALL_GetFileLength()" previously.
 */
void OCALL_LoadFile(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len)
{
  string filename_str(filename_len, '\0');
  for(size_t i=0; i<filename_len; i++)
    filename_str[i] = filename[i];
  

  ifstream ifs(filename_str, ios::binary);

  if(!ifs){
    cerr << "ERROR: [OCALL_LoadFile] file cannot open..." << endl;
    cerr << "filename: " << filename_str << endl;
    exit(-1);
  }


  ifs.read((char*)data, data_len);
  
  ifs.close();
}


/*
 * Function to get file.
 * < input >
 * [const char]   *filename: filename.
 * [size_t]    filename_len: length of filename.
 * [uint8_t]          *data: variable to hold file content.
 * [size_t]        data_len: length of file content. get variable using "OCALL_GetFileLength()" previously.
 */
void OCALL_LoadFile_char(const char* filename, size_t filename_len, uint8_t *data, size_t data_len)
{
  ifstream ifs(filename, ios::binary);

  if(!ifs){
    cerr << "ERROR: [OCALL_LoadFile_char] file cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    exit(-1);
  }


  ifs.read((char*)data, data_len);
  
  ifs.close();
}


/*
 * Function to write file.
 * < input >
 * [uint8_t]   *filename: filename.
 * [size_t] filename_len: length of filename.
 * [uint8_t]       *data: file content.
 * [size_t]     data_len: length of file content.
 */
void OCALL_SaveFile(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len)
{
  string filename_str(filename_len, '\0');
  for(size_t i=0; i<filename_len; i++)
    filename_str[i] = filename[i];

  
  ofstream ofs(filename_str, ios::binary);

  if(!ofs){
    cerr << "ERROR: [OCALL_SaveFile] file cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    exit(-1);
  }

  
  ofs.write((const char*)data, data_len);

  ofs.close();
}


/*
 * Function to overwrite file.
 * < input >
 * [uint8_t]   *filename: filename.
 * [size_t] filename_len: length of filename.
 * [uint8_t]       *data: file content.
 * [size_t]     data_len: length of file content. 
 */
void OCALL_SaveFile_add(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len)
{
  string filename_str(filename_len, '\0');
  for(size_t i=0; i<filename_len; i++)
    filename_str[i] = filename[i];

  
  ofstream ofs(filename_str, ios::app | ios::binary);

  if(!ofs){
    cerr << "ERROR: [OCALL_SaveFile_add] file cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    exit(-1);
  }

  
  ofs.write((const char*)data, data_len);

  ofs.close();
}


void OCALL_SaveMetadata(int Z_blocks, int data_num, const char* filename, size_t filesize_len)
{
  ofstream ofs(filename, ios::out | ios::binary);

  if(!ofs){
    cerr << "ERROR: [OCALL_SaveMetadata] file cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    exit(-1);
  }


  ofs << Z_blocks << endl;
  ofs << data_num << endl;

  ofs.close();
}


void OCALL_LoadMetadata(int *Z_blocks, int *data_num, const char* filename, size_t filesize_len)
{
  int Z = 0, num = 0;
  
  ifstream ifs(filename, ios::in | ios::binary);

  if(!ifs){
    cerr << "ERROR: [OCALL_LoadMetadata] file cannot open..." << endl;
    cerr << "filename: " << filename << endl;
    exit(-1);
  }


  ifs >> Z;
  ifs >> num;

  
  *Z_blocks = Z;
  *data_num = num;

  
  ifs.close();
}
