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
	cache-fetcher.o \
	cache-fetcher-simple.o \
	clear-cache-impl.o \
	distsn.o \
	model-abstracter.o \
	model-collector.o \
	report-cache.o \
	sort-user-speed.o \
	sper-model-collector.o \
	tinyxml2.o \
	user-avatar-cron.o \
	user-match-api.o \
	user-speed-api.o \
	user-speed-cron.o \
	user-words-cron.o
cgi_binaries = \
	vinayaka-user-speed-api.cgi
cron_binaries = \
	vinayaka-user-avatar-cron \
	vinayaka-user-speed-cron \
	vinayaka-store-raw-toots-cron \
	vinayaka-collect-raw-toots-cron
impl_binaries = \
	vinayaka-user-words-impl \
	vinayaka-user-match-impl \
	vinayaka-user-match-cache-fetcher \
	vinayaka-user-match-cache-fetcher-simple \
	vinayaka-model-collector-impl \
	vinayaka-super-model-collector-impl \
	vinayaka-model-abstracter-impl \
	vinayaka-clear-cache-impl
impl_scripts = \
	vinayaka-user-match-resource-guard
binaries = $(cgi_binaries) $(cron_binaries) $(impl_binaries)
cgi_scripts = \
	vinayaka-user-match-api.cgi \
	vinayaka-user-match-simple-api.cgi \
	vinayaka-user-match-sampling-api.cgi \
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

vinayaka-user-match-cache-fetcher-simple: tinyxml2.o distsn.o cache-fetcher-simple.o
	$(CXX) tinyxml2.o distsn.o cache-fetcher-simple.o -lcurl $(CFLAGS) -o $@

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

vinayaka-clear-cache-impl: tinyxml2.o distsn.o clear-cache-impl.o
	$(CXX) tinyxml2.o distsn.o clear-cache-impl.o -lcurl $(CFLAGS) -o $@

vinayaka-store-raw-toots-cron: tinyxml2.o distsn.o sort-user-speed.o store-raw-toots.o
	$(CXX) tinyxml2.o distsn.o sort-user-speed.o store-raw-toots.o -lcurl $(CFLAGS) -o $@

vinayaka-collect-raw-toots-cron: tinyxml2.o distsn.o sort-user-speed.o collect-raw-toots.o
	$(CXX) tinyxml2.o distsn.o sort-user-speed.o collect-raw-toots.o -lcurl $(CFLAGS) -o $@

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
	$(MKDIR) -p /var/lib/vinayaka/raw-toots
	$(CHMOD) -R a+rwx /var/lib/vinayaka

refresh:
	-$(RM) /var/lib/vinayaka/*.json
	-$(RM) /var/lib/vinayaka/*.csv
	-$(RM) /var/lib/vinayaka/user-speed/*
	-$(RM) /var/lib/vinayaka/raw-toots/*.csv

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


