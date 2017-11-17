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


class User {
public:
	string host;
	string user;
public:
	User () { /* Do nothing. */ };
	User (string a_host, string a_user) {
		host = a_host;
		user = a_user;
	};
	bool operator < (const User &r) const {
		return host < r.host || (host == r.host && user < r.user);
	};
};


static vector <User> get_users ()
{
	vector <User> users;
	string query = string {"http://distsn.org/cgi-bin/distsn-user-recommendation-api.cgi?10000"};
	cerr << query << endl;
	string reply = http_get (query);
	picojson::value json_value;
	string error = picojson::parse (json_value, reply);
	if (! error.empty ()) {
		cerr << error << endl;
		exit (1);
	}
	auto user_jsons = json_value.get <picojson::array> ();
	for (auto user_json: user_jsons) {
		auto user_object = user_json.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string username = user_object.at (string {"username"}).get <string> ();
		users.push_back (User {host, username});
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
			auto words = get_words (user.host, user.user, word_length, vocabulary_size);
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


static void save_union_of_history (unsigned int word_length, unsigned int vocabulary_size)
{
	map <User, set <string>> users_to_words;
	for (unsigned int cn = 0; cn < history_variations; cn ++) {
		stringstream filename;
		filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << "." << cn << ".json";
		try {
			vector <UserAndWords> users_and_words = read_storage (filename.str ());
			for (auto user_and_words: users_and_words) {
				User user {user_and_words.host, user_and_words.user};
				set <string> words {user_and_words.words.begin (), user_and_words.words.end ()};
				if (users_to_words.find (user) == users_to_words.end ()) {
					users_to_words.insert (pair <User, set <string>> {user, words});
				} else {
					users_to_words.at (user).insert (words.begin (), words.end ());
				}
			}
		} catch (ModelException e) {
			cerr << "ModelException " << e.line << " " << endl;
		}
	}
	vector <pair <User, vector <string>>> users_and_words;
	for (auto user_and_words: users_to_words) {
		User user = user_and_words.first;
		vector <string> words {user_and_words.second.begin (), user_and_words.second.end ()};
		users_and_words.push_back (pair <User, vector <string>> {user, words});
	}
	stringstream filename;
	filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << ".json";
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
		save_union_of_history (word_length, vocabulary_size);
	} else {
		cerr << "Too few arguments " << __LINE__ << endl;
		return 1;
	}
}


