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
bindir = /usr/local/bin

objects = \
	tinyxml2.o \
	distsn.o \
	user-words-cron.o \
	user-match-api.o \
	user-avatar-cron.o
cgi_binaries = \
	vinayaka-user-match-api.cgi
cron_binaries = \
	vinayaka-user-words-impl \
	vinayaka-user-avatar-cron
binaries = $(cgi_binaries) $(cron_binaries)
cgi_scripts =
cron_scripts = \
	vinayaka-user-words-meta-cron
cgis = $(cgi_binaries) $(cgi_scripts)
crons = $(cron_binaries) $(cron_scripts)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

all: $(binaries)

vinayaka-user-words-impl: tinyxml2.o distsn.o user-words-cron.o
	$(CXX) tinyxml2.o distsn.o user-words-cron.o -lcurl $(CFLAGS) -o $@

vinayaka-user-match-api.cgi: tinyxml2.o distsn.o user-match-api.o
	$(CXX) tinyxml2.o distsn.o user-match-api.o -lcurl $(CFLAGS) -o $@

vinayaka-user-avatar-cron: tinyxml2.o distsn.o user-avatar-cron.o
	$(CXX) tinyxml2.o distsn.o user-avatar-cron.o -lcurl $(CFLAGS) -o $@

install: $(cgis)
	$(INSTALL_PROGRAM) $(cgis) --target-directory=$(cgidir)
	$(INSTALL_PROGRAM) $(crons) --target-directory=$(bindir)

uninstall:
	-$(RM) $(addprefix $(bindir)/,$(crons))
	-$(RM) $(addprefix $(cgidir)/,$(cgis))

initialize:
	$(MKDIR) -p /var/lib/vinayaka
	$(CHMOD) -R a+rwx /var/lib/vinayaka
	echo 'lock' > /var/lib/vinayaka/lock
	$(CHMOD) a+rw /var/lib/vinayaka/lock

refresh:
	-$(RM) /var/lib/vinayaka/*.json

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


