#!/usr/bin/env python
# -*- coding: utf-8 -*-

#*
#* SplitVCFData_size.py
#*
#* Copyright (c) 2020 IWATA Daiki
#*
#* This software is released under the MIT License.
#* see http://opensource.org/licenses/mit-license
#*


import sys
import os


# 特定の文字(mark) でpadding する関数．
def StringPadding(mark, length):
    assert len(mark) == 1, "mark length is not correct..."
    
    padding = ""
    
    for i in range(length):
        padding += mark


    return padding


# (存在しない場合のみ) ディレクトリを作成する関数．
def CreateDirectory(dir_name):
    if(os.path.isdir(dir_name) == False):
        os.mkdir(dir_name)



if __name__ == "__main__":
    
    argvs = sys.argv
    argc  = len(argvs)
    
    if argc != 7:
        print("ERROR: stdin is not correct...")
        print("USAGE: python SplitVCFData_size.py [in_path] [out_path] [chrID] [nation].")
        quit()


    # 変数定義
    input_filepath  = argvs[1]
    output_filepath = argvs[2]
    chromosomeID    = argvs[3]
    nation          = argvs[4]
    THRES_FILE_SIZE = int(argvs[5])
    FILE_PADDING    = int(argvs[6])
    MAX_FILE_SIZE   = THRES_FILE_SIZE + FILE_PADDING
    # input_filename  = input_filepath + "chr" + chromosomeID + "_" + nation + ".recode.vcf"
    input_filename  = input_filepath + "chr" + chromosomeID + "_" + nation + "_sample.vcf"
    CreateDirectory(output_filepath)
    
    header      = []
    data        = []
    split_data  = []
    header_length = 0
    data_length   = 0
    file_num      = 0

    # vcf ファイルを改行区切りでリストとして取得する．
    with open(input_filename, 'r') as f:
        vcf_files = f.readlines()
        

    # line の最初の2 文字でデータを分類する．
    for line in vcf_files:
        check = line[0:2]

        if check == "#C":    # 要素名．
            header.append(line)
            header_length += len(line)
        elif check != "##":  # データ本体．
            data.append(line)


        
    split_data.extend(header)
    data_length = header_length

    for each_data in data:
        if data_length < THRES_FILE_SIZE:
            split_data.append(each_data)
            data_length += len(each_data)
        else:
            split_data.append(each_data)
            data_length += len(each_data)
            
            # データサイズが一定になるようにパディングする．
            padding = StringPadding("*", MAX_FILE_SIZE - data_length)
            split_data.append(padding)
        
            
            # ファイルに書き込む．
            filename = output_filepath + "chr" + chromosomeID + "_" + nation + "_" + str(file_num) + ".vcf"            
            with open(filename, 'w') as f:
                f.writelines(split_data)


            # 変数の更新，再初期化
            file_num += 1
            data_length = header_length
            del split_data[:]
            split_data.extend(header)
            
    print("SUCCESS: Split vcf data(chrID: " + chromosomeID + ", nation: " + nation + ")")
