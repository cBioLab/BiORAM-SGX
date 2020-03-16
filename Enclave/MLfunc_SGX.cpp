/*!
 *
 * MLfunc_SGX.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#include "MLfunc_SGX.hpp"


/* 1. Fisher's Exact Test. */

void AlignVariable(vector<double>& vec)
{
  while(vec[0] >= 10){
    vec[0] /= 10.0;
    vec[1] += 1.0;
  }
  while(0 < vec[0] && vec[0] < 1){
    vec[0] *= 10.0;
    vec[1] -= 1.0;
  }

  return;
}


// Function to calculate probability that create one 2×3 table.
void getProbability(vector<double>& prob, int a, int b, int c, int d, int e, int f,
		    int v, int w, int x, int y, int z, int N)
{
  int num_phase = 1, den_phase = 1;
  int numerator = 0, denominator = 0;

  prob[0] = 1.0;
  prob[1] = 0.0;
  
  
  numerator   = v;
  denominator = N;
  
  while(num_phase != 6 || den_phase != 8){
    if(numerator <= 1){ // 更新に使用する値(分子)の値の決定．
      if(num_phase == 1)       numerator = w;
      else if(num_phase == 2)  numerator = x;
      else if(num_phase == 3)  numerator = y;
      else if(num_phase == 4)  numerator = z;
      else if(num_phase >= 5)  numerator = 1; // numerator の計算が終了した場合．

      if(num_phase < 6)  // num_phase の最大値= 6 とする．
	num_phase++;
    }
    
    if(denominator <= 1){ // 更新に使用する値(分母)の値の決定．
      if(den_phase == 1)       denominator = a;
      else if(den_phase == 2)  denominator = b;
      else if(den_phase == 3)  denominator = c;
      else if(den_phase == 4)  denominator = d;
      else if(den_phase == 5)  denominator = e;
      else if(den_phase == 6)  denominator = f;
      else if(den_phase >= 7)  denominator = 1; // denominator の計算が終了した場合．

      if(den_phase < 8) // den_phase の最大値= 8 とする．
	den_phase++;
    }

    if(numerator == 0)    numerator = 1; // v~z に0 が含まれていた場合の処理．0! = 1
    if(denominator == 0)  denominator = 1; // a~f, N に0 が含まれていた場合の処理．0! = 1
    
    
    // update prob.
    prob[0] *= (double)numerator / (double)denominator;

    AlignVariable(prob);

    
    numerator--;
    denominator--;
  }  

  return;
}


void getP_value(int v, int w, int x, int y, int z, int N, vector<double> prob, vector<double>& p_value)
{
  p_value[0] = 0.0;
  p_value[1] = 0.0;
  vector<double> tmp(2, 0.0);
  int a, b, c, d, e, f;
  int min;
  
  if(x > v){
    min = v;
  }
  else{
    min = x;
  }
  
  
  // calculate all probs in each predicted table.
  for(int i=0; i<=min; i++){
    for(int j=0; j<=v-i; j++){
      a = i;
      b = j;

      c = v - b - a;
      d = x - a;
      e = y - b;
      f = z - c;

      // each cell in table is more than 0.
      if(a < 0 || b < 0 || c < 0 || d < 0 || e < 0 || f < 0)
        continue;

      
      // calculate prob.
      getProbability(tmp, a, b, c, d, e, f, v, w, x, y, z, N);
      
      if(prob[1] == tmp[1] && prob[0] >= tmp[0]){
	if(p_value[0] == 0.0){  // 最初は代入する．
	  p_value[0] = tmp[0];
	  p_value[1] = tmp[1];
	}
	else{
	  tmp[0] *= pow(10, tmp[1] - p_value[1]);
	  p_value[0] += tmp[0];
	}
      }
      else if(prob[1] > tmp[1]){
	if(p_value[0] == 0.0){  // 最初は代入する．
	  p_value[0] = tmp[0];
	  p_value[1] = tmp[1];
	}
	else{
	  tmp[0] *= pow(10, tmp[1] - p_value[1]);
	  p_value[0] += tmp[0];
	}
      }

      AlignVariable(p_value);
    }
  }


  
  vector<double>().swap(tmp);
  
  return;
}


void FisherExactTest(vector< vector<int> >& ContingencyTable, vector<double>& p_value)
{
  /*
   * We assume following table. Each valiables is corresponded.
   *
   * ┌─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┬───────┐
   * │    nation \ value   │          0          │          1          │          2          │ Total │
   * ├─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼───────┤
   * │       nation1       │ a = ContTable[0][0] │ b = ContTable[0][1] │ c = ContTable[0][1] │   v   │
   * ├─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼───────┤
   * │       nation2       │ d = ContTable[1][0] │ e = ContTable[1][1] │ f = ContTable[0][1] │   w   │
   * ├─────────────────────┼─────────────────────┼─────────────────────┼─────────────────────┼───────┤
   * │        Total        │          x          │          y          │          z          │   N   │
   * └─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┴───────┘
   *
   */

  int a = ContingencyTable[0][0];
  int b = ContingencyTable[0][1];
  int c = ContingencyTable[0][2];
  int d = ContingencyTable[1][0];
  int e = ContingencyTable[1][1];
  int f = ContingencyTable[1][2];
  int v = ContingencyTable[0][0] + ContingencyTable[0][1] + ContingencyTable[0][2];
  int w = ContingencyTable[1][0] + ContingencyTable[1][1] + ContingencyTable[1][2];
  int x = ContingencyTable[0][0] + ContingencyTable[1][0];
  int y = ContingencyTable[0][1] + ContingencyTable[1][1];
  int z = ContingencyTable[0][2] + ContingencyTable[1][2];
  int N = x + y + z;

  assert(v != 0);
  assert(w != 0);
  assert(N != 0);

  
  vector<double> prob(2, 0.0); // prob = prob[0] * 10^prob[1].
  prob[0] = 1.0;

  
  getProbability(prob, a, b, c, d, e, f, v, w, x, y, z, N);
  
  
  getP_value(v, w, x, y, z, N, prob, p_value);
  


  vector<double>().swap(prob);
  
  return;
}





/* 2. Logistic Regression. */

/*
 * Function to split data.
 * < input >
 * [vector<int>] result: vector to hold each data (should be int type).
 * [string]         str: original charactor.
 * [char]           del: deliminator (ex. ',').
 */
void SplitStringInt(vector<int>& result, string str, char del)
{
  int first = 0;
  int last  = str.find_first_of(del);
  
  while(first < str.size()){
    string split_str(str, first, last - first);
    
    result.push_back(atoi(split_str.c_str()));

    
    first = last + 1;
    last = str.find_first_of(del, first);

    
    if(last == string::npos)
      last = str.size();

    
    string().swap(split_str);
  }


  return;
}


void LogisticRegression(vector< vector<double> >& x, vector<int>& y, vector<double>& theta,
			int iteration, int regularization)
{
  int N = x.size();     // ポジション数 + 1
  int M = x[0].size();  // データ数．
    
  vector<double> LR_function(M, 0.0);
  double diff_negLLF = 0.0;
  string cout = "";
  
  // ロジスティック回帰で使用する係数．
  int lambda = 1;
  double rate = 0.01;

  
  // 回帰係数の初期化．0 <= theta[i] <= 0.01
  for(int i=0; i<N; i++){
    theta.push_back((double)UniformDistribution_int(0, 1e4) / 1e6);
    // OCALL_print_double(theta[i]);
  }

  
  for(int itr=0; itr<iteration; itr++){
    // y - sigmoid[theta*x] を求める．
    for(int i=0; i<N; i++){
      for(int j=0; j<M; j++){
	if(i == 0)
	  LR_function[j] = 0.0;
	
	LR_function[j] -= theta[i] * x[i][j];
	
	
	if(i == N - 1)
	  LR_function[j] = y[j] - 1 / (1 + exp(LR_function[j]));
      }
    }


    // 微分した負の対数尤度関数を導出し，theta を更新する．
    for(int i=0; i<N; i++){
      for(int j=0; j<M; j++){
	if(j == 0)
	  diff_negLLF = 0.0;
	
	diff_negLLF -= LR_function[j] * theta[i] * x[i][j];


	if(j == M-1){
	  if(regularization == 1)  // 正則化項ありの場合．
	    diff_negLLF = (lambda * theta[i] + diff_negLLF) / M;

	  theta[i] -= rate * diff_negLLF;
	}	  
      }

      cout += to_string(theta[i]) + ", ";
    }

    // OCALL_print(cout.c_str());
    cout = "";
  }

  
  // メモリ開放．
  string().swap(cout);
  vector<double>().swap(LR_function);
  
  return;
}





/* 3. Principal Component Analysis. */

// Function to normalize data.
// Assume that data must be M×N vector.
void getNormalizedData(int row, int column, vector< vector<double> >& x)
{
  double ave = 0.0, SD = 0.0;
  
  for(int i=0; i<row; i++){
    ave = 0.0, SD = 0.0;
    
    for(int j=0; j<column; j++){
      ave += x[i][j];
    }
    ave /= column;


    for(int j=0; j<column; j++){
      SD += (x[i][j] - ave) * (x[i][j] - ave);
    }
    SD = sqrt(SD / column);

    
    for(int j=0; j<column; j++){
      x[i][j] = (x[i][j] - ave) / SD;
    }
  }

  
  return;
}


// Function to calculate covariance.
void getCovarianceMatrix(int row, int column, vector< vector<double> >& x, vector< vector<double> >& covariance)
{
  for(int i=0; i<row; i++){
    for(int j=i; j<column; j++){
      for(int k=0; k<row; k++)
	covariance[i][j] += x[i][k] * x[j][k];
    }

    
    for(int j=i; j<column; j++){
      covariance[i][j] /= (row - 1);
      
      if(j != i)
	covariance[j][i] = covariance[i][j];
    }
  }

  
  return;
}


// Function to get n_component eigenvectors using power method.
void getEigenVector(vector< vector<double> >& A, vector< vector<double> >& eigenvector, int n_component, const double thres)
{
  // Initialization.
  const int N = (int)A[0].size();
  
  vector<double> y(N, 0);
  int count;
  double eigenvalue, prv_eigenvalue;
  bool itrFlag;

  assert(N*N != A.size());
  assert(eigenvector[0].size()*n_component != eigenvector.size());


  for(int itr=0; itr<n_component; itr++){
    // Initialization.
    count = 0;
    itrFlag = true;
    eigenvalue = 0.0;

    
    while(itrFlag){
      prv_eigenvalue = eigenvalue;
      eigenvalue = 0.0;
      
      for(int i=0; i<N; i++){
	y[i] = 0;
	
	// y = Ax (x converges to eigenvector).
	for(int j=0; j<N; j++)
	  y[i] += A[i][j] * eigenvector[itr][j];

	if(i == 0)  eigenvalue = y[i] * eigenvector[itr][i];
	else        eigenvalue += y[i] * eigenvector[itr][i];
      }

      
      if(fabs(eigenvalue - prv_eigenvalue) < thres)
	itrFlag = false;
      
      
      if(itrFlag){
	double length = 0.0;

	for(int j=0; j<N; j++){
	  eigenvector[itr][j] = y[j];
	  length += eigenvector[itr][j] * eigenvector[itr][j];
	}
	length = sqrt(length);
	
	for(int j=0; j<N; j++)
	  eigenvector[itr][j] /= length;

	
	count++;
      }
    }


    // update A.
    for(int i=0; i<N; i++){
      for(int j=0; j<N; j++)
	A[i][j] -= eigenvalue * eigenvector[itr][i] * eigenvector[itr][j];
    }

    // string EigenCheck = "calculate " + to_string(count) + " times to solve EigenProblem. Eigenvalue: " + to_string(eigenvalue);
    // OCALL_print(EigenCheck.c_str());
  }



  vector<double>().swap(y);
  
  return;
}


// reduce dimension to n_component.
void ReduceDimension(int row, int column, vector< vector<double> >& x, vector< vector<double> >& eigenvector,
		     vector< vector<double> >& reduced, int n_component)
{
  vector< vector<double> > tmp(row, vector<double>(n_component, 0.0));

  for(int i=0; i<row; i++){
    for(int k=0; k<column; k++){
      for(int j=0; j<n_component; j++)
	tmp[i][j] += x[k][i] * eigenvector[j][k];
    }
  }

  for(int i=0; i<row; i++){
    for(int j=0; j<n_component; j++){
      reduced[i].push_back(0.0);
      
      for(int k=0; k<column; k++)
	reduced[i][j] += tmp[i][j] * eigenvector[j][k];
    }
  }


  // メモリ開放．
  vector< vector<double> >().swap(tmp);
  
  return;
}


void PCA(vector< vector<double> >& x, vector< vector<double> >& PCA_data, int n_component)
{
  // Initialization.
  int N = x.size();     // position size.
  int M = x[0].size();  // data num.
  const double thres = 1e-6;
  string cout = "";

  
  
  // Normalize data. In case of SNP's data, we do not need to normalize data.
  // getNormalizedData(N, M, x);
  
  
  // Get covariance.
  // In this function, we reverse row and col of variable "covariance" in consideration of following calculation.
  vector< vector<double> > covariance(N, vector<double>(N, 0.0));
  getCovarianceMatrix(M, N, x, covariance);


  
  // prepare n_component×N eigenvectors.
  // eigenvectors initialize Unit vactor.
  vector< vector<double> > eigenvector(n_component, vector<double>(N, 1/sqrt((double)N)));
  
  // Get eigenvectors using power method.
  getEigenVector(covariance, eigenvector, n_component, thres);
  

  // Reduce dimension using eigenvectors.
  ReduceDimension(M, N, x, eigenvector, PCA_data, n_component);
  

  string().swap(cout);
  vector< vector<double> >().swap(covariance);
  vector< vector<double> >().swap(eigenvector);
  
  return;
}
