.PHONY: all docs firmware clean test clean build

all: docs

docs:
	$(MAKE) -C docs/ html

docs-clean:
	rm -R docs/_build

firmware:
	cd embedded/firmware; platformio run -t upload

sa-debug:
	smartanthill --logger.level=DEBUG

cc-debug:
	twistd -n --pidfile=twistd-cc.pid smartanthill-cc

dashboard-build:
	cd smartanthill/dashboard/site; grunt build

dashboard-debug:
	cd smartanthill/dashboard/site; grunt serve

dashboard-clean:
	cd smartanthill/dashboard/site; grunt clean

dashboard-beautify-js:
	cd smartanthill/dashboard/site/app; find . -name '*.js' -exec js-beautify -nr -s 2 {} +

test:
	tox
	cd smartanthill/dashboard/site; grunt test

py-clean:
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	rm twisted/plugins/dropin.cache

build:
	make dashboard-build
	python setup.py sdist

clean: docs-clean py-clean dashboard-clean
