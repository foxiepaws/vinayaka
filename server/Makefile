.PHONY: all clean install uninstall initialize refresh-cache refresh-all activate-cgi

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
TOUCH = touch

cgidir = /usr/lib/cgi-bin
bindir = /usr/local/bin
etcdir = /etc/vinayaka

objects = \
	cache-fetcher.o \
	cache-fetcher-osa.o \
	cache-fetcher-filtered.o \
	cache-fetcher-suggestions.o \
	clear-cache-impl.o \
	distsn.o \
	report-cache.o \
	sort-user-speed.o \
	user-avatar-cron.o \
	user-match-api.o \
	user-speed-api.o \
	user-speed-cron.o \
	store-raw-toots.o \
	collect-raw-toots.o \
	user-new-cron.o \
	user-search-api.o \
	user-new-suggestions-api.o \
	cache-fetcher-misskey.o

cgi_binaries = \
	vinayaka-user-speed-api.cgi \
	vinayaka-user-search-api.cgi \
	vinayaka-user-match-filtered-api.cgi \
	vinayaka-user-match-suggestions-api.cgi \
	vinayaka-user-new-suggestions-api.cgi \
	vinayaka-user-match-misskey-api.cgi

cron_binaries = \
	vinayaka-user-avatar-cron \
	vinayaka-user-speed-cron \
	vinayaka-store-raw-toots-cron \
	vinayaka-collect-raw-toots-cron \
	vinayaka-user-new-cron

impl_binaries = \
	vinayaka-user-match-impl \
	vinayaka-user-match-cache-fetcher \
	vinayaka-user-match-cache-fetcher-osa \
	vinayaka-clear-cache-impl

impl_scripts = \
	vinayaka-user-match-resource-guard

binaries = $(cgi_binaries) $(cron_binaries) $(impl_binaries)

cgi_scripts = \
	vinayaka-user-match-api.cgi \
	vinayaka-user-match-osa-api.cgi \
	vinayaka-user-profiles-api.cgi \
	vinayaka-user-new-api.cgi

cron_scripts = \
	vinayaka-clear-cache-cron \
	vinayaka-https-renew-cron

cgis = $(cgi_binaries) $(cgi_scripts)

crons = $(cron_binaries) $(cron_scripts)

impls = $(impl_binaries) $(impl_scripts)

etcs = blacklisted_users.csv

loading_libraries = -lsocialnet-1 -lcurl -ltinyxml2 -llanguagemodel-1

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

all: $(binaries)

vinayaka-user-match-impl: distsn.o user-match-api.o sort-user-speed.o
	$(CXX) distsn.o user-match-api.o sort-user-speed.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-avatar-cron: distsn.o user-avatar-cron.o sort-user-speed.o
	$(CXX) distsn.o user-avatar-cron.o sort-user-speed.o $(loading_libraries)  $(CFLAGS) -o $@

vinayaka-user-match-cache-fetcher: distsn.o cache-fetcher.o
	$(CXX) distsn.o cache-fetcher.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-cache-fetcher-osa: distsn.o cache-fetcher-osa.o
	$(CXX) distsn.o cache-fetcher-osa.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-filtered-api.cgi: distsn.o cache-fetcher-filtered.o
	$(CXX) distsn.o cache-fetcher-filtered.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-suggestions-api.cgi: distsn.o cache-fetcher-suggestions.o
	$(CXX) distsn.o cache-fetcher-suggestions.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-speed-cron: distsn.o user-speed-cron.o
	$(CXX) distsn.o user-speed-cron.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-speed-api.cgi: distsn.o sort-user-speed.o user-speed-api.o
	$(CXX) distsn.o sort-user-speed.o user-speed-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-clear-cache-impl: distsn.o clear-cache-impl.o
	$(CXX) distsn.o clear-cache-impl.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-store-raw-toots-cron: distsn.o sort-user-speed.o store-raw-toots.o
	$(CXX) distsn.o sort-user-speed.o store-raw-toots.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-collect-raw-toots-cron: distsn.o sort-user-speed.o collect-raw-toots.o
	$(CXX) distsn.o sort-user-speed.o collect-raw-toots.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-new-cron: distsn.o sort-user-speed.o user-new-cron.o
	$(CXX) distsn.o sort-user-speed.o user-new-cron.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-search-api.cgi: distsn.o sort-user-speed.o user-search-api.o
	$(CXX) distsn.o sort-user-speed.o user-search-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-new-suggestions-api.cgi: distsn.o sort-user-speed.o user-new-suggestions-api.o
	$(CXX) distsn.o sort-user-speed.o user-new-suggestions-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-misskey-api.cgi: distsn.o cache-fetcher-misskey.o
	$(CXX) distsn.o cache-fetcher-misskey.o $(loading_libraries) $(CFLAGS) -o $@

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
	$(MKDIR) -p /var/lib/vinayaka/model
	$(MKDIR) -p /var/lib/vinayaka/user-first-toot
	$(TOUCH) /var/lib/vinayaka/match-cache.csv
	$(TOUCH) /var/lib/vinayaka/model/concrete-user-words.csv
	$(TOUCH) /var/lib/vinayaka/model/popularity.csv
	$(TOUCH) /var/lib/vinayaka/users-new-cache.json
	$(CHMOD) -R a+rwx /var/lib/vinayaka

refresh-cache:
	-$(RM) /var/lib/vinayaka/match-cache.csv

refresh-all: refresh-cache
	-$(RM) /var/lib/vinayaka/*.json
	-$(RM) /var/lib/vinayaka/*.csv
	-$(RM) /var/lib/vinayaka/user-speed/*
	-$(RM) /var/lib/vinayaka/raw-toots/*.csv
	-$(RM) /var/lib/vinayaka/model/*.csv
	-$(RM) /var/lib/vinayaka/user-first-toot/*.json

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


