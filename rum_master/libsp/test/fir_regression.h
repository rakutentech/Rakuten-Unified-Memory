#ifndef FIR_REGRESSION_H_
#define FIR_REGRESSION_H_

#include "fir.h"


typedef std::complex<double> cxdouble;

/**
 * Entry function that will run all tests.
 */
void regress_fir(void);


void regress_fir_double(void);
template <class T>
void
regress_fir_do(
		FIRBasic<T> &firBasic,
		const std::vector<T> &coeff,
		const std::vector<T> &in,
		const std::string &fileNamePrefix
		);

void regress_fir_complex_double(void);

void regress_fir_manual_phase(const size_t phase);
void regress_fir_automatic_phase(const size_t mainTapIndex);

template <class T>
void write_to(const std::string &fileName, std::vector<T> data);

double urand(void);

#endif /* FIR_REGRESSION_H_ */
