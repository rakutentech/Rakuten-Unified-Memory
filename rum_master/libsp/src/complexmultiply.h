/**\file
 * \brief Templated model of complex multiplication.
 *
 * Contains a textbook 4-scalar-multiply and a 3-scalar-multiply versions. 
 * Inputs are assumed quantized.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef COMPLEXMULTIPLY_H_
#define COMPLEXMULTIPLY_H_

#include <complex>


/**
 * out = x * y
 * Textbook 4-scalar-multiply.
 * x and y are assumed quantized and double type.
 */
template<class T_MULT, class T_OUT>
std::complex<double>
cxmult(const double xr, const double xi, const double yr, const double yi)
{
	T_MULT mult1 = xr * yr;
	T_MULT mult2 = xi * yi;
	T_OUT outr = mult1 - mult2;

	T_MULT mult3 = xr * yi;
	T_MULT mult4 = xi * yr;
	T_OUT outi = mult3 + mult4;

	return std::complex<double>(outr, outi);
}


/**
 * out = x * y
 * Textbook 4-scalar multiply.
 * x and y are assumed quantized and std::complex<double> type.
 */
template<class T_MULT, class T_OUT>
std::complex<double>
cxmult(const std::complex<double> &x, const std::complex<double> &y)
{
	return cxmult<T_MULT, T_OUT>(x.real(), x.imag(), y.real(), y.imag());
}


/**
 * out = x * y
 * 3-scalar multiply.
 * x and y are assumed quantized and double type.
 */
template<class T_MULT_REAL, class T_MULT_IMAG, class T_OUT>
std::complex<double>
cxmult3(const double xr, const double xi, const double yr, const double yi)
{
	T_MULT_REAL mult1 = xr * yr;
	T_MULT_REAL mult2 = xi * yi;
	T_OUT outr = mult1 - mult2;

	double sum1 = xr+xi;
	double sum2 = yr+yi;
	T_MULT_IMAG mult3 = sum1 * sum2;
	T_OUT outi = mult3 - mult1 - mult2;

	return std::complex<double>(outr, outi);
}


/**
 * out = x * y
 * 3-scalar multiply.
 * x and y are assumed quantized and std::complex<double> type.
 */
template<class T_MULT_REAL, class T_MULT_IMAG, class T_OUT>
std::complex<double>
cxmult3(const std::complex<double> &x, const std::complex<double> &y)
{
	return cxmult3<T_MULT_REAL, T_MULT_IMAG, T_OUT>(x.real(), x.imag(), y.real(), y.imag());
}


#endif /* COMPLEXMULTIPLY_H_ */
