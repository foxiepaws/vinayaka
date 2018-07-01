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
	const unsigned int model_size {1600};
	vector <string> toots_vector {toots.begin (), toots.end ()};
	vector <string> model_6 = get_words_from_toots (toots_vector, 6, model_size);
	vector <string> model_7 = get_words_from_toots (toots_vector, 7, model_size);
	vector <string> model_8 = get_words_from_toots (toots_vector, 8, model_size);
	vector <string> model_9 = get_words_from_toots (toots_vector, 9, model_size);
	vector <string> model_12 = get_words_from_toots (toots_vector, 12, model_size);
	set <string> all;
	all.insert (model_6.begin (), model_6.end ());
	all.insert (model_7.begin (), model_7.end ());
	all.insert (model_8.begin (), model_8.end ());
	all.insert (model_9.begin (), model_9.end ());
	all.insert (model_12.begin (), model_12.end ());
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


static map <string, unsigned int> get_words_to_speakers (map <User, set <string>> a_users_to_words)
{
	map <string, unsigned int> words_to_speakers;

	for (auto user_to_words: a_users_to_words) {
		auto words = user_to_words.second;
		for (auto word: words) {
			if (words_to_speakers.find (word) == words_to_speakers.end ()) {
				words_to_speakers.insert (pair <string, unsigned int> {word, 1});
			} else {
				words_to_speakers.at (word) ++;
			}
		}
	}
	
	return words_to_speakers;
}


static void write_concrete_user_words (map <User, set <string>> users_to_toots, map <string, unsigned int> words_to_speakers)
{
	ofstream out {"/var/lib/vinayaka/model/concrete-user-words.csv"};
	for (auto user_to_toots: users_to_toots) {
		User user = user_to_toots.first;
		set <string> toots = user_to_toots.second;
		vector <string> toots_vector {toots.begin (), toots.end ()};
		const unsigned int vocabulary_size {800};
		vector <string> model_6 = get_words_from_toots (toots_vector, 6, vocabulary_size, words_to_speakers);
		vector <string> model_7 = get_words_from_toots (toots_vector, 7, vocabulary_size, words_to_speakers);
		vector <string> model_8 = get_words_from_toots (toots_vector, 8, vocabulary_size, words_to_speakers);
		vector <string> model_9 = get_words_from_toots (toots_vector, 9, vocabulary_size, words_to_speakers);
		vector <string> model_12 = get_words_from_toots (toots_vector, 12, vocabulary_size, words_to_speakers);
		set <string> all;
		all.insert (model_6.begin (), model_6.end ());
		all.insert (model_7.begin (), model_7.end ());
		all.insert (model_8.begin (), model_8.end ());
		all.insert (model_9.begin (), model_9.end ());
		all.insert (model_12.begin (), model_12.end ());
		for (auto abstract_word: all) {
			out << "\"" << escape_csv (user.host) << "\",";
			out << "\"" << escape_csv (user.user) << "\",";
			out << "\"" << escape_csv (abstract_word) << "\"" << endl;
		}
	}
}


static void write_occupancy (map <string, unsigned int> words_to_speakers)
{
	ofstream out {"/var/lib/vinayaka/model/occupancy.csv"};
	for (auto word_to_speakers: words_to_speakers) {
		string word = word_to_speakers.first;
		unsigned int speakers = word_to_speakers.second;
		out << "\"" << escape_csv (word) << "\",";
		out << "\"" << speakers << "\"" << endl;
	}
}



int main (int argc, char **argv)
{
	cerr << "get_union_of_history" << endl;
	map <User, set <string>> users_to_toots = get_union_of_history (5000);

	cerr << "save_users_to_toots" << endl;
	save_users_to_toots (users_to_toots);

	cerr << "get_full_words" << endl;
	map <User, set <string>> full_words = get_full_words (users_to_toots);

	cerr << "get_words_to_speakers" << endl;
	map <string, unsigned int> words_to_speakers = get_words_to_speakers (full_words);

	cerr << "write_concrete_user_words" << endl;
	write_concrete_user_words (users_to_toots, words_to_speakers);

	cerr << "write_occupancy" << endl;
	write_occupancy (words_to_speakers);
}


