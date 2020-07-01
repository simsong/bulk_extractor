#
# https://www.gnu.org/software/automake/manual/html_node/Parallel-Test-Harness.html

EXTRA_DIST = README.md alert_list.txt find_list.txt redlist.txt banner.txt stop_list.txt stop_list_context.txt http_test.py regress.py Data/README.txt

# We write tests is a variety of langauges
PYTHON=python3
TESTS = test_pdf1.py
TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)
AM_PY_LOG_FLAGS = -v

regress:
	@echo Making regression tests
	python3 regress.py --download
	python3 regress.py --datacheck
	python3 regress.py
	python3 regress.py --fast
	python3 regress.py --full

clean-local:
	-rm -rf regress-*/

