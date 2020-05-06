EXTRA_DIST = README.txt alert_list.txt find_list.txt redlist.txt banner.txt stop_list.txt stop_list_context.txt http_test.py regress.py Data/README.txt

regress:
	@echo Making regression tests
	python3 regress.py --download
	python3 regress.py --datacheck
	python3 regress.py
	python3 regress.py --fast
	python3 regress.py --full

clean-local:
	-rm -rf regress-*/
