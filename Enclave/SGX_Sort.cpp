/*!
 *
 * SGX_Sort.cpp
 *
 * Copyright (c) 2020 IWATA Daiki
 *
 * This software is released under the MIT License.
 * see http://opensource.org/licenses/mit-license
 */


#include "SGX_Sort.hpp"


int partition(double *first, double *last, int *sub_first, double pivot){
  int l = 0;
  int r = last - first - 1;


  while(true){
    while(first[l]  < pivot) ++l;
    while(first[r] >= pivot) --r;
    if(l >= r)  return l;

    swap_double(&first[l],  &first[r]);
    swap_int(&sub_first[l], &sub_first[r]);
  }
}


void QuickSort_LRresult(double *first, double *last, int *sub_first, int *sub_last){
  int size = last - first;
  if (size <= 1) return;


  double a = first[0];
  double b = first[size / 2];
  double c = first[size - 1];

  
  double pivot;
  if((a-b)*(b-c)*(c-a) != 0)  // pivot: median between a, b and c.
    pivot = fmax(fmin(a, b), fmin(fmax(a, b), c));

  else{  // If median cannot calculate, pivot is large one of the two.
    bool flag = true;


    for(int i=1; i<size; i++){
      if(first[i-1] != first[i]){
        pivot = fmax(first[i-1], first[i]);
        flag = false;
        break;
      }
    }

    if(flag)  return;
  }

  
  int k = partition(first, last, sub_first, pivot); // split array.
  QuickSort_LRresult(first, first+k, sub_first, sub_first+k);
  QuickSort_LRresult(first+k, last,  sub_first+k,  sub_last);
}
