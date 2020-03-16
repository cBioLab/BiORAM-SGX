/*!
 *
 * SGX_Measurement.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 *
 */


#include "SGX_Measurement.hpp"



chrono::system_clock::time_point chrono_start;

void OCALL_chrono_start()
{
  chrono_start = chrono::system_clock::now();
}


double OCALL_chrono_end()
{
  chrono::system_clock::time_point chrono_end = chrono::system_clock::now();
  double elapsed =
    chrono::duration_cast<chrono::milliseconds>(chrono_end - chrono_start).count();

  return elapsed;
}





chrono::system_clock::time_point chrono_Total_start, chrono_Total_end;
chrono::system_clock::time_point chrono_AESenc_start, chrono_AESenc_end;
chrono::system_clock::time_point chrono_AESdec_start, chrono_AESdec_end;
chrono::system_clock::time_point chrono_FileIn_start, chrono_FileIn_end;
chrono::system_clock::time_point chrono_FileOut_start, chrono_FileOut_end;
chrono::system_clock::time_point chrono_GetPath_start, chrono_GetPath_end;
chrono::system_clock::time_point chrono_WritePath_start, chrono_WritePath_end;
chrono::system_clock::time_point chrono_Check_start, chrono_Check_end;
double Total_elapsed, AESenc_elapsed, AESdec_elapsed, FileIn_elapsed, FileOut_elapsed;
double GetPath_elapsed, WritePath_elapsed, Check_elapsed;


void OCALL_chrono_Total_start()
{
  chrono_Total_start = chrono::system_clock::now();
}


void OCALL_chrono_Total_end()
{
  chrono_Total_end = chrono::system_clock::now();
  double elapsed   = chrono::duration_cast<chrono::milliseconds>(chrono_Total_end - chrono_Total_start).count();

  Total_elapsed   += elapsed;
}


void OCALL_chrono_AESenc_start()
{
  chrono_AESenc_start = chrono::system_clock::now();
}


void OCALL_chrono_AESenc_end()
{
  chrono_AESenc_end = chrono::system_clock::now();
  double elapsed    = chrono::duration_cast<chrono::microseconds>(chrono_AESenc_end - chrono_AESenc_start).count();

  AESenc_elapsed   += elapsed / 1000;
}


void OCALL_chrono_AESdec_start()
{
  chrono_AESdec_start = chrono::system_clock::now();
}


void OCALL_chrono_AESdec_end()
{
  chrono_AESdec_end = chrono::system_clock::now();
  double elapsed    = chrono::duration_cast<chrono::microseconds>(chrono_AESdec_end - chrono_AESdec_start).count();

  AESdec_elapsed   += elapsed / 1000;
}


void OCALL_chrono_FileIn_start()
{
  chrono_FileIn_start = chrono::system_clock::now();
}


void OCALL_chrono_FileIn_end()
{
  chrono_FileIn_end = chrono::system_clock::now();
  double elapsed    = chrono::duration_cast<chrono::microseconds>(chrono_FileIn_end - chrono_FileIn_start).count();

  FileIn_elapsed   += elapsed / 1000;
}


void OCALL_chrono_FileOut_start()
{
  chrono_FileOut_start = chrono::system_clock::now();
}


void OCALL_chrono_FileOut_end()
{
  chrono_FileOut_end = chrono::system_clock::now();
  double elapsed     = chrono::duration_cast<chrono::microseconds>(chrono_FileOut_end - chrono_FileOut_start).count();

  FileOut_elapsed   += elapsed / 1000;
}


void OCALL_chrono_GetPath_start()
{
  chrono_GetPath_start = chrono::system_clock::now();
}


void OCALL_chrono_GetPath_end()
{
  chrono_GetPath_end = chrono::system_clock::now();
  double elapsed     = chrono::duration_cast<chrono::microseconds>(chrono_GetPath_end - chrono_GetPath_start).count();

  GetPath_elapsed   += elapsed / 1000;
}


void OCALL_chrono_WritePath_start()
{
  chrono_WritePath_start = chrono::system_clock::now();
}


void OCALL_chrono_WritePath_end()
{
  chrono_WritePath_end = chrono::system_clock::now();
  double elapsed       = chrono::duration_cast<chrono::microseconds>(chrono_WritePath_end - chrono_WritePath_start).count();

  WritePath_elapsed   += elapsed / 1000;
}


void OCALL_chrono_Check_start()
{
  chrono_Check_start = chrono::system_clock::now();
}


void OCALL_chrono_Check_end()
{
  chrono_Check_end = chrono::system_clock::now();
  double elapsed   = chrono::duration_cast<chrono::microseconds>(chrono_Check_end - chrono_Check_start).count();

  Check_elapsed   += elapsed / 1000;
}


void OCALL_ShowMeasuredTime()
{
  cout << endl;
  cout << "--------------- Calculation Time ---------------" << endl;
  cout << "AES encryption: " << AESenc_elapsed  << "[ms]." << endl;
  cout << "AES decryption: " << AESdec_elapsed  << "[ms]." << endl;
  cout << "       File In: " << FileIn_elapsed  << "[ms]." << endl;
  cout << "      File Out: " << FileOut_elapsed << "[ms]." << endl << endl;

  cout << "  GetPath() func: " << GetPath_elapsed   << "[ms]." << endl;
  cout << "WritePath() func: " << WritePath_elapsed << "[ms]." << endl;

  cout << "\ncheck: " << Check_elapsed << "[ms]." << endl;
  
  cout << "\nTotal: " << Total_elapsed << "[ms]." << endl;
  
  cout << "------------------------------------------------" << endl;
  cout << endl;
}





chrono::system_clock::time_point chrono_FS_start,  chrono_FS_end;
chrono::system_clock::time_point chrono_ML_start,  chrono_ML_end;
chrono::system_clock::time_point chrono_res_start, chrono_res_end;

void OCALL_FS_start()
{
  chrono_FS_start = chrono::system_clock::now();
}


void OCALL_FS_end()
{
  chrono_FS_end = chrono::system_clock::now();
}


void OCALL_ML_start()
{
  chrono_ML_start = chrono::system_clock::now();
}


void OCALL_ML_end()
{
  chrono_ML_end = chrono::system_clock::now();
}


void OCALL_res_start()
{
  chrono_res_start = chrono::system_clock::now();
}


void OCALL_res_end()
{
  chrono_res_end = chrono::system_clock::now();
}


void OCALL_ExpResult()
{
  double FS_time  = chrono::duration_cast<chrono::microseconds>(chrono_FS_end  - chrono_FS_start).count()  / 1000.0;
  double ML_time  = chrono::duration_cast<chrono::microseconds>(chrono_ML_end  - chrono_ML_start).count()  / 1000.0;
  double res_time = chrono::duration_cast<chrono::microseconds>(chrono_res_end - chrono_res_start).count() / 1000.0;
  
  cout << endl;
  cout << "--------------- Calculation Time ---------------" << endl;
  cout << "File Search: " << FS_time  << "[ms]." << endl;
  cout << "ML function: " << ML_time  << "[ms]." << endl;
  cout << "      Total: " << res_time << "[ms]." << endl;
  
  cout << "------------------------------------------------" << endl;
  cout << endl;
}
