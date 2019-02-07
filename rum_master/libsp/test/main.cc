/**\file
 * \brief Run test code.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#include <iostream>
#include <iomanip>
#include <complex>
#include <cmath>

#include "complexmultiply.h"
#include "fir_regression.h"
#include "iir_regression.h"
#include "estimator.h"

void test_cxmult(void)
{
	std::cout << __func__ << "..." << std::endl;
	for (size_t i = 0; i < 8; i++) {
		std::cout << "i " << i << std::endl;
		double realp = std::cos(2.0*M_PI*i/8.0);
		double imagp = std::sin(2.0*M_PI*i/8.0);
		std::complex<double> x(realp, imagp);
		std::complex<double> y = conj(x);
		std::complex<double> z;
		z = cxmult<double, double>(realp, imagp, realp, -imagp);
		std::cout << z << std::endl;
		z = cxmult<double, double>(x, y);
		std::cout << z << std::endl;
		z = cxmult3<double, double, double>(realp, imagp, realp, -imagp);
		std::cout << z << std::endl;
		z = cxmult3<double, double, double>(x, y);
		std::cout << z << std::endl;
	}
}


void test_estimator_real(void)
{
	std::cout << __func__ << "..." << std::endl;
	
	const size_t N = 10;
	std::vector<double> samples(N);
	for (size_t i = 1; i <= N; i++)
		samples[i-1] = i;
	std::cout << "Samples are: ";
	for (size_t i = 1; i <= N; i++)
		std::cout << samples[i-1] << " ";
	std::cout << std::endl;
	
	MeanEstimator<double> me;
	for (size_t i = 1; i <= N; i++)
		me.record(samples[i-1]);
	std::cout << "Mean of " << me.count() << " sample(s) is " << me.estimate() << std::endl;

	VarianceEstimator<double> veu(me.estimate(), true); // unbiased
	VarianceEstimator<double> veb(me.estimate(), false); // biased
	for (size_t i = 1; i <= N; i++) {
		veu.record(samples[i-1]);
		veb.record(samples[i-1]);
	}
	std::cout << "Unbiased variance of " << veu.count() << " sample(s) is " << veu.estimate() << std::endl;
	std::cout << "Biased variance of " << veb.count() << " sample(s) is " << veb.estimate() << std::endl;

	me.reset();
	veu.reset();
	veb.reset();
	std::cout << "After resets:" << std::endl;
	std::cout << "Mean of " << me.count() << " sample(s) is " << me.estimate() << std::endl;
	std::cout << "Unbiased variance of " << veu.count() << " sample(s) is " << veu.estimate() << std::endl;
	std::cout << "Biased variance of " << veb.count() << " sample(s) is " << veb.estimate() << std::endl;
}
	
void test_estimator_complex(void)
{
	std::cout << __func__ << "..." << std::endl;

	const size_t N = 10;
	std::vector< std::complex<double> > samples(N);
	for (size_t i = 1; i <= N; i++)
		samples[i-1] = std::complex<double>(double(i), double(2*i));

	std::cout << "Samples are: ";
	for (size_t i = 1; i <= N; i++)
		std::cout << samples[i-1] << " ";
	std::cout << std::endl;

	MeanEstimator< std::complex<double> > me;
	for (size_t i = 1; i <= N; i++)
		me.record(samples[i-1]);
	std::cout << "Mean of " << me.count() << " sample(s) is " << me.estimate() << std::endl;

	VarianceEstimator< std::complex<double> > veu(me.estimate(), true); // unbiased
	for (size_t i = 1; i <= N; i++) {
		veu.record(samples[i-1]);
	}
	std::cout << "Unbiased variance of " << veu.count() << " sample(s) is " << veu.estimate() << std::endl;

	me.reset();
	veu.reset();
	//veb.reset();
	std::cout << "After resets:" << std::endl;
	std::cout << "Mean of " << me.count() << " sample(s) is " << me.estimate() << std::endl;
	std::cout << "Unbiased variance of " << veu.count() << " sample(s) is " << veu.estimate() << std::endl;
	//std::cout << "Biased variance of " << veb.count() << " sample(s) is " << veb.estimate() << std::endl;
}

void test_estimator(void)
{
	test_estimator_real();
	test_estimator_complex();
}


int main(int argc, char **argv)
{
	test_cxmult();
//	regress_fir();
//	regress_iirbiquad();
	test_estimator();
	
	return 0;
}
