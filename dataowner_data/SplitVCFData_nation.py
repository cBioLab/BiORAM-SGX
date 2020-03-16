#!/usr/bin/env python
# -*- coding: utf-8 -*-

#*
#* Sp;itVCFData_nation.py
#*
#* Copyright (c) 2020 IWATA Daiki
#*
#* This software is released under the MIT License.
#* see http://opensource.org/licenses/mit-license
#*


import os
import sys
import subprocess
import xlrd

ACB_list = []  # African Caribbean in Barbados
ASW_list = []  # African Ancestry in Southwest US
BEB_list = []  # Bengali in Bangladesh
CDX_list = []  # Chinese Dai in Xishuangbanna, China
CEU_list = []  # Utah residents with Northern and Western European ancestry
CHB_list = []  # Han Chinese in Bejing, China
CHS_list = []  # Southern Han Chinese, China
CLM_list = []  # Colombian in Medellin, Colombia
ESN_list = []  # Esan in Nigeria
FIN_list = []  # Finnish in Finland
GBR_list = []  # British in England and Scotland
GIH_list = []  # Gujarati Indian in Houston,TX
GWD_list = []  # Gambian in Western Division, The Gambia
IBS_list = []  # Iberian populations in Spain
ITU_list = []  # Indian Telugu in the UK
JPT_list = []  # Japanese in Tokyo, Japan
KHV_list = []  # Kinh in Ho Chi Minh City, Vietnam
LWK_list = []  # Luhya in Webuye, Kenya
MSL_list = []  # Mende in Sierra Leone
MXL_list = []  # Mexican Ancestry in Los Angeles, California
PEL_list = []  # Peruvian in Lima, Peru
PJL_list = []  # Punjabi in Lahore,Pakistan
PUR_list = []  # Puerto Rican in Puerto Rico
STU_list = []  # Sri Lankan Tamil in the UK
TSI_list = []  # Toscani in Italy
YRI_list = []  # Yoruba in Ibadan, Nigeria



# vcftools を用いて国籍ごとに分けたvcf ファイルを作成する関数．
def ExecCommand(chrID, nation, nation_list, in_filepath, in_filename, out_filepath):
    out_filename = "chr" + chrID + "_" + nation
    command = ["vcftools"]

    for name in nation_list:
        command.append("--indv")
        command.append(name.encode('utf-8'))

    command.append("--vcf")
    command.append(in_filepath + in_filename)
    command.append("--recode")
    command.append("--out")
    command.append(out_filepath + out_filename)

    try:
        subprocess.check_call(command)
    except:
        print("cannot execute command in creating " + out_filename + "...")


# (存在しない場合のみ) ディレクトリを作成する関数．
def CreateDirectory(dir_name):
    if(os.path.isdir(dir_name) == False):
        os.mkdir(dir_name)





if __name__ == "__main__":
    argvs = sys.argv
    argc  = len(argvs)

    if argc != 2:
        print("ERROR: stdin is not correct...")
        print("USAGE: python SplitVCFData_size.py [chrID].")
        quit()

    # 変数定義．
    chrID           = argvs[1]
    input_filepath  = "./"
    output_filepath = "./"

    input_filename  = "ALL.chr" + chrID + ".phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf"
    # input_filename  = "ALL.chr" + chrID + ".phase3_shapeit2_mvncall_integrated_v1b.20130502.genotypes.vcf"
    # input_filename  = "ALL.chr" + chrID + ".phase3_integrated_v1b.20130502.genotypes.vcf"
    nation_filename = "20130606_sample_info_.xlsx"
    
    
    
    # Excel ファイルの特定のシートを開く
    xlsx = xlrd.open_workbook(nation_filename)
    sheet = xlsx.sheet_by_name(u'Sample Info')
    # print(sheet.nrows) # 行数．
    # print(sheet.ncols) # 列数．


    # 国籍ごとにサンプルを分類する．
    for i in range(sheet.nrows):
        name   = sheet.cell_value(i, 0)
        nation = sheet.cell_value(i, 2)
        
        if nation == "ACB":
            ACB_list.append(name)
            
        elif nation == "ASW":
            ASW_list.append(name)
        
        elif nation == "BEB":
            BEB_list.append(name)
            
        elif nation == "CDX":
            CDX_list.append(name)
            
        elif nation == "CEU":
            CEU_list.append(name)
            
        elif nation == "CHB":
            CHB_list.append(name)
            
        elif nation == "CHS":
            CHS_list.append(name)
        
        elif nation == "CLM":
            CLM_list.append(name)
            
        elif nation == "ESN":
            ESN_list.append(name)
            
        elif nation == "FIN":
            FIN_list.append(name)
        
        elif nation == "GBR":
            GBR_list.append(name)
            
        elif nation == "GIH":
            GIH_list.append(name)
            
        elif nation == "GWD":
            GWD_list.append(name)
            
        elif nation == "IBS":
            IBS_list.append(name)
            
        elif nation == "ITU":
            ITU_list.append(name)
            
        elif nation == "JPT":
            JPT_list.append(name)
            
        elif nation == "KHV":
            KHV_list.append(name)
            
        elif nation == "LWK":
            LWK_list.append(name)
            
        elif nation == "MSL":
            MSL_list.append(name)
            
        elif nation == "MXL":
            MXL_list.append(name)
            
        elif nation == "PEL":
            PEL_list.append(name)
            
        elif nation == "PJL":
            PJL_list.append(name)
            
        elif nation == "PUR":
            PUR_list.append(name)
            
        elif nation == "STU":
            STU_list.append(name)
            
        elif nation == "TSI":
            TSI_list.append(name)
            
        elif nation == "YRI":
            YRI_list.append(name)

    print("ACB: " + str(len(ACB_list)))
    print("ASW: " + str(len(ASW_list)))
    print("BEB: " + str(len(BEB_list)))
    print("CDX: " + str(len(CDX_list)))
    print("CEU: " + str(len(CEU_list)))
    print("CHB: " + str(len(CHB_list)))
    print("CHS: " + str(len(CHS_list)))
    print("CLM: " + str(len(CLM_list)))
    print("ESN: " + str(len(ESN_list)))
    print("FIN: " + str(len(FIN_list)))
    print("GBR: " + str(len(GBR_list)))
    print("GIH: " + str(len(GIH_list)))
    print("GWD: " + str(len(GWD_list)))
    print("IBS: " + str(len(IBS_list)))
    print("ITU: " + str(len(ITU_list)))
    print("JPT: " + str(len(JPT_list)))
    print("KHV: " + str(len(KHV_list)))
    print("LWK: " + str(len(LWK_list)))
    print("MSL: " + str(len(MSL_list)))
    print("MXL: " + str(len(MXL_list)))
    print("PEL: " + str(len(PEL_list)))
    print("PJL: " + str(len(PJL_list)))
    print("PUR: " + str(len(PUR_list)))
    print("STU: " + str(len(STU_list)))
    print("TSI: " + str(len(TSI_list)))
    print("YRI: " + str(len(YRI_list)))
    
    #print(GBR_list)


    # vcftools を用いて国籍ごとに分けたvcf ファイルを作成する．
    #ExecCommand(chrID, "ACB", ACB_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "ASW", ASW_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "BEB", BEB_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "CDX", CDX_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "CEU", CEU_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "CHB", CHB_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "CHS", CHS_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "CLM", CLM_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "ESN", ESN_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "FIN", FIN_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "GBR", GBR_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "GIH", GIH_list, input_filepath, input_filename, output_filepath)
    ExecCommand(chrID, "GWD", GWD_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "IBS", IBS_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "ITU", ITU_list, input_filepath, input_filename, output_filepath)
    ExecCommand(chrID, "JPT", JPT_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "KHV", KHV_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "LWK", LWK_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "MSL", MSL_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "MXL", MXL_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "PEL", PEL_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "PJL", PJL_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "PUR", PUR_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "STU", STU_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "TSI", TSI_list, input_filepath, input_filename, output_filepath)
    #ExecCommand(chrID, "YRI", YRI_list, input_filepath, input_filename, output_filepath)
    
