#ifndef DISTSN_H
#define DISTSN_H


#include <curl/curl.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sys/file.h>
#include <unistd.h>

#include <tinyxml2.h>

#include "picojson.h"


class ExceptionWithLineNumber: public std::exception {
public:
	unsigned int line;
public:
	ExceptionWithLineNumber () {
		line = 0;
	};
	ExceptionWithLineNumber (unsigned int a_line) {
		line = a_line;
	};
};


class TootException: public ExceptionWithLineNumber {
public:
	TootException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
	TootException () { };
};


class UserException: public ExceptionWithLineNumber {
public:
	UserException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
	UserException () { };
};


class ModelException: public ExceptionWithLineNumber {
public:
	ModelException () { };
	ModelException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
};


class ParseException: public ExceptionWithLineNumber {
public:
	ParseException () { };
	ParseException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
};


class UserAndWords {
public:
	std::string user;
	std::string host;
	std::vector <std::string> words;
};


class UserAndSpeed {
public:
	std::string host;
	std::string username;
	double speed;
	bool blacklisted;
public:
	UserAndSpeed (std::string a_host, std::string a_username, double a_speed, bool a_blacklisted) {
		host = a_host;
		username = a_username;
		speed = a_speed;
		blacklisted = a_blacklisted;
	};
};


class User {
public:
	std::string host;
	std::string user;
public:
	User () { /* Do nothing. */ };
	User (std::string a_host, std::string a_user) {
		host = a_host;
		user = a_user;
	};
	bool operator < (const User &r) const {
		return host < r.host || (host == r.host && user < r.user);
	};
};


class Profile {
public:
	std::string screen_name;
	std::string bio;
	std::string avatar;
	std::string type;
	std::string url;
};


class FileLock {
public:
	int fd;
	std::string path;
public:
	FileLock (std::string a_path, int operation = LOCK_EX);
	~FileLock ();
};


std::string escape_json (std::string in);
std::vector <std::string> get_words_from_toots
	(std::vector <std::string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size);
std::vector <std::string> get_words_from_toots
	(std::vector <std::string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size,
	std::map <std::string, unsigned int> word_to_popularity,
	unsigned int minimum_popularity);
double get_rarity (unsigned int occupancy);

std::vector <UserAndWords> read_storage (std::string filename);

std::vector <std::vector <std::string>> parse_csv (FILE *in);
std::string escape_csv (std::string in);
std::string escape_utf8_fragment (std::string in);

std::map <User, Profile> read_profiles ();
bool safe_url (std::string url);
std::string fetch_cache (std::string a_host, std::string a_user, bool & a_hit);

std::set <User> get_optouted_users ();


/* sort-user-speed.cpp */
std::vector <UserAndSpeed> get_users_and_speed ();
std::vector <UserAndSpeed> get_users_and_speed_impl (double limit);
std::set <User> get_blacklisted_users ();


#endif /* #ifndef DISTSN_H */

