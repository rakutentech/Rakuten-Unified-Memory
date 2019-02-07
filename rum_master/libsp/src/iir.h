/**\file
 * \brief Contains floating- and fixed-point (templates) IIR filters.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef IIR_H_
#define IIR_H_

#include "filter.h"

#include <vector>
#include <deque>
#include <string>
#include <algorithm>

/**Textbook IIR biquad filter.
 * Transfer function:
 *              b0 +  b1 z^-1 +  b2  z^-2
 * H(z) = gain ----------------------------
 *              1  +  a1 z^-1 +  a2  z^-2
 *
 * Difference equation:
 * y(n) = gain ( b0 x(n) + b1 x(n-1) + b2 x(n-3) - a1 y(n-1) - a2 y(n-2) )
 *
 * Coefficient vector: [b0 b1 b2 a0 a1 a2 gain]
 * a0 is assumed to be 1.
 */
template<class T>
class IIRBiquad : public Filter<T>
{
public:
	/** Define coefficients.
	 * Coefficient vector: [b0 b1 b2 a0 a1 a2 gain]
	 * a0 is assumed to be 1.0.
	 * If gain is missing, it is assumed to be 1.0.
	 */
	IIRBiquad(const std::vector<T> &Coeffs)
	: Filter<T>(Coeffs)
	, DelayLine_(2, T(0))
	{
		if (Coeffs.size() == 0)
			throw std::string("Number of coefficients is zero!");
		if (Coeffs.size() < 6)
			throw std::string("Expect 6 coefficients!");
		b0_ = Coeffs[0];
		b1_ = Coeffs[1];
		b2_ = Coeffs[2];
		a1_ = Coeffs[4];
		a2_ = Coeffs[5];
		if (Coeffs.size() > 6)
			gain_ = Coeffs[6];
		else
			gain_ = 1.0;
	}

	virtual ~IIRBiquad() { }

	/** Zero delay line. */
	virtual void init(void)
	{
		std::fill(DelayLine_.begin(), DelayLine_.end(), T(0));
	}

	/**
	 * Drop w(n-2) for next cycle.
	 * index==0 is w(n). index==1 is w(n-1) and index==2 w(n-2).
	 */
	virtual void shift(const T &In)
	{
		DelayLine_.push_front(In);
		DelayLine_.pop_back();
	}

	/** Compute an output using direct form II.
	 * w(n) = gain x(n) - a1 w(n-1) - a2 w(n-2)
	 * y(n) = b0 w(n) + b1 w(n-1) + b2 w(n-2)
	 */
	virtual T compute(void)
	{
		T w = gain_ * In_ - a1_ * DelayLine_[0] - a2_ * DelayLine_[1];
		T y =   b0_ * w   + b1_ * DelayLine_[0] + b2_ * DelayLine_[1];
		shift(w);
		return y;
	}

	/** Filter operation. */
	virtual T filter(const T &In)
	{
		In_ = In;
		T out = compute();
		return out;
	}

protected:
	std::deque<T> DelayLine_;
	T In_;
	T b0_;
	T b1_;
	T b2_;
	T a1_;
	T a2_;
	T gain_;

private:
	IIRBiquad(); // disabled
};


/**
 * IIR biquad filter with fixed-point modeling.
 */
template <
	class T_MULT_GAIN=double,
	class T_MULT_FB=double,
	class T_W=double,
	class T_MULT_FF=double,
	class T_OUT=double>
class IIRBiquadQ : public IIRBiquad<double>
{
public:
	/** Define coefficients.
	 * Coefficient vector: [b0 b1 b2 a0 a1 a2 gain]
	 * a0 is assumed to be 1.0.
	 * If gain is missing, it is assumed to be 1.0.
	 */
	IIRBiquadQ(const std::vector<double> &Coeffs)
	: IIRBiquad<double>(Coeffs)
	{ }

	virtual ~IIRBiquadQ() { }

	/** Compute an output using direct form II.
	 * w(n) = gain x(n) - a1 w(n-1) - a2 w(n-2)
	 * y(n) = b0 w(n) + b1 w(n-1) + b2 w(n-2)
	 *
	 * Assumes input and coefficients are already quantized.
	 */
	virtual double compute(void)
	{
		//T w = gain_ * In_ - a1_ * DelayLine_[0] - a2_ * DelayLine_[1];
		//T y =   b0_ * w   + b1_ * DelayLine_[0] + b2_ * DelayLine_[1];
		T_MULT_GAIN mg = gain_ * In_;
		T_MULT_FB mfb1 = a1_ * DelayLine_[0];
		T_MULT_FB mfb2 = a2_ * DelayLine_[1];
		T_W w = mg - mfb1 - mfb2;

		T_MULT_FF mff1 = b0_ * (double)w;
		T_MULT_FF mff2 = b1_ * DelayLine_[0];
		T_MULT_FF mff3 = b2_ * DelayLine_[1];
		T_OUT y = mff1 + mff2 + mff3;

		shift((double)w);
		return (double)y;
	}

protected:

private:
};

#endif
