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


class UserAndSimilarity {
public:
	string user;
	string host;
	double similarity;
};


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


static bool by_similarity_desc (const UserAndSimilarity &a, const UserAndSimilarity &b)
{
	return b.similarity < a.similarity;
}


static double get_similarity (set <string> listener_set, set <string> speaker_set, set <string> & a_intersection)
{
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));
	double similarity = (static_cast <double> (intersection.size ()) * 2.0)
		/ (static_cast <double> (listener_set.size ()) + static_cast <double> (speaker_set.size ()));
	a_intersection = intersection;
	return similarity;
}


static string escape_utf8_fragment (string in) {
	string out;
	for (unsigned int cn = 0; cn < in.size (); cn ++) {
		unsigned int c = static_cast <unsigned char> (in.at (cn));
		if ((0x00 <= c && c < 0x20) || c == ' ') {
			out += "�";
		} else if ((0xc2 <= c && c <= 0xdf && in.size () - cn < 2)
			|| (0xe0 <= c && c <= 0xef && in.size () - cn < 3)
			|| (0xf0 <= c && c <= 0xf7 && in.size () - cn < 4)
			|| (0xf8 <= c && c <= 0xfb && in.size () - cn < 5)
			|| (0xfc <= c && c <= 0xfd && in.size () - cn < 6))
		{
			for (unsigned int x = 0; x < in.size () - cn; x ++) {
				out += "�";
			}
			break;
		} else {
			out.push_back (c);
		}
	}
	return out;
}


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


static set <string> get_words_of_listener (vector <string> toots, vector <ModelTopology> models)
{
	set <string> words;
	for (auto model: models) {
		vector <string> words_in_a_model = get_words_from_toots (toots, model.word_length, model.vocabulary_size);
		words.insert (words_in_a_model.begin (), words_in_a_model.end ());
	}
	return words;
};


static map <User, set <string>> get_words_of_speakers (vector <ModelTopology> models)
{
	map <User, set <string>> users_to_words;
	for (auto model: models) {
		stringstream filename;
		filename << "/var/lib/vinayaka/user-words." << model.word_length << "." << model.vocabulary_size << ".json";
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
			cerr << "ModelException " << e.line << " " << filename.str () << endl;
		}
	}
	return users_to_words;
}


int main (int argc, char **argv)
{
	if (argc < 3) {
		exit (1);
	}
	string host {argv [1]};
	string user {argv [2]};

	cerr << user << "@" << host << endl;
	string screen_name;
	string bio;
	vector <string> toots;
	get_profile (host, user, screen_name, bio, toots);
	toots.push_back (screen_name);
	toots.push_back (screen_name);
	toots.push_back (bio);
	toots.push_back (bio);

	vector <ModelTopology> models = {
		ModelTopology {3, 400},
		ModelTopology {6, 400},
		ModelTopology {9, 400},
		ModelTopology {12, 400},
		ModelTopology {15, 400},
		ModelTopology {18, 400}
	};
	
	set <string> words_of_listener = get_words_of_listener (toots, models);
	map <User, set <string>> speaker_to_words = get_words_of_speakers (models);

	vector <UserAndSimilarity> speakers_and_similarity;
	map <User, set <string>> speaker_to_intersection;
	
	for (auto speaker_and_words: speaker_to_words) {
		User speaker = speaker_and_words.first;
		set <string> words_of_speaker = speaker_and_words.second;
		set <string> intersection;
		double similarity = get_similarity (words_of_listener, words_of_speaker, intersection);
		UserAndSimilarity speaker_and_similarity;
		speaker_and_similarity.user = speaker.user;
		speaker_and_similarity.host = speaker.host;
		speaker_and_similarity.similarity = similarity;
		speakers_and_similarity.push_back (speaker_and_similarity);
		speaker_to_intersection.insert (pair <User, set <string>> {speaker, intersection});
	}

	stable_sort (speakers_and_similarity.begin (), speakers_and_similarity.end (), by_similarity_desc);
	
	cout << "Content-Type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < speakers_and_similarity.size () && cn < 1000; cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto speaker = speakers_and_similarity.at (cn);
		set <string> intersection_set = speaker_to_intersection.at (User {speaker.host, speaker.user});
		vector <string> intersection {intersection_set.begin (), intersection_set.end ()};
		cout
			<< "{"
			<< "\"host\":\"" << escape_json (speaker.host) << "\","
			<< "\"user\":\"" << escape_json (speaker.user) << "\","
			<< "\"similarity\":" << speaker.similarity << ",";
		cout << "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				cout << ",";
			}
			cout << "\"" << escape_json (escape_utf8_fragment (intersection [cn_intersection])) << "\"";
		}
		cout << "]";
		cout << "}";
	}
	cout << "]";
}



