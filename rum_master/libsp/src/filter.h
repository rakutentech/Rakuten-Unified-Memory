/**\file
 * \brief Abstract base class of a filter
 *
 * \author Rick Lan
 * \copyright See LICENSE for license.
 */

#ifndef FILTER_H_
#define FILTER_H_

#include <vector>

/**Abstract base class of a filter
 *
 * Defines interfaces of a filter for derived classes:
 * - Stores coefficients.
 * - Computes an output.
 * - Updates memory in the filter.
 */
template<class T>
class Filter
{
public:
	Filter(const std::vector<T> &Coeffs)
	: Coeffs_(Coeffs)
	{ }

	virtual ~Filter()
	{ }

protected:
	const std::vector<T> Coeffs_;

	/** Update memory in the filter. */
	virtual void shift(const T &In) = 0;

	/** Compute an output. */
	virtual T compute(void) = 0;

	/** Do filter operation. */
	virtual T filter(const T &In) = 0;

private:
	Filter(); // disabled
};



#endif /* FILTER_H_ */
