#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <random>
#include "picojson.h"
#include "tinyxml2.h"
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


static vector <User> get_users ()
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < 20000; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


static void write_to_storage (vector <pair <User, vector <string>>> users_and_words, ofstream &out)
{
	out << "[";
	for (unsigned int a = 0; a < users_and_words.size (); a ++) {
		if (0 < a) {
			out << ",";
		}
		auto user_and_words = users_and_words.at (a);
		auto user = user_and_words.first;
		auto words = user_and_words.second;
		out
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"words\":"
			<< "[";
			for (unsigned int b = 0; b < words.size (); b ++) {
				if (0 < b) {
					out << ",";
				}
				auto word = words.at (b);
				out << "\"" << escape_json (word) << "\"";
			}
		out << "]" << "}";
	}
	out << "]";
}


static const unsigned int history_variations = 24;

	
static void get_and_save_words (unsigned int word_length, unsigned int vocabulary_size, vector <User> users)
{
	vector <pair <User, vector <string>>> users_and_words;
	for (auto user: users) {
		try {
			auto words = get_words (false /* pagenation */, user.host, user.user, word_length, vocabulary_size);
			users_and_words.push_back (pair <User, vector <string>> {user, words});
		} catch (UserException e) {
			cerr << "Error " << user.user << "@" << user.host << " " << e.line << endl;
		}
	}
	
	random_device device;
	auto random_engine = default_random_engine (device ());
	uniform_int_distribution <unsigned int> distribution (0, history_variations - 1);
	unsigned int random_number = distribution (random_engine);
	
	stringstream filename;
	filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << "." << random_number << ".json";
	ofstream out {filename.str ()};
	write_to_storage (users_and_words, out);
}


int main (int argc, char **argv)
{
	if (2 < argc) {
		stringstream word_length_s {argv [1]};
		unsigned int word_length;
		word_length_s >> word_length;
		stringstream vocabulary_size_s {argv [2]};
		unsigned int vocabulary_size;
		vocabulary_size_s >> vocabulary_size;
		auto users = get_users ();
		get_and_save_words (word_length, vocabulary_size, users);
	} else {
		cerr << "Too few arguments " << __LINE__ << endl;
		return 1;
	}
}


