.PHONY: all clean install uninstall initialize refresh-all refresh-cache activate-cgi

all:
	-$(MAKE) all -C server

clean:
	-$(MAKE) clean -C server

install:
	-$(MAKE) install -C client
	-$(MAKE) install -C server

uninstall:
	-$(MAKE) uninstall -C client
	-$(MAKE) uninstall -C server

initialize:
	-$(MAKE) initialize -C server

refresh-all:
	-$(MAKE) refresh-all -C server

refresh-cache:
	-$(MAKE) refresh-cache -C server

activate-cgi:
	-$(MAKE) activate-cgi -C server

