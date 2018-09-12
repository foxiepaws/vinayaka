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
#include <limits>
#include "picojson.h"
#include "tinyxml2.h"
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


static const unsigned int history_variations = 24;

	
static vector <User> get_users (unsigned int size)
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < size; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


static map <User, set <string>> read_users_to_toots (string filename)
{
	map <User, set <string>> users_to_toots;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		throw (ModelException {__LINE__});
	}
	vector <vector <string>> table;
	try {
		table = parse_csv (in);
	} catch (ParseException e) {
		throw (ModelException {__LINE__});
	}
	for (auto row: table) {
		if (2 < row.size ()) {
			User user {row.at (0), row.at (1)};
			string toot = row.at (2);
			if (users_to_toots.find (user) == users_to_toots.end ()) {
				users_to_toots.insert (pair <User, set <string>> {user, set <string> {toot}});
			} else {
				users_to_toots.at (user).insert (toot);
			}
		}
	}
	return users_to_toots;
}


static map <User, set <string>> get_union_of_history (unsigned int size)
{
	map <User, set <string>> users_to_toots;

	auto users = get_users (size);
	for (auto user: users) {
		users_to_toots.insert (pair <User, set <string>> {user, set <string> {}});
	}

	for (unsigned int cn = 0; cn < history_variations; cn ++) {
		stringstream filename;
		filename << "/var/lib/vinayaka/raw-toots/" << cn << ".csv";
		try {
			map <User, set <string>> users_to_toots_point = read_users_to_toots (filename.str ());
			for (auto user_to_toots_point: users_to_toots_point) {
				User user {user_to_toots_point.first};
				set <string> toots {user_to_toots_point.second};
				if (users_to_toots.find (user) != users_to_toots.end ()) {
					users_to_toots.at (user).insert (toots.begin (), toots.end ());
				}
			}
		} catch (ModelException e) {
			cerr << "ModelException " << e.line << " " << endl;
		}
	}
	
	return users_to_toots;
}


static void save_users_to_toots (map <User, set <string>> users_to_toots)
{
	ofstream out {"/var/lib/vinayaka/raw-toots/all.csv"};
	for (auto user_to_toots: users_to_toots) {
		auto user = user_to_toots.first;
		auto toots = user_to_toots.second;
		for (auto toot: toots) {
			out
				<< "\"" << escape_csv (user.host) << "\","
				<< "\"" << escape_csv (user.user) << "\","
				<< "\"" << escape_csv (toot) << "\"" << endl;
		}
	}
}


static set <string> get_full_words (set <string> toots)
{
	const unsigned int vocabulary_size {6400};
	vector <string> toots_vector {toots.begin (), toots.end ()};
	vector <string> model_6 = get_words_from_toots (toots_vector, 6, vocabulary_size);
	set <string> all;
	all.insert (model_6.begin (), model_6.end ());
	return all;
}


static map <User, set <string>> get_full_words (map <User, set <string>> users_to_toots)
{
	map <User, set <string>> users_to_words;
	for (auto user_to_toots: users_to_toots) {
		User user = user_to_toots.first;
		set <string> toots = user_to_toots.second;
		set <string> words = get_full_words (toots);
		users_to_words.insert (pair <User, set <string>> {user, words});
	}
	return users_to_words;
}


static map <string, unsigned int> get_words_to_popularity (map <User, set <string>> a_users_to_words)
{
	map <string, unsigned int> words_to_popularity;

	for (auto user_to_words: a_users_to_words) {
		auto words = user_to_words.second;
		for (auto word: words) {
			if (words_to_popularity.find (word) == words_to_popularity.end ()) {
				words_to_popularity.insert (pair <string, unsigned int> {word, 1});
			} else {
				words_to_popularity.at (word) ++;
			}
		}
	}
	
	return words_to_popularity;
}


static void write_concrete_user_words
	(map <User, set <string>> users_to_toots,
	map <string, unsigned int> words_to_popularity,
	unsigned int minimum_popularity)
{
	stringstream out;
	unsigned int cn = 0;
	for (auto user_to_toots: users_to_toots) {
		if (cn % 100 == 0) {
			cerr << cn << " ";
		}
		cn ++;
		
		User user = user_to_toots.first;
		set <string> toots = user_to_toots.second;
		vector <string> toots_vector {toots.begin (), toots.end ()};
		const unsigned int vocabulary_size {1600};
		vector <string> model_6 = get_words_from_toots (toots_vector, 6, vocabulary_size, words_to_popularity, minimum_popularity);
		set <string> all;
		all.insert (model_6.begin (), model_6.end ());
		for (auto abstract_word: all) {
			out << "\"" << escape_csv (user.host) << "\",";
			out << "\"" << escape_csv (user.user) << "\",";
			out << "\"" << escape_csv (abstract_word) << "\"" << endl;
		}
	}
	cerr << endl;
	
	{
		string file_name {"/var/lib/vinayaka/model/concrete-user-words.csv"};
		FileLock lock {file_name};
		ofstream fout {file_name};
		fout << out.str ();
	}
}


static void write_popularity (map <string, unsigned int> words_to_popularity)
{
	string file_name {"/var/lib/vinayaka/model/popularity.csv"};
	FileLock lock {file_name};
	ofstream out {file_name};
	for (auto word_to_popularity: words_to_popularity) {
		string word = word_to_popularity.first;
		unsigned int speakers = word_to_popularity.second;
		out << "\"" << escape_csv (word) << "\",";
		out << "\"" << speakers << "\"" << endl;
	}
}


static map <string, unsigned int> compress_words_to_popularity
	(map <string, unsigned int> in, unsigned int minimum_popularity)
{
	map <string, unsigned int> out;
	for (auto word_to_popularity: in) {
		if (minimum_popularity < word_to_popularity.second) {
			out.insert (word_to_popularity);
		}
	}
	return out;
}


int main (int argc, char **argv)
{
	cerr << "get_union_of_history" << endl;
	map <User, set <string>> users_to_toots = get_union_of_history (5000);

	cerr << "save_users_to_toots" << endl;
	save_users_to_toots (users_to_toots);

	cerr << "get_full_words" << endl;
	map <User, set <string>> full_words = get_full_words (users_to_toots);

	cerr << "get_words_to_popularity" << endl;
	map <string, unsigned int> raw_words_to_popularity = get_words_to_popularity (full_words);

	unsigned int minimum_popularity {64};

	cerr << "compress_words_to_popularity" << endl;
	map <string, unsigned int> words_to_popularity = compress_words_to_popularity (raw_words_to_popularity, minimum_popularity);

	cerr << "write_concrete_user_words" << endl;
	write_concrete_user_words (users_to_toots, words_to_popularity, minimum_popularity);

	cerr << "write_popularity" << endl;
	write_popularity (words_to_popularity);
}


