/**\file
 * \brief Generator of test data for fir.h regression.
 *
 * Will generate data set for regression test in MATLAB.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#include "fir.h"
#include "fir_regression.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>


void regress_fir(void)
{
	std::cout << __func__ << "..." << std::endl;

	regress_fir_double();
	regress_fir_complex_double();

	regress_fir_manual_phase(0);
	regress_fir_manual_phase(1);
	regress_fir_manual_phase(2);

	regress_fir_automatic_phase(4);
	regress_fir_automatic_phase(5);
	regress_fir_automatic_phase(6);
	regress_fir_automatic_phase(7);
}


void regress_fir_double(void)
{
	std::cout << __func__ << "..." << std::endl;

	const size_t coeffLength = 13; // center tap is the main tap when decimation by 3
	// going to use same coeff for filter(), decimate() and interp()
	std::vector<double> coeff(coeffLength);
	// create a symmetric filter
	for (size_t i = 0; i < coeff.size()/2; i++)
	{
		coeff[i] = urand();
		coeff[coeff.size()-1 - i] = coeff[i];
	}
	if (coeffLength % 2)
		coeff[coeffLength/2] = urand();
	std::cout << "coeffs are:" << std::endl;
	for (size_t i = 0; i < coeff.size(); i++)
		std::cout << i << " " << coeff[i] << std::endl;

	// going to use same input for filter(), decimate() and interp()
	std::vector<double> in(coeffLength*30);
	for (size_t i = 0; i < in.size(); i++)
		in[i] = urand();



	FIRBasic<double> firBasic(coeff);
	regress_fir_do(firBasic, coeff, in, "regress_fir_basic_");

	FIR<double,double,double,double> fir(coeff);
	regress_fir_do(fir, coeff, in, "regress_fir_");

	FIRSymmetric<double,double,double,double,double> firSymmetric(coeff);
	regress_fir_do(firSymmetric, coeff, in, "regress_fir_symmetric_");

}

template <class T>
void
regress_fir_do(
		FIRBasic<T> &firBasic,
		const std::vector<T> &coeff,
		const std::vector<T> &in,
		const std::string &fileNamePrefix
		)
{
	std::cout << "Main tap index: " << firBasic.main_tap_index() << std::endl;
	write_to<T>(fileNamePrefix + "coeff.txt", coeff);
	write_to<T>(fileNamePrefix + "in.txt", in);

	// Test filter()
	std::vector<T> out(in.size());
	for (size_t i = 0; i < out.size(); i++)
		out[i] = firBasic.filter(in[i]);
	write_to<T>(fileNamePrefix + "filter.txt", out);
	firBasic.init(); // clear delay line

	// Test decimate()
	out.clear();
	std::vector<T> in3(3);
	for (size_t i = 0; i < in.size(); i++)
	{
		switch (i%3) {
		case 0:
			in3[0] = in[i];
			break;
		case 1:
			in3[1] = in[i];
			break;
		case 2:
			in3[2] = in[i];
			out.push_back(firBasic.decimate(in3));
			break;
		default:
			throw std::exception();
		}
	}
	write_to<T>(fileNamePrefix + "decimate.txt", out);
	firBasic.init(); // clear delay line

	// Test interp()
	const size_t rate = 3;
	std::vector<T> interpOut(rate);
	std::vector<T> allOut;
	for (size_t i = 0; i < in.size(); i++) {
		interpOut = firBasic.interp(in[i], rate);
		for (size_t r = 0; r < rate; r++)
			allOut.push_back(interpOut[r]);
	}
	write_to<T>(fileNamePrefix + "interp.txt", allOut);
	firBasic.init(); // clear delay line

}


void regress_fir_complex_double(void)
{
	std::cout << __func__ << "..." << std::endl;

	const size_t coeffLength = 13; // center tap is the main tap when decimation by 3
	// going to use same coeff for filter(), decimate() and interp()
	std::vector<cxdouble> coeff(coeffLength);
	// create a symmetric filter
	for (size_t i = 0; i < coeff.size()/2; i++)
	{
		coeff[i] = cxdouble(urand(), urand());
		coeff[coeff.size()-1 - i] = coeff[i];
	}
	if (coeffLength % 2)
		coeff[coeffLength/2] = cxdouble(urand(), urand());
	std::cout << "coeffs are:" << std::endl;
	for (size_t i = 0; i < coeff.size(); i++)
		std::cout << i << " " << coeff[i] << std::endl;

	// going to use same input for filter(), decimate() and interp()
	std::vector<cxdouble> in(coeffLength*30);
	for (size_t i = 0; i < in.size(); i++)
		in[i] = cxdouble(urand(), urand());



	FIRBasic<cxdouble> firBasic(coeff);
	regress_fir_do(firBasic, coeff, in, "regress_fir_basic_cx_");

	CxFIR<double,double,double,double> fir(coeff);
	regress_fir_do(fir, coeff, in, "regress_fir_cx_");

// Not implemented yet
//	CxFIRSymmetric<double,double,double,double,double> firSymmetric(coeff);
//	regress_fir_do(firSymmetric, coeff, in, "regress_fir_symmetric_cx_");

}

/** Test phase selection */
void regress_fir_manual_phase(const size_t phase)
{
	std::cout << __func__ << "..." << std::endl;
	std::cout << "going to test phase=" << phase << std::endl;

	const size_t coeffLength = 13; // center tap is the main tap when decimation by 3
	std::vector<double> coeff(coeffLength);
	// create a symmetric filter
	for (size_t i = 0; i < coeff.size(); i++)
		coeff[i] = i+1;
	std::cout << "coeffs are:";
	for (size_t i = 0; i < coeff.size(); i++)
		std::cout << coeff[i] << " ";
	std::cout << std::endl;

	// create impulse
	std::cout << "Input is an impulse" << std::endl;
	std::vector<double> in(coeffLength, 0.0);
	in[0] = 1.0;

	FIRBasic<double> fir(coeff);

	std::vector<double> out;
	std::vector<double> in3(3);
	for (size_t i = 0; i < in.size(); i++)
	{
		switch (i%3) {
		case 0:
			in3[0] = in[i];
			break;
		case 1:
			in3[1] = in[i];
			break;
		case 2:
			in3[2] = in[i];
			out.push_back(fir.decimate(in3, phase)); // by 3
			break;
		default:
			throw std::exception();
		}
	}

	std::cout << "outputs are: ";
	for (size_t i = 0; i < out.size(); i++)
		std::cout << out[i] << " ";
	std::cout << std::endl;

}

/** Test automatic phase */
void regress_fir_automatic_phase(const size_t mainTapIndex)
{
	std::cout << __func__ << "..." << std::endl;
	std::cout << "going to test mainTapIndex=" << mainTapIndex << std::endl;

	const size_t coeffLength = 13; // center tap is the main tap when decimation by 3
	std::vector<double> coeff(coeffLength);
	// create a symmetric filter
	for (size_t i = 0; i < coeff.size(); i++)
		coeff[i] = i+1;
	std::cout << "coeffs are:";
	for (size_t i = 0; i < coeff.size(); i++)
		std::cout << coeff[i] << " ";
	std::cout << std::endl;

	// create impulse
	std::cout << "Input is an impulse" << std::endl;
	std::vector<double> in(coeffLength, 0.0);
	in[0] = 1.0;

	FIRBasic<double> fir(coeff, mainTapIndex);

	std::vector<double> out;
	std::vector<double> in3(3);
	for (size_t i = 0; i < in.size(); i++)
	{
		switch (i%3) {
		case 0:
			in3[0] = in[i];
			break;
		case 1:
			in3[1] = in[i];
			break;
		case 2:
			in3[2] = in[i];
			out.push_back(fir.decimate(in3)); // by 3
			break;
		default:
			throw std::exception();
		}
	}

	std::cout << "outputs are: ";
	for (size_t i = 0; i < out.size(); i++)
		std::cout << out[i] << " ";
	std::cout << std::endl;

}

template <class T>
void write_to(const std::string &fileName, std::vector<T> data)
{
	std::ofstream fp;
	fp.open(fileName.c_str());
	if (!fp) {
		std::cerr << "Unable to open " << fileName << " for writing!" << std::endl;
		throw std::exception();
	}
	for (size_t i = 0; i < data.size(); i++)
		fp << data[i] << std::endl;
	fp.close();
}

double urand(void)
{
	return std::rand() / double (RAND_MAX);
}
