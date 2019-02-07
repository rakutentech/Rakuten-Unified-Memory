/**\file
 * Header file for IIR regression routines.
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef IIR_REGRESSION_H_
#define IIR_REGRESSION_H_

#include <vector>

/**
 * Main entry point for regression.
 */
void regress_iirbiquad(void);

void regress_iirbiquad_float(const std::vector<double> &coeffs, const std::string &prefix);
void regress_iirbiquad_fixed(const std::vector<double> &coeffs, const std::string &prefix);

#endif
