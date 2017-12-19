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
etcdir = /etc/vinayaka

objects = \
	tinyxml2.o \
	distsn.o \
	user-words-cron.o \
	user-match-api.o \
	user-avatar-cron.o
cgi_binaries = \
	vinayaka-user-speed-api.cgi
cron_binaries = \
	vinayaka-user-avatar-cron \
	vinayaka-user-speed-cron
impl_binaries = \
	vinayaka-user-words-impl \
	vinayaka-user-match-impl \
	vinayaka-user-match-cache-fetcher \
	vinayaka-model-collector-impl \
	vinayaka-super-model-collector-impl \
	vinayaka-model-abstracter-impl
impl_scripts = \
	vinayaka-user-match-resource-guard
binaries = $(cgi_binaries) $(cron_binaries) $(impl_binaries)
cgi_scripts = \
	vinayaka-user-match-api.cgi \
	vinayaka-user-profiles-api.cgi
cron_scripts = \
	vinayaka-user-words-meta-cron \
	vinayaka-clear-cache-cron \
	vinayaka-model-collector-cron
cgis = $(cgi_binaries) $(cgi_scripts)
crons = $(cron_binaries) $(cron_scripts)
impls = $(impl_binaries) $(impl_scripts)
etcs = blacklisted_users.csv

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

all: $(binaries)

vinayaka-user-words-impl: tinyxml2.o distsn.o user-words-cron.o sort-user-speed.o
	$(CXX) tinyxml2.o distsn.o user-words-cron.o sort-user-speed.o -lcurl $(CFLAGS) -o $@

vinayaka-user-match-impl: tinyxml2.o distsn.o user-match-api.o sort-user-speed.o
	$(CXX) tinyxml2.o distsn.o user-match-api.o  sort-user-speed.o -lcurl $(CFLAGS) -o $@

vinayaka-user-avatar-cron: tinyxml2.o distsn.o user-avatar-cron.o sort-user-speed.o
	$(CXX) tinyxml2.o distsn.o user-avatar-cron.o sort-user-speed.o -lcurl $(CFLAGS) -o $@

vinayaka-user-match-cache-fetcher: tinyxml2.o distsn.o cache-fetcher.o
	$(CXX) tinyxml2.o distsn.o cache-fetcher.o -lcurl $(CFLAGS) -o $@

vinayaka-model-collector-impl: tinyxml2.o distsn.o model-collector.o sort-user-speed.o
	$(CXX) tinyxml2.o distsn.o model-collector.o sort-user-speed.o -lcurl $(CFLAGS) -o $@

vinayaka-super-model-collector-impl: tinyxml2.o distsn.o super-model-collector.o
	$(CXX) tinyxml2.o distsn.o super-model-collector.o -lcurl $(CFLAGS) -o $@

vinayaka-model-abstracter-impl: tinyxml2.o distsn.o model-abstracter.o
	$(CXX) tinyxml2.o distsn.o model-abstracter.o -lcurl $(CFLAGS) -o $@

vinayaka-user-speed-cron: tinyxml2.o distsn.o user-speed-cron.o
	$(CXX) tinyxml2.o distsn.o user-speed-cron.o -lcurl $(CFLAGS) -o $@

vinayaka-user-speed-api.cgi: tinyxml2.o distsn.o sort-user-speed.o user-speed-api.o
	$(CXX) tinyxml2.o distsn.o sort-user-speed.o user-speed-api.o -lcurl $(CFLAGS) -o $@

install: $(cgis)
	$(INSTALL_PROGRAM) $(cgis) --target-directory=$(cgidir)
	$(INSTALL_PROGRAM) $(crons) --target-directory=$(bindir)
	$(INSTALL_PROGRAM) $(impls) --target-directory=$(bindir)
	$(MKDIR) -p $(etcdir)
	$(INSTALL_DATA) $(etcs) --target-directory=$(etcdir)

uninstall:
	-$(RM) $(addprefix $(etcdir)/,$(etcs))
	-$(RMDIR) $(etcdir)
	-$(RM) $(addprefix $(bindir)/,$(impls))
	-$(RM) $(addprefix $(bindir)/,$(crons))
	-$(RM) $(addprefix $(cgidir)/,$(cgis))

initialize:
	$(MKDIR) -p /var/lib/vinayaka
	$(MKDIR) -p /var/lib/vinayaka/user-speed
	$(CHMOD) -R a+rwx /var/lib/vinayaka

refresh:
	-$(RM) /var/lib/vinayaka/*.json
	-$(RM) /var/lib/vinayaka/*.csv

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


