.PHONY: all docs firmware clean test clean

all: docs

docs:
	$(MAKE) -C docs/ html

docs-clean:
	rm -R docs/_build

firmware:
	cd embedded/firmware; platformio run -t upload

sa-debug:
	smartanthill --workspacedir=examples/arduino-router/workspace --logger.level=DEBUG

cc-debug:
	twistd -n --pidfile=twistd-cc.pid smartanthill-cc

dashboard-build:
	cd smartanthill/dashboard/site; grunt build

dashboard-debug:
	cd smartanthill/dashboard/site; grunt serve

dashboard-clean:
	cd smartanthill/dashboard/site; grunt clean

test:
	tox
	cd smartanthill/dashboard/site; grunt test

py-clean:
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	rm twisted/plugins/dropin.cache

clean: docs-clean py-clean dashboard-clean
