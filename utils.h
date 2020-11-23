#ifndef UTILS_H
#define UTILS_H

#include <tuple>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <Eigen/Sparse>

#include <pybind11/pybind11.h>

namespace py = pybind11;

using namespace std;

namespace utils {


struct SparseData
{
  SparseData() {}

  SparseData(vector<float> data_, vector<int> indices_): data(data_), indices(indices_) {} 

  void push(int index, float value) {
    data.push_back(value);
    indices.push_back(index);
  }

  vector<float> data;
  vector<int> indices;
};

template<typename T>
std::vector<float> linspace(T start_in, T end_in, int num_in)
{

  std::vector<float> linspaced;

  float start = static_cast<float>(start_in);
  float end = static_cast<float>(end_in);
  float num = static_cast<float>(num_in);

  if (num == 0) { return linspaced; }
  if (num == 1) 
    {
      linspaced.push_back(start);
      return linspaced;
    }

  float delta = (end - start) / (num - 1);

  for(int i=0; i < num-1; ++i)
    {
      linspaced.push_back(start + delta * i);
    }
  linspaced.push_back(end); // I want to ensure that start and end
                            // are exactly the same as the input
  return linspaced;
}

template<typename T>
std::vector<int> argsort(const std::vector<T>& data) {

  std::vector<int> v(data.size());

  std::iota(v.begin(), v.end(), 0);
  std::sort(v.begin(), v.end(), [&](int i, int j){ return data[i] < data[j]; });

  return v;
}

template<typename T>
std::vector<T> arrange_by_indices(const std::vector<T>& data, std::vector<int>& indices) 
{
  std::vector<T> v(indices.size());

  for( int i = 0; i < indices.size(); ++i ) {
    v[i] = data[indices[i]];
  }

  return v;
}



std::tuple<std::vector<int>, std::vector<int>, std::vector<float>> to_row_format(const Eigen::SparseMatrix<float, Eigen::RowMajor>& M);
Eigen::SparseMatrix<float, Eigen::RowMajor> create_sparse(vector<int>& rows, vector<int>& cols, vector<float>& vals, int size, int density);
Eigen::SparseMatrix<float, Eigen::RowMajor> create_sparse(const vector<SparseData>& X, int size, int density);
float rdist(const vector<float>& x, const vector<float>& y);
float clip(float value);
long tau_rand_int(vector<long>& state);
vector<vector<float>> pairwise_distances(vector<vector<float>>& X);


}

#endif