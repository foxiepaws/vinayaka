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
static const double diameter_tolerance = 0.5;
static const unsigned int minimum_size_of_abstract_word = 2;
static const unsigned int minimum_speakers_of_word = 3;


class AbstractWord {
public:
	string label;
	vector <string> words;
	double diameter;
	string extreme_a;
	string extreme_b;
public:
	void update_diameter ();
	void divide (AbstractWord &a, AbstractWord &b) const;
public:
	bool operator < (const AbstractWord &r) const {
		return label < r.label;
	};
};


static vector <UserAndWords> users_and_words;
static map <string, vector <unsigned int>> words_to_speakers;
static map <string, string> concrete_to_abstract_words;

	
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
	vector <string> toots_vector {toots.begin (), toots.end ()};
	vector <string> model_6 = get_words_from_toots (toots_vector, 6, numeric_limits <unsigned int>::max ());
	vector <string> model_7 = get_words_from_toots (toots_vector, 7, numeric_limits <unsigned int>::max ());
	vector <string> model_8 = get_words_from_toots (toots_vector, 8, numeric_limits <unsigned int>::max ());
	vector <string> model_9 = get_words_from_toots (toots_vector, 9, numeric_limits <unsigned int>::max ());
	vector <string> model_12 = get_words_from_toots (toots_vector, 12, numeric_limits <unsigned int>::max ());
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


static void get_users_and_words (map <User, set <string>> a_users_to_words)
{
	users_and_words.clear ();

	for (auto user_to_words: a_users_to_words) {
		User user = user_to_words.first;
		set <string> words = user_to_words.second;
		UserAndWords user_and_words;
		user_and_words.host = user.host;
		user_and_words.user = user.user;
		for (auto word: words) {
			user_and_words.words.push_back (word);
		}
		users_and_words.push_back (user_and_words);
	}
}


static void get_words_to_speakers ()
{
	map <string, vector <unsigned int>> words_to_speakers_raw;

	for (unsigned int cn = 0; cn < users_and_words.size (); cn ++) {
		const vector <string> &words = users_and_words.at (cn).words;
		for (auto word: words) {
			if (words_to_speakers_raw.find (word) == words_to_speakers_raw.end ()) {
				vector <unsigned int> speakers;
				speakers.push_back (cn);
				words_to_speakers_raw.insert (pair <string, vector <unsigned int>> {word, speakers});
			} else {
				words_to_speakers_raw.at (word).push_back (cn);
			}
		}
	}

	words_to_speakers.clear ();
	for (auto word_to_speakers: words_to_speakers_raw) {
		if (minimum_speakers_of_word <= word_to_speakers.second.size ()) {
			words_to_speakers.insert (word_to_speakers);
		}
	}

}


static double distance (const string &word_a, const string &word_b)
{
	const vector <unsigned int> speakers_a = words_to_speakers.at (word_a);
	const vector <unsigned int> speakers_b = words_to_speakers.at (word_b);
	vector <unsigned int> intersection;
	set_intersection
		(speakers_a.begin (),
		speakers_a.end (),
		speakers_b.begin (),
		speakers_b.end (),
		inserter (intersection, intersection.end ()));
	unsigned int numerator = speakers_a.size () + speakers_b.size () - 2 * intersection.size ();
	unsigned int denominator = speakers_a.size () + speakers_b.size () - intersection.size ();
	return static_cast <double> (numerator) / static_cast <double> (denominator);
}


static set <AbstractWord> get_abstract_words_impl (AbstractWord abstract_word)
{
	if (1000 < abstract_word.words.size ()) {
		cerr << "abstract_word.words.size: " << abstract_word.words.size () << endl;
		cerr << "abstract_word.diameter: " << abstract_word.diameter << endl;
	}

	set <AbstractWord> abstract_words;

	if (abstract_word.diameter <= diameter_tolerance) {
		abstract_words.insert (abstract_word);
	} else {
		AbstractWord a;
		AbstractWord b;
		abstract_word.divide (a, b);
		if (minimum_size_of_abstract_word <= a.words.size ()) {
			a.update_diameter ();
			set <AbstractWord> abstract_words_a = get_abstract_words_impl (a);
			abstract_words.insert (abstract_words_a.begin (), abstract_words_a.end ());
		}
		if (minimum_size_of_abstract_word <= b.words.size ()) {
			b.update_diameter ();
			set <AbstractWord> abstract_words_b = get_abstract_words_impl (b);
			abstract_words.insert (abstract_words_b.begin (), abstract_words_b.end ());
		}
	}
	return abstract_words;
}


static void get_abstract_words ()
{
	AbstractWord root;
	for (auto word_and_speakers: words_to_speakers) {
		root.words.push_back (word_and_speakers.first);
	}
	root.update_diameter ();
	
	set <AbstractWord> abstract_words = get_abstract_words_impl (root);
	
	ofstream out {"/var/lib/vinayaka/model/abstract-words.csv"};

	for (auto abstract_word: abstract_words) {
		for (auto word: abstract_word.words) {
			out << "\"" << escape_csv (escape_utf8_fragment (abstract_word.label)) << "\",";
			out << "\"" << escape_csv (escape_utf8_fragment (word)) << "\"" << endl;
		}
	}
	
	for (auto abstract_word: abstract_words) {
		for (string concrete_word: abstract_word.words) {
			if (concrete_to_abstract_words.find (concrete_word) == concrete_to_abstract_words.end ()) {
				concrete_to_abstract_words.insert (pair <string, string> {concrete_word, abstract_word.label});
			}
		}
	}
}


static void write_concrete_to_abstract_words ()
{
	ofstream out {"/var/lib/vinayaka/model/concrete-to-abstract-words.csv"};
	
	for (auto concrete_and_abstract_word: concrete_to_abstract_words) {
		out << "\"" << escape_csv (concrete_and_abstract_word.first) << "\",";
		out << "\"" << escape_csv (concrete_and_abstract_word.second) << "\"" << endl;
	}
}


static void write_abstract_user_words (map <User, set <string>> users_to_toots)
{
	ofstream out {"/var/lib/vinayaka/model/abstract-user-words.csv"};
	for (auto user_to_toots: users_to_toots) {
		User user = user_to_toots.first;
		set <string> toots = user_to_toots.second;
		vector <string> toots_vector {toots.begin (), toots.end ()};
		vector <string> model_6 = get_words_from_toots (toots_vector, 6, 800, concrete_to_abstract_words);
		vector <string> model_7 = get_words_from_toots (toots_vector, 7, 800, concrete_to_abstract_words);
		vector <string> model_8 = get_words_from_toots (toots_vector, 8, 800, concrete_to_abstract_words);
		vector <string> model_9 = get_words_from_toots (toots_vector, 9, 800, concrete_to_abstract_words);
		vector <string> model_12 = get_words_from_toots (toots_vector, 12, 800, concrete_to_abstract_words);
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


int main (int argc, char **argv)
{
	map <User, set <string>> users_to_toots = get_union_of_history (5000);
	save_users_to_toots (users_to_toots);
	map <User, set <string>> full_words = get_full_words (users_to_toots);
	get_users_and_words (full_words);
	get_words_to_speakers ();
	get_abstract_words ();
	write_concrete_to_abstract_words ();
	write_abstract_user_words (users_to_toots);
}


void AbstractWord::update_diameter ()
{
	if (words.size () == 1) {
		diameter = 0;
		extreme_a = words.at (0);
		extreme_b = words.at (0);
	} else if (words.size () == 2) {
		diameter = distance (words.at (0), words.at (1));
		extreme_a = words.at (0);
		extreme_b = words.at (1);
	} else {
		string a;
		string b = words.at (0);
		unsigned int iteration = 4 + max (0.0, log (words.size ()) / log (2));

		for (unsigned int cn = 0; cn < iteration; cn ++) {
			string max_distance_word = b;
			double max_distance = 0;
		
			for (auto word: words) {
				double this_distance = distance (b, word);
				if (max_distance < this_distance) {
					max_distance = this_distance;
					max_distance_word = word;
				}
				if (1.0 <= max_distance) {
					break;
				}
			}
					
			a = b;
			b = max_distance_word;
			
			if (1.0 <= max_distance) {
				break;
			}
		}
	
		diameter = distance (a, b);
		extreme_a = a;
		extreme_b = b;
	}
}


void AbstractWord::divide (AbstractWord &a, AbstractWord &b) const
{
	a.label = extreme_a;
	a.words.clear ();
	b.label = extreme_b;
	b.words.clear ();

	unsigned int toggle = 0;

	for (auto word: words) {
		double distance_a = distance (word, extreme_a);
		double distance_b = distance (word, extreme_b);
		if (distance_a < distance_b) {
			a.words.push_back (word);
		} else if (distance_b < distance_a){
			b.words.push_back (word);
		} else {
			if (toggle == 0) {
				a.words.push_back (word);
			} else {
				b.words.push_back (word);
			}
			toggle = (toggle + 1) % 2;
		}
	}
}



