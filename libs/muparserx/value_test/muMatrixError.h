#ifndef MU_MATRIX_ERROR_H
#define MU_MATRIX_ERROR_H

#include <stdexcept>
#include <string>

namespace mu
{
  class MatrixError : public std::runtime_error
  {
  public:
    explicit MatrixError(const std::string &sMsg)
        :std::runtime_error(sMsg)
    {}
  };
}

#endif