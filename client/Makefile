.PHONY: install uninstall

srcdir = .

htmldir = /var/www/html

VPATH = $(srcdir)

SHELL = /bin/sh
RM = /bin/rm -vf
INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m a=rx
INSTALL_DATA = $(INSTALL) -m a=r
MKDIR = mkdir -p
RMDIR = rmdir

www_files = \
	index.html \
	fake-mastodon.css \
	kasaikuminkan.jpg \
	google214b9ca3f27d115d.html \
	user-match.js \
	missing.svg \
	user-speed.html \
	user-speed.js \
	ja.html \
	ja.js \
	missing.png \
	blacklisted.png \
	user-speed-ja.html \
	user-new.html \
	user-new.js


install: $(www_files)
	$(INSTALL_DATA) $(www_files) --target-directory=$(htmldir)

uninstall:
	-$(RM) $(addprefix $(htmldir)/,$(www_files))


