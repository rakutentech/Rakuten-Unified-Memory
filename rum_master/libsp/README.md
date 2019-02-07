libsp  [![Build Status](https://travis-ci.org/rlan/libsp.svg?branch=master)](https://travis-ci.org/rlan/libsp)
============
A header-files-only, templated C++ library of signal processing algorithms and their fixed-point model.

## Build

```sh
mkdir -p build && cd build && cmake ../test && make
```

## Documentation
Generate documentation using doxygen:

```sh
doxygen Doxyfile
```

Documentation entry point: `doc/html/index.html`

## TODO

Items in no particular order.

- [x] Complex multiplier
- [x] FIR filter
- [x] FIR filter with symmetric coefficients
- [x] FIR filter with complex coefficients
- [ ] Transposed FIR filter
- [x] Multi-rate FIR filter
- [x] IIR biquad filter
- [x] Estimator (mean and variance)
- [ ] Signal statistics
- [ ] Fixed-point number class
- [ ] Auto fixed-point format computation
- [ ] Use cpp/hpp
- [ ] Use namespace
- [ ] Use Boost::Test
- [ ] travis-ci: test both gcc and clang

## Maintainer

Rick Lan

## License

[BSD 3-clause](LICENSE)
