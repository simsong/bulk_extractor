#!/usr/bin/make -f

# This software was developed at the National Institute of Standards
# and Technology by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code this software is not subject to copyright
# protection and is in the public domain. NIST assumes no
# responsibility whatsoever for its use by other parties, and makes
# no guarantees, expressed or implied, about its quality,
# reliability, or any other characteristic.
#
# We would appreciate acknowledgement if the software is used.

SHELL ?= /bin/bash

XMLLINT ?= $(shell which xmllint)
ifeq ($(XMLLINT),)
$(error XMLLINT not found)
endif

SAMPLE_FILES__PASS := \
  difference_test_0.xml \
  difference_test_1.xml \
  difference_test_2.xml \
  difference_test_3.xml \
  fileobjectexample.xml

# TODO Any remaining issues with the upstream tool should be resolved.
SAMPLE_FILES__SKIP := \
  piecewise.xml \
  simple.xml

SAMPLE_FILES := \
  $(SAMPLE_FILES__FAIL) \
  $(SAMPLE_FILES__SKIP)

VALIDATES_LOG_FILES__PASS := $(foreach sample_file,$(SAMPLE_FILES__PASS),$(sample_file).validates.log)

VALIDATES_LOG_FILES__SKIP := $(foreach sample_file,$(SAMPLE_FILES__SKIP),$(sample_file).validates.log)

VALIDATES_LOG_FILES := \
  $(VALIDATES_LOG_FILES__PASS) \
  $(VALIDATES_LOG_FILES__SKIP)

all:

.PHONY: \
  check-TODO

%.validates.log: \
  % \
  ../schema/dfxml.xsd
	$(XMLLINT) \
	  --noout \
	  --schema ../schema/dfxml.xsd \
	  $< \
	  2> $<.err.log
	touch $@

../schema/dfxml.xsd:
	@echo "dfxml.xsd not found.  To check out the DFXML schema (necessary to run unit tests in the /samples directory), please run 'make schema-init' in the repository root." >&2
	exit 2

check: \
  $(VALIDATES_LOG_FILES__PASS)

check-TODO: \
  $(VALIDATES_LOG_FILES__SKIP)

clean:
	@rm -f *.err.log *.validates.log
