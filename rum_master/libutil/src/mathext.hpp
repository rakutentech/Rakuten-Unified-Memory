/**
 * A collection of math utility routines.
 */

#ifndef MATHEXT_HPP_
#define MATHEXT_HPP_

#include <cstdlib>


class MathExt
{
public:

/** Is power of 2
 * Recall the bit representation of an unsigned integer, x. When x is a power 
 * of 2, only one bit is a 1 and all other bits are 0's. Now think about x-1.
 * x-1 would have 1's at all bit positons least significant than the "1" in x.
 * Therefore, if we bitwise-and x and (x-1), the result is 0 for all power-of-2
 * integers. x == 0 is a special case and is not a power-of-2.
 * 
 * Complexity: O(1)
 * 
 * \return True if input is a power of 2.
 */
static bool
isPowerOf2(size_t x)
{
  return (x > 0 && !(x & (x - 1)));
}

/**
 * Mask rand() to N bits
 */
static int
rand_width(size_t n_bits)
{
    unsigned int mask = (1UL << n_bits) - 1;
    return rand() & mask;
}

protected:
private:

}; // class MathExt


#endif // MATHEXT_HPP_
