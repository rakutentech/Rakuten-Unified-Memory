/**\file
 * \brief Contains floating- and fixed-point (templates) FIR filters.
 *
 * This file contains the following classes
 * - FIRBasic: floating-point FIR filter with real-valued coefficients.
 * - FIR<>: templated fixed-point FIR filter with real-valued coefficients.
 * - FIRSymmetric<>: templated fixed-point FIR filter with real-valued coefficients. 
 * Number of multiplies reduced by taking advantage of coefficient symmetry. 
 * - CxFIR<>: templated fixed-point FIR filter with complex-valued coefficients.
 *
 * All of the above support
 * - filter(): textbook filter operation.
 * - decimate(): filter then downsample.
 * - interp(): upsample then filter.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef FIR_H_
#define FIR_H_

#include <deque>
#include <vector>
#include <complex>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>

#include "filter.h"
#include "complexmultiply.h"


/**
 * Basic floating-point implementations of FIR filter. Used as base class for
 * fixed-point implementations.
 *
 * It stores the coefficients and delay line.
 * It has a init() to zero the delay line.
 * Computation is done in full double's.
 * Runs at one-sample rate.
 *
 * Difference equation:
 * Shift delay line and DelayLine[0] = in
 * out = Coeffs[0]*DelayLine[0]
 *       + Coeffs[1]*DelayLine[1]
 *       + ...
 *       + Coeffs[N-1]*DelayLine[N-1];
 */
template<class T>
class FIRBasic : public Filter<T>
{
public:
	/**
	 * Constructs a filter with given coefficients.
	 *
	 * Computes main tap index as
	 * - center tap if filter length is odd
	 * - left center tap if filter length is even
	 * This tap index is used to select the phase during decimation such that if
	 * input was a impulse, the output contains the main tap.
	 */
	FIRBasic(const std::vector<T> &Coeffs)
	: Filter<T>(Coeffs)
	, DelayLine_(Coeffs.size(), 0)
	, MainTapIndex_(Coeffs.size()/2 - (Coeffs.size()%2 ? 0 : 1))
	{
		if (Coeffs.size() == 0)
			throw std::string("Number of coefficients is zero!");
	}

	/**
	 * Constructs a filter with given coefficients.
	 *
	 * Main tap index is used to select the phase during decimation such that if
	 * input was a impulse, the output contains the main tap.
	 */
	FIRBasic(const std::vector<T> &Coeffs,	const size_t MainTapIndex)
	: Filter<T>(Coeffs)
	, DelayLine_(Coeffs.size(), 0)
	, MainTapIndex_(MainTapIndex)
	{
		if (Coeffs.size() == 0)
			throw std::string("Number of coefficients is zero!");
		if (MainTapIndex >= Coeffs.size())
			throw std::string("Main tap index must be < number of coeffs!");
	}

	/** Zero delay line. */
	void init(void)
	{
		std::fill(DelayLine_.begin(), DelayLine_.end(), T(0));
	}

	/** Textbook filter operation. Fill delay line, multiply and accumulate. */
	virtual T
	filter(const T &In)
	{
		shift(In);
		return compute();
	}

	/**
	 * Filter then downsample.
	 *
	 * \param In	Downsample at the rate of its size. In[0] is the oldest.
	 * \param Phase	Phase of the output. Output at other phases are trashed.
	 */
	virtual T
	decimate(const std::vector<T> &In, const size_t Phase)
	{
		if (Phase >= In.size())
		{
			std::stringstream msg;
			msg << "Phase selected must be smaller than input size!"
				<< " Phase: " << Phase
				<< " Size: " << In.size() << std::endl;
			throw msg.str();
		}

		T Out = 0;
		for (size_t i = 0; i < In.size(); i++)
		{
			shift(In[i]);
			if (i == Phase)
				Out = compute();
		}
		return Out;
	}

	/**
	 * Filter then downsample.
	 *
	 * Output phase is automatically calculated such that if input is a impulse,
	 * output contains the main tap.
	 *
	 * \param In	Downsample at the rate of its size. In[0] is the oldest.
	 * 				DO NOT vary its size from call to call.
	 */
	virtual T
	decimate(const std::vector<T> &In)
	{
		const size_t Phase = MainTapIndex_ % In.size();
		/*std::cout
			<< "MainTapIndex_ " << MainTapIndex_
			<< " In.size() " << In.size()
			<< " Phase " << Phase
			<< std::endl;*/
		return decimate(In, Phase);
	}

	/**
	 * Upsample then filter using polyphase implementation.
	 */
	virtual std::vector<T>
	interp(const T &In, const size_t Rate = 2)
	{
		shift(In);
		return compute_polyphase(Rate);
	}

	size_t main_tap_index(void) { return MainTapIndex_; }

	virtual ~FIRBasic()	{ }

private:
	FIRBasic(); // disabled

protected:
	std::deque<T> DelayLine_;
	const size_t MainTapIndex_;

	/**
	 * Shift and add input to the delay line. DelayLine[0] is the newest
	 * while DelayLine[N-1] oldest.
	 *
	 * Derived class typically replaces this with their own implementation.
	 */
	virtual void
	shift(const T &In)
	{
		DelayLine_.pop_back();
		DelayLine_.push_front(In);
	}

	/**
	 * Compute the difference equation.
	 *
	 * Derived class typically replaces this with their own implementation.
	 */
	virtual T
	compute(void)
	{
		T Out = 0;
		for (size_t i = 0; i < Filter<T>::Coeffs_.size(); i++)
			Out += DelayLine_[i]*Filter<T>::Coeffs_[i];;
		return Out;
	}

	/**
	 * Compute the difference equation using polyphase filters.
	 *
	 * Derived class typically replaces this with their own implementation.
	 */
	virtual std::vector<T>
	compute_polyphase(const size_t Rate)
	{
		std::vector<T> Out(Rate);
		for (size_t OutI = 0; OutI < Rate; OutI++)
		{
			// Polyphase sub-filter
			T Accum = 0;
			size_t InI = 0;
			for (	size_t CoeffI = OutI;
					CoeffI < Filter<T>::Coeffs_.size();
					CoeffI += Rate)
				Accum += Filter<T>::Coeffs_[CoeffI]*DelayLine_[InI++];
			Out[OutI] = Accum;
		}
		return Out;
	}
};

/**
 * Direct form FIR filter for real-valued coefficients.
 *
 * Runs at one-sample rate.
 *
 * Assumes coefficients are already quantized.
 *
 * Difference equation:
 * Shift delay line and DelayLine[0] = in
 * out = Coeffs[0]*DelayLine[0]
 *       + Coeffs[1]*DelayLine[1]
 *       + ...
 *       + Coeffs[N-1]*DelayLine[N-1];
 */
template<
	class T_IN=double,
	class T_MUL=double,
	class T_ACC=double,
	class T_OUT=double>
class FIR : public FIRBasic<double>
{
public:
	typedef double value_type;

	FIR(
		const std::vector<double> &Coeffs)
	: FIRBasic<double>(Coeffs)
	{ }

	FIR(
		const std::vector<double> &Coeffs,
		const size_t MainTapIndex)
	: FIRBasic<double>(Coeffs, MainTapIndex)
	{ }

private:
	FIR(); // disabled

protected:
	void
	shift(const double In)
	{
		T_IN qIn = In;
		FIRBasic<double>::shift(qIn);
	}

	double
	compute(void)
	{
		T_ACC Accum = 0.0;
		for (size_t i = 0; i < Coeffs_.size(); i++)
		{
			T_MUL Product = DelayLine_[i]*Coeffs_[i];
			Accum += Product;
		}
		T_OUT Out = (double)Accum;
		return Out;
	}

	std::vector<double>
	compute_polyphase(const size_t Rate)
	{
		std::vector<double> Out(Rate);
		for (size_t OutI = 0; OutI < Rate; OutI++)
		{
			// Polyphase sub-filter
			T_ACC Accum = 0;
			size_t InI = 0;
			for (size_t CoeffI = OutI; CoeffI < Coeffs_.size();	CoeffI += Rate)
			{
				T_MUL Product = Coeffs_[CoeffI]*DelayLine_[InI++];
				Accum += Product;
			}
			T_OUT q = (double)Accum;
			Out[OutI] = q;
		}
		return Out;
	}

};


/**
 * Direct form FIR filter for real-valued coefficients that takes advantage of symmetric 
 * coefficients by adding the inputs first, then multiply and accumulate.
 *
 * Runs at one-sample rate.
 *
 * Assumes coefficients are already quantized.
 * Assumes coefficients are symmetric (ie no checking done).
 *
 * Difference equation for 5-coefficient case:
 * Shift delay line and DelayLine[0] = in
 * out =   Coeffs[0]*(DelayLine[0] + DelayLine[4])
 *       + Coeffs[1]*(DelayLine[1] + DelayLine[3])
 *       + Coeffs[2]*DelayLine[2];
 */
template<
	class T_IN=double,
	class T_SUM=double,
	class T_MUL=double,
	class T_ACC=double,
	class T_OUT=double>
class FIRSymmetric : public FIRBasic<double>
{
public:
	typedef std::complex<double> value_type;

	FIRSymmetric(
		const std::vector<double> &Coeffs)
	: FIRBasic<double>(Coeffs)
	, IsOddLength_(Coeffs.size() % 2 ? true : false)
	, NumSymmetric_(Coeffs.size()/2)
	{ }

	FIRSymmetric(
		const std::vector<double> &Coeffs,
		const size_t MainTapIndex)
	: FIRBasic<double>(Coeffs, MainTapIndex)
	, IsOddLength_(Coeffs.size() % 2 ? true : false)
	, NumSymmetric_(Coeffs.size()/2)
	{ }

private:
	FIRSymmetric(); // disabled

protected:
	void
	shift(const double In)
	{
		T_IN qIn = In;
		FIRBasic<double>::shift(qIn);
	}

	double
	compute(void)
	{
		T_ACC Accum = 0.0;
		for (size_t i = 0; i < NumSymmetric_; i++)
		{
			T_SUM Sum = DelayLine_[i] + DelayLine_[Coeffs_.size()-1 - i];
			T_MUL Product = Sum*Coeffs_[i];
			Accum += Product;
		}

		if (IsOddLength_)
		{
			T_MUL Product = DelayLine_[NumSymmetric_]*Coeffs_[NumSymmetric_];
			Accum += Product;
		}

		T_OUT Out = (double)Accum;
		return Out;
	}

	std::vector<double>
	compute_polyphase(const size_t Rate)
	{
		std::vector<double> Out(Rate);
		for (size_t OutI = 0; OutI < Rate; OutI++)
		{
			// Polyphase sub-filter
			T_ACC Accum = 0;
			size_t InI = 0;
			for (size_t CoeffI = OutI; CoeffI < Coeffs_.size();	CoeffI += Rate)
			{
				T_MUL Product = Coeffs_[CoeffI]*DelayLine_[InI++];
				Accum += Product;
			}
			T_OUT q = (double)Accum;
			Out[OutI] = q;
		}
		return Out;
	}

	const bool IsOddLength_;
	const size_t NumSymmetric_;
};


/**
 * Direct form FIR filter for complex-valued coefficients.
 *
 * Runs at one-sample rate.
 *
 * Assumes coefficients are already quantized.
 *
 * Difference equation:
 * Shift delay line and DelayLine[0] = in
 * out = Coeffs[0]*DelayLine[0]
 *       + Coeffs[1]*DelayLine[1]
 *       + ...
 *       + Coeffs[N-1]*DelayLine[N-1];
 */
template<
	class T_IN=double,
	class T_CMUL=double,
	class T_CMUL_OUT=double,
	class T_ACC=double,
	class T_OUT=double>
class CxFIR : public FIRBasic< std::complex<double> >
{
public:
	typedef std::complex<double> value_type;

	CxFIR(
		const std::vector<value_type> &Coeffs)
	: FIRBasic<value_type>(Coeffs)
	{ }

	CxFIR(
		const std::vector<value_type> &Coeffs,
		const size_t MainTapIndex)
	: FIRBasic<value_type>(Coeffs, MainTapIndex)
	{ }

private:
	CxFIR(); // disabled

protected:
	void
	shift(const value_type &In)
	{
		std::complex<T_IN> qIn = In;
		T_IN qReal = In.real();
		T_IN qImag = In.imag();
		FIRBasic<value_type>::shift(value_type(qReal,qImag));
	}

	value_type
	compute(void)
	{
		std::complex<T_ACC> Accum(0, 0);
		for (size_t i = 0; i < Coeffs_.size(); i++)
		{
			value_type Product =
					cxmult<T_CMUL, T_CMUL_OUT>(DelayLine_[i], Coeffs_[i]);
			Accum += Product;
		}
		T_OUT OutReal = Accum.real();
		T_OUT OutImag = Accum.imag();
		return value_type(OutReal, OutImag);
	}

	std::vector<value_type>
	compute_polyphase(const size_t Rate)
	{
		std::vector<value_type> Out(Rate);
		for (size_t OutI = 0; OutI < Rate; OutI++)
		{
			// Polyphase sub-filter
			std::complex<T_ACC> Accum(0, 0);
			size_t InI = 0;
			for (size_t CoeffI = OutI; CoeffI < Coeffs_.size();	CoeffI += Rate)
			{
				value_type Product =
						cxmult<T_CMUL, T_CMUL_OUT>
								(Coeffs_[CoeffI], DelayLine_[InI++]);
				Accum += Product;
			}
			T_OUT OutReal = Accum.real();
			T_OUT OutImag = Accum.imag();
			Out[OutI] = value_type(OutReal, OutImag);
		}
		return Out;
	}

};


#endif /* FIR_H_ */
