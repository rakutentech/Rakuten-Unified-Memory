## List executables to build
noinst_PROGRAMS = main

## .h files for all templates
HFILES_TEMPLATES = \
	estimator.h \
	complexmultiply.h \
	filter.h \
	fir.h

## .cc files for all templates
CCFILES_TEMPLATES =

## .h files for all non-templates
HFILES = \
	fir_regression.h \
	iir_regression.h

## .cc files for all non-templates
CCFILES = \
	fir_regression.cc \
	iir_regression.cc \
	main.cc

## all the source files that are compiled into the
main_SOURCES = \
	$(HFILES) \
	$(CCFILES)

## These are the files that will get installed into the includes directory
## Should include all .h files and the .cc for all templates.
include_HEADERS = \
	$(HFILES) \
	$(HFILES_TEMPLATES) \
	$(CCFILES_TEMPLATES)
