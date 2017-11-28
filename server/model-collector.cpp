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


static void write_to_storage (vector <pair <User, vector <string>>> users_and_words, ofstream &out)
{
	for (auto user_and_words: users_and_words) {
		auto user = user_and_words.first;
		auto words = user_and_words.second;
		out
			<< "\"" << escape_csv (user.host) << "\","
			<< "\"" << escape_csv (user.user) << "\",";

			for (auto word: words) {
				out << "\"" << escape_csv (word) << "\",";
			}
		out << endl;
	}
}


static const unsigned int history_variations = 24;


class WordAndCount {
public:
	string word;
	unsigned int count;
public:
	WordAndCount ();
	WordAndCount (string a_word, unsigned int a_count) {
		word = a_word;
		a_count = count;
	};
	bool operator < (const WordAndCount &r) const {
		return (r.count < count) || (r.count == count && r.word < word);
	};
};


static vector <string> get_top_words (vector <string> a_words, unsigned int a_vocabulary_size)
{
	map <string, unsigned int> occupancy;
	for (auto word: a_words) {
		if (occupancy.find (word) == occupancy.end ()) {
			occupancy.insert (pair <string, unsigned int> {word, 1});
		} else {
			occupancy.at (word) ++;
		}
	}
	vector <WordAndCount> words_and_counts;
	for (auto i: occupancy) {
		string word = i.first;
		unsigned int count = i.second;
		words_and_counts.push_back (WordAndCount (word, count));
	};
	sort (words_and_counts.begin (), words_and_counts.end ());
	vector <string> words;
	for (unsigned int cn = 0; cn < words_and_counts.size () && cn < a_vocabulary_size; cn ++) {
		words.push_back (words_and_counts.at (cn).word);
	}
	return words;
}

	
static void save_union_of_history (unsigned int word_length, unsigned int vocabulary_size)
{
	map <User, vector <string>> users_to_words;
	for (unsigned int cn = 0; cn < history_variations; cn ++) {
		stringstream filename;
		filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << "." << cn << ".json";
		try {
			vector <UserAndWords> users_and_words = read_storage (filename.str ());
			for (auto user_and_words: users_and_words) {
				User user {user_and_words.host, user_and_words.user};
				vector <string> words {user_and_words.words};
				if (users_to_words.find (user) == users_to_words.end ()) {
					users_to_words.insert (pair <User, vector <string>> {user, words});
				} else {
					users_to_words.at (user).insert (users_to_words.at (user).end (), words.begin (), words.end ());
				}
			}
		} catch (ModelException e) {
			cerr << "ModelException " << e.line << " " << endl;
		}
	}
	vector <pair <User, vector <string>>> users_and_words;
	for (auto user_and_words: users_to_words) {
		User user = user_and_words.first;
		vector <string> words {user_and_words.second};
		vector <string> top_words = get_top_words (words, vocabulary_size * 2);
		users_and_words.push_back (pair <User, vector <string>> {user, top_words});
	}
	stringstream filename;
	filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << ".csv";
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
		save_union_of_history (word_length, vocabulary_size);
	} else {
		cerr << "Too few arguments " << __LINE__ << endl;
		return 1;
	}
}


