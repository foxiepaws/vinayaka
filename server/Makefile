.PHONY: all clean install uninstall initialize refresh activate-cgi

srcdir = .

CFLAGS = -std=c++14 -Wall -Wpointer-arith -O2 -fno-builtin -g

VPATH = $(srcdir)

SHELL = /bin/sh
RM = /bin/rm -vf
INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m a=rx
INSTALL_DATA = $(INSTALL) -m a=r
MKDIR = mkdir
RMDIR = rmdir
CHMOD = chmod

cgidir = /usr/lib/cgi-bin

objects = \
	tinyxml2.o \
	distsn.o \
	user-words-cron.o \
	user-match-api.o
cgi_binaries = \
	vinayaka-user-match-api.cgi
cron_binaries = \
	vinayaka-user-words-cron
binaries = $(cgi_binaries) $(cron_binaries)
cgi_scripts =
cgis = $(cgi_binaries) $(cgi_scripts)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

all: $(binaries)

vinayaka-user-words-cron: tinyxml2.o distsn.o user-words-cron.o
	$(CXX) tinyxml2.o distsn.o user-words-cron.o -lcurl $(CFLAGS) -o $@

vinayaka-user-match-api.cgi: tinyxml2.o distsn.o user-match-api.o
	$(CXX) tinyxml2.o distsn.o user-match-api.o -lcurl $(CFLAGS) -o $@

install: $(cgis)
	$(INSTALL_PROGRAM) $(cgis) --target-directory=$(cgidir)

uninstall:
	-$(RM) $(addprefix $(cgidir)/,$(cgis))

initialize:
	$(MKDIR) -p /var/lib/vinayaka
	$(CHMOD) -R a+rwx /var/lib/vinayaka

refresh:
	-$(RM) /var/lib/vinayaka/*.json

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


