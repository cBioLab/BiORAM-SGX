/*!
 *
 * Oblivious_RAM.hpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#ifndef _ObliviousRAM_SGX_hpp
#define ObliviousRAM_SGX_hpp

#pragma once

#include "Enclave_t.h"
#include <sgx_attributes.h>
#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <sgx_tseal.h>

#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "SGX_Math.hpp"

using namespace std;


// #define MEASURE_FileIn
// #define MEASURE_FileOut
// #define MEASURE_AESenc
// #define MEASURE_AESdec
// #define MEASURE_GetPath
// #define MEASURE_WritePath

#define MAX_INT        2147483647
#define MAX_BLOCK_SIZE 4
#define CONTENT_SIZE   102000
#define FLAG_SIZE      1
#define NONCE_SIZE     12
#define MAC_SIZE       16
#define AES_KEY_SIZE   16
#define NATION_SIZE    3


const string GREEN_START  = "\033[32m";
const string YELLOW_START = "\033[33m";
const string BLUE_START   = "\033[34m";
const string COLOR_END    = "\033[m";

const string ITEM_extension  = ".oramtree";
const string TABLE_extension = ".table";
const string NONCE_extension = ".nonce";
const string MAC_extension   = ".mac";

const int ORAMTreeItem_len = MAX_BLOCK_SIZE + CONTENT_SIZE + sizeof(int) + FLAG_SIZE
                           + 4 * NONCE_SIZE + 4 * MAC_SIZE;



typedef struct _ORAM_PositionMap{
  uint8_t* block;
  int      LeafID;
  int      chrID;
  int      position;
  uint8_t* nation;
} ORAM_PositionMap;


typedef struct _ORAM_Tree{
  uint8_t *block;
  uint8_t *block_nonce;
  uint8_t *content;
  uint8_t *content_nonce;
  int     *LeafID;
  uint8_t *LeafID_nonce;
  uint8_t *TFflag;         // 0: false, 1: true.
  uint8_t *TFflag_nonce;
} ORAM_Tree;


typedef struct _ORAM_Stash{
  uint8_t* block;
  int      LeafID;
  uint8_t* valid;
  uint8_t* content;
} ORAM_Stash;


typedef struct _ORAM_Search{
  uint8_t* block;
  int      LeafID;
  uint8_t* content;
} ORAM_Search;



void ORAM_initialization(int Z_blocks, string& in_topdir, string& in_dirname,
			 string& out_topdir, string& out_dirname, uint8_t* user_aes_key);

void ORAM_FileSearch(string dirname, int chrID, string nation, int position);

void ORAM_GetData_Fisher(int chrID, string nation, string topdir, string dirname,
			 int position, vector<int>& ContingencyTable);

void ORAM_GetData_LR(int chrID, string nation_1, string dirname_1, string nation_2, string dirname_2,
		     string topdir, vector<int>& position, vector< vector<double> >& x, vector<int>& y);

void ORAM_GetData_PCA(int chrID, string nation, string topdir, string dirname,
		      vector<int>& position, vector< vector<double> >& x);


#endif
