#!/usr/bin/env python
# -*- coding: utf-8 -*-

#*
#* SelectPosition.py
#*
#* Copyright (c) 2020 IWATA Daiki
#*
#* This software is released under the MIT License.
#* see http://opensource.org/licenses/mit-license
#*



import random


filepath       = "/mnt/sdb1/iwata/vcf_init/"
input_filename = filepath + "PositionList_chr22"


select_num    = 10
position_list = []
select_pos    = []
output        = ""


if __name__ == "__main__":
    print("Load position list: " + input_filename)

    with open(input_filename, 'r') as f:
        position_list = [s.strip() for s in f.readlines()]


    ## position_list からランダムにselect_num 個を選ぶ(重複なし)．
    select_pos = random.sample(position_list, select_num)

    print(','.join(select_pos))
