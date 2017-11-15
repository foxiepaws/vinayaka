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


class UserAndWords {
public:
	string user;
	string host;
	vector <string> words;
};


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


class ModelException: public ExceptionWithLineNumber {
public:
	ModelException () { };
	ModelException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
};


static bool by_similarity_desc (const UserAndSimilarity &a, const UserAndSimilarity &b)
{
	return b.similarity < a.similarity;
}


static vector <UserAndWords> read_storage (string filename)
{
	FILE * in = fopen (filename.c_str (), "rb");
	if (in == nullptr) {
		throw (ModelException {__LINE__});
	}
        string s;
        for (; ; ) {
                if (feof (in)) {
                        break;
                }
                char b [1024];
                fgets (b, 1024, in);
                s += string {b};
        }
        picojson::value json_value;
        picojson::parse (json_value, s);
        auto array = json_value.get <picojson::array> ();
        
        vector <UserAndWords> memo;
        
        for (auto user_value: array) {
        	auto user_object = user_value.get <picojson::object> ();
        	string user = user_object.at (string {"user"}).get <string> ();
        	string host = user_object.at (string {"host"}).get <string> ();
        	auto words_array = user_object.at (string {"words"}).get <picojson::array> ();
        	vector <string> words;
        	for (auto word_value: words_array) {
        		string word = word_value.get <string> ();
        		words.push_back (word);
        	}
        	UserAndWords user_and_words;
        	user_and_words.user = user;
        	user_and_words.host = host;
        	user_and_words.words = words;
        	memo.push_back (user_and_words);
        }
	return memo;
}


static double get_similarity (vector <string> listener, vector <string> speaker)
{
	set <string> listener_set {listener.begin (), listener.end ()};
	set <string> speaker_set {speaker.begin (), speaker.end ()};
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));
	double similarity = static_cast <double> (intersection.size ()) / static_cast <double> (speaker_set.size ());
	return similarity;
}


static map <User, double> get_users_and_similarity (vector <string> toots, unsigned int word_length, unsigned int vocabulary_size)
{
	vector <string> words = get_words_from_toots (toots, word_length, vocabulary_size);
	stringstream filename;
	filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << ".xml";
	vector <UserAndWords> users_and_words = read_storage (filename.str ());
	
	map <User, double> users_and_similarity;
	
	for (auto user_and_words: users_and_words) {
		double similarity = get_similarity (words, user_and_words.words);
		User user;
		user.host = user_and_words.host;
		user.user = user_and_words.user;
		users_and_similarity.insert (pair <User, double> {user, similarity});
	}
	
	return users_and_similarity;
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
	toots.push_back (bio);
	
	unsigned int available_models = 0;
	
	map <User, double> map_6_100;
	try {
		map_6_100 = get_users_and_similarity (toots, 6, 100);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_6_1000;
	try {
		map_6_1000 = get_users_and_similarity (toots, 6, 1000);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_12_100;
	try {
		map_12_100 = get_users_and_similarity (toots, 12, 100);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_12_1000;
	try {
		map_12_1000 = get_users_and_similarity (toots, 12, 1000);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	vector <UserAndSimilarity> users_and_similarity;
	for (auto user_in_map: map_6_1000) {
		User user = user_in_map.first;
		double similarity_6_100 = (map_6_100.find (user) == map_6_100.end ()? 0: map_6_100.at (user));
		double similarity_6_1000 = (map_6_1000.find (user) == map_6_1000.end ()? 0: map_6_1000.at (user));
		double similarity_12_100 = (map_12_100.find (user) == map_12_100.end ()? 0: map_12_100.at (user));
		double similarity_12_1000 = (map_12_1000.find (user) == map_12_1000.end ()? 0: map_12_1000.at (user));

		double similarity = (similarity_6_100 + similarity_6_1000 + similarity_12_100 + similarity_12_1000)
			/ static_cast <double> (available_models);
		UserAndSimilarity user_and_similarity;
		user_and_similarity.user = user.user;
		user_and_similarity.host = user.host;
		user_and_similarity.similarity = similarity;
		users_and_similarity.push_back (user_and_similarity);
	}
	
	stable_sort (users_and_similarity.begin (), users_and_similarity.end (), by_similarity_desc);
	
	cout << "Content-Type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < users_and_similarity.size () && cn < 100; cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto user = users_and_similarity.at (cn);
		cout
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"similarity\":" << user.similarity
			<< "}";
	}
	cout << "]";
}



