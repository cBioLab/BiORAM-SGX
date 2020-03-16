/*!
 *
 * SGX_Fileio.hpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 */


#ifndef _SGX_Fileio_hpp
#define SGX_Fileio_hpp

#pragma once

#include "Enclave_u.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;



int OCALL_GetFileLength(uint8_t *filename, size_t filename_len);

void OCALL_LoadFile(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len);

void OCALL_LoadFile_char(const char* filename, size_t filename_len, uint8_t *data, size_t data_len);

void OCALL_LoadMetadata(int *Z_blocks, int *data_num, const char* filename, size_t filesize_len);

void OCALL_SaveFile(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len);

void OCALL_SaveFile_add(uint8_t *filename, size_t filename_len, uint8_t *data, size_t data_len);

void OCALL_SaveMetadata(int Z_blocks, int data_num, const char* filename, size_t filename_len);

int getFileNum(const char* path, string extension, vector<string>& filepaths);

void OCALL_DirectoryCheck(const char* filepath, size_t filepath_len);

int OCALL_GetLengthOfFilenames(const char* path, size_t path_len, const char* extension, size_t extension_len);

void OCALL_GetFilenames(uint8_t* filenames, size_t files_len, const char* path, size_t path_len,
                        const char* extension, size_t extension_len);

int OCALL_GetFileNumber(const char* path, size_t path_len, const char* extension, size_t extension_len);

int OCALL_GetFilenameLength(int num);

void OCALL_GetFilename(uint8_t* filename, size_t filename_len, int num);



#endif
