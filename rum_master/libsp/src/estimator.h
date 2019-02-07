/**\file
 * \brief Estimator classes for random variables.
 *
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef ESTIMATOR_H_
#define ESTIMATOR_H_

#include <cmath>
#include <cstdlib>
#include <complex>


/**Protocol of an estimator
 * Technique: abstract base class
 */
template<class T>
class Estimator
{
public:
	Estimator() { }
	virtual ~Estimator() { }
protected:
	virtual void reset(void) = 0;
	virtual void record(const T &x) = 0;
	virtual T estimate(void) = 0;
	virtual size_t count(void) = 0;
private:
};


/**A sample mean estimator
 * For scalar random variable and does not support vector random variables.
 * Reference: http://mathworld.wolfram.com/SampleMean.html
 */
template<class T>
class MeanEstimator : public Estimator<T>
{
public:
	MeanEstimator()
	: accumulator_(T(0))
	, count_(0)
	{ }
	
	virtual void record(const T &x)
	{
		accumulator_ = accumulator_ + x;
		count_++;
	}
	
	virtual T estimate(void)
	{
		if (count_ == 0)
			return T(0);
		return accumulator_ / static_cast<T>(count_);
	}
	
	virtual void reset(void)
	{
		accumulator_ = T(0);
		count_ = 0;
	}
	
	virtual size_t count(void)
	{
		return count_;
	}
	
protected:
	T accumulator_;
	size_t count_;
private:
};


/**A sample variance estimator
 * Can be unbiased or biased.
 * Reference: http://mathworld.wolfram.com/SampleVariance.html
 */
template<class T>
class VarianceEstimator : public MeanEstimator<T>
{
public:
	VarianceEstimator()
	: unbiased_(true)
	, mean_(T(0))
	{ }
	
	VarianceEstimator(const T &mean, bool unbiased = true)
	: unbiased_(unbiased)
	, mean_(mean)
	{ }
	
	virtual void record(const T &x)
	{
		MeanEstimator<T>::accumulator_ = MeanEstimator<T>::accumulator_ + (x - mean_)*(x - mean_);
		MeanEstimator<T>::count_++;
	}
	
	virtual T estimate(void)
	{
		if (MeanEstimator<T>::count_ == 0)
			return T(0);

		if (unbiased_)
			return MeanEstimator<T>::accumulator_ / static_cast<T>(MeanEstimator<T>::count_ - 1);
		else
			return MeanEstimator<T>::accumulator_ / static_cast<T>(MeanEstimator<T>::count_);
	}
	
	virtual void reset(void)
	{
		MeanEstimator<T>::reset();
		mean_ = T(0);
	}


	
protected:
	const bool unbiased_;
	T mean_;
private:
};


/**A sample variance estimator of complex<T> type.
 * Can be unbiased or biased.
 * Reference: http://mathworld.wolfram.com/SampleVariance.html
 * Also see: VarianceEstimator<T>
 * Technique: partial template specialization
 */
template<class T>
class VarianceEstimator< std::complex<T> > : public MeanEstimator<T>
{
public:
	VarianceEstimator()
	: unbiased_(true)
	, mean_(std::complex<T>(0,0))
	{ }
	
	VarianceEstimator(const std::complex<T> &mean, bool unbiased = true)
	: unbiased_(unbiased)
	, mean_(mean)
	{ }
	
	virtual void record(const std::complex<T> &x)
	{
		MeanEstimator<T>::accumulator_ = MeanEstimator<T>::accumulator_ + std::norm(x - mean_);
		MeanEstimator<T>::count_++;
	}
	
	virtual T estimate(void)
	{
		if (MeanEstimator<T>::count_ == 0)
			return NAN;

		if (unbiased_)
			return MeanEstimator<T>::accumulator_ / static_cast<T>(MeanEstimator<T>::count_ - 1);
		else
			return MeanEstimator<T>::accumulator_ / static_cast<T>(MeanEstimator<T>::count_);
	}
	
	virtual void reset(void)
	{
		MeanEstimator<T>::reset();
		mean_ = std::complex<T>(0,0);
	}

protected:
	const bool unbiased_;
	std::complex<T> mean_;
private:
};







#endif