#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
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


class ModelTopology {
public:
	unsigned int word_length;
	unsigned int vocabulary_size;
public:
	ModelTopology () { };
	ModelTopology (unsigned int a_word_length, unsigned int a_vocabulary_size) {
		word_length = a_word_length;
		vocabulary_size = a_vocabulary_size;
	};
public:
	bool operator < (const ModelTopology &right) const {
		return vocabulary_size < right.vocabulary_size
			|| (vocabulary_size == right.vocabulary_size && word_length < right.word_length);
	}
};


static map <User, set <string>> get_words_of_speakers (vector <ModelTopology> models)
{
	map <User, set <string>> users_to_words;
	for (auto model: models) {
		stringstream filename;
		filename << "/var/lib/vinayaka/user-words." << model.word_length << "." << model.vocabulary_size << ".csv";
		FILE *in = fopen (filename.str ().c_str (), "r");
		if (in == nullptr) {
			cerr << "File not found: " << filename.str () << endl;
		} else {
			try {
				vector <vector <string>> table = parse_csv (in);
				fclose (in);
				for (auto row: table) {
					if (2 < row.size ()) {
						User user {row.at (0), row.at (1)};
						set <string> words;
						for (unsigned int cn = 2; cn < row.size (); cn ++) {
							words.insert (row.at (cn));
						}
						if (users_to_words.find (user) == users_to_words.end ()) {
							users_to_words.insert (pair <User, set <string>> {user, words});
						} else {
							users_to_words.at (user).insert (words.begin (), words.end ());
						}
					}
				}
			} catch (ParseException e) {
				cerr << "ParseException " << e.line << " " << filename.str () << endl;
			}
		}
	}
	return users_to_words;
}


void save_model (map <User, set <string>> speaker_to_words, ofstream &out)
{
	for (auto speaker_and_words: speaker_to_words) {
		User user {speaker_and_words.first};
		set <string> words {speaker_and_words.second};
		out << "\"" << escape_csv (user.host) << "\",";
		out << "\"" << escape_csv (user.user) << "\",";
		for (auto word: words) {
			out << "\"" << escape_csv (word) << "\",";
		}
		out << endl;
	}
}


int main (int argc, char **argv)
{
	vector <ModelTopology> models = {
		ModelTopology {6, 400},
		ModelTopology {9, 400},
	};
	
	map <User, set <string>> speaker_to_words = get_words_of_speakers (models);
	ofstream out {"/var/lib/vinayaka/user-words.csv"};
	save_model (speaker_to_words, out);
}



