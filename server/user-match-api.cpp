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


static double get_similarity (vector <string> listener, vector <string> speaker, set <string> & a_intersection)
{
	set <string> listener_set {listener.begin (), listener.end ()};
	set <string> speaker_set {speaker.begin (), speaker.end ()};
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));
	double similarity = (static_cast <double> (intersection.size ()) * 2.0)
		/ (static_cast <double> (listener_set.size ()) + static_cast <double> (speaker_set.size ()));
	a_intersection = intersection;
	return similarity;
}


static map <User, double> get_users_and_similarity
	(vector <string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size,
	map <User, set <string>> & a_users_and_intersection)
{
	vector <string> words = get_words_from_toots (toots, word_length, vocabulary_size);
	stringstream filename;
	filename << "/var/lib/vinayaka/user-words." << word_length << "." << vocabulary_size << ".xml";
	vector <UserAndWords> users_and_words = read_storage (filename.str ());
	
	map <User, double> users_and_similarity;
	map <User, set <string>> users_and_intersection;
	
	for (auto user_and_words: users_and_words) {
		set <string> intersection;
		double similarity = get_similarity (words, user_and_words.words, intersection);
		User user;
		user.host = user_and_words.host;
		user.user = user_and_words.user;
		users_and_similarity.insert (pair <User, double> {user, similarity});
		users_and_intersection.insert (pair <User, set <string>> {user, intersection});
	}
	
	a_users_and_intersection = users_and_intersection;
	return users_and_similarity;
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
	
	map <User, double> map_6_20;
	map <User, set <string>> intersection_6_20;
	try {
		map_6_20 = get_users_and_similarity (toots, 6, 20, intersection_6_20);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_6_100;
	map <User, set <string>> intersection_6_100;
	try {
		map_6_100 = get_users_and_similarity (toots, 6, 100, intersection_6_100);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_9_40;
	map <User, set <string>> intersection_9_40;
	try {
		map_9_40 = get_users_and_similarity (toots, 9, 40, intersection_9_40);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	map <User, double> map_12_100;
	map <User, set <string>> intersection_12_100;
	try {
		map_12_100 = get_users_and_similarity (toots, 12, 100, intersection_12_100);
		available_models ++;
	} catch (ModelException e) {
		/* Do nothing. */
	}

	vector <UserAndSimilarity> users_and_similarity;
	for (auto user_in_map: map_12_100) {
		User user = user_in_map.first;
		double similarity_6_20 = (map_6_20.find (user) == map_6_20.end ()? 0: map_6_20.at (user));
		double similarity_6_100 = (map_6_100.find (user) == map_6_100.end ()? 0: map_6_100.at (user));
		double similarity_9_40 = (map_9_40.find (user) == map_9_40.end ()? 0: map_9_40.at (user));
		double similarity_12_100 = (map_12_100.find (user) == map_12_100.end ()? 0: map_12_100.at (user));

		double similarity = (similarity_6_20 + similarity_6_100 + similarity_9_40 + similarity_12_100)
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

		vector <string> intersection;
		if (intersection_6_20.find (User {user.host, user.user}) != intersection_6_20.end ()) {
			set <string> intersection_in_a_model = intersection_6_20.at (User {user.host, user.user});
			intersection.insert (intersection.end (), intersection_in_a_model.begin (), intersection_in_a_model.end ());
		}
		if (intersection_6_100.find (User {user.host, user.user}) != intersection_6_100.end ()) {
			set <string> intersection_in_a_model = intersection_6_100.at (User {user.host, user.user});
			intersection.insert (intersection.end (), intersection_in_a_model.begin (), intersection_in_a_model.end ());
		}
		if (intersection_9_40.find (User {user.host, user.user}) != intersection_9_40.end ()) {
			set <string> intersection_in_a_model = intersection_6_20.at (User {user.host, user.user});
			intersection.insert (intersection.end (), intersection_in_a_model.begin (), intersection_in_a_model.end ());
		}
		if (intersection_12_100.find (User {user.host, user.user}) != intersection_12_100.end ()) {
			set <string> intersection_in_a_model = intersection_12_100.at (User {user.host, user.user});
			intersection.insert (intersection.end (), intersection_in_a_model.begin (), intersection_in_a_model.end ());
		}

		cout
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"similarity\":" << user.similarity << ",";
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



