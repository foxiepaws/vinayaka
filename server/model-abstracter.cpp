#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include "distsn.h"


using namespace std;


class AbstractWord {
public:
	string label;
	vector <string> words;
	unsigned int diameter;
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


static const unsigned int abstract_word_size = 400;
static const unsigned int minimum_size_of_abstract_word = 3;


static vector <UserAndWords> users_and_words;
static map <string, vector <unsigned int>> words_to_speakers;
static map <string, unsigned int> concrete_to_abstract_words;


static void get_users_and_words ()
{
	users_and_words.clear ();

	string filename {"/var/lib/vinayaka/user-words.csv"};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
		exit (1);
	}
	vector <vector <string>> table = parse_csv (in);
	fclose (in);

	for (auto row: table) {
		if (2 < row.size ()) {
			UserAndWords user_and_words;
			user_and_words.host = row.at (0);
			user_and_words.user = row.at (1);
			for (unsigned int cn = 2; cn < row.size (); cn ++) {
				user_and_words.words.push_back (row.at (cn));
			}
			users_and_words.push_back (user_and_words);
		}
	}
}


static void get_words_to_speakers ()
{
	words_to_speakers.clear ();

	for (unsigned int cn = 0; cn < users_and_words.size (); cn ++) {
		const vector <string> &words = users_and_words.at (cn).words;
		for (auto word: words) {
			if (words_to_speakers.find (word) == words_to_speakers.end ()) {
				vector <unsigned int> speakers;
				speakers.push_back (cn);
				words_to_speakers.insert (pair <string, vector <unsigned int>> {word, speakers});
			} else {
				words_to_speakers.at (word).push_back (cn);
			}
		}
	}
}


static unsigned int distance (const string &word_a, const string &word_b)
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
	return speakers_a.size () + speakers_b.size () - 2 * intersection.size ();
}


static void get_abstract_words ()
{
	AbstractWord root;
	for (auto word_and_speakers: words_to_speakers) {
		root.words.push_back (word_and_speakers.first);
	}
	root.update_diameter ();
	
	set <AbstractWord> abstract_words;
	abstract_words.insert (root);
	
	for (; ; ) {
		cerr << abstract_words.size () << endl;
		if (abstract_word_size <= abstract_words.size ()) {
			break;
		}
		AbstractWord divided;
		unsigned int max_diameter = 0;
		for (auto &abstract_word: abstract_words) {
			if (max_diameter < abstract_word.diameter) {
				max_diameter = abstract_word.diameter;
				divided = abstract_word;
			}
		}
		cerr << "max_diameter: " << max_diameter << endl;
		if (max_diameter == 0) {
			break;
		}
		abstract_words.erase (divided);
		AbstractWord a;
		AbstractWord b;
		divided.divide (a, b);
		
		if (minimum_size_of_abstract_word <= a.words.size ()) {
			a.update_diameter ();
			abstract_words.insert (a);
		}
		if (minimum_size_of_abstract_word <= b.words.size ()) {
			b.update_diameter ();
			abstract_words.insert (b);
		}
	}
	
	vector <AbstractWord> abstract_words_vector {abstract_words.begin (), abstract_words.end ()};

	ofstream out {"/var/lib/vinayaka/abstract-words.csv"};

	for (unsigned int cn = 0; cn < abstract_words_vector.size (); cn ++) {
		AbstractWord abstract_word = abstract_words_vector.at (cn);
		out << "\"" << cn << "\",";
		out << "\"" << escape_csv (escape_utf8_fragment (abstract_word.label)) << "\",";
		for (auto word: abstract_word.words) {
			out << "\"" << escape_csv (escape_utf8_fragment (word)) << "\",";
		}
		out << endl;
	}
	
	for (unsigned int cn = 0; cn < abstract_words_vector.size (); cn ++) {
		AbstractWord abstract_word = abstract_words_vector.at (cn);
		for (string concrete_word: abstract_word.words) {
			if (concrete_to_abstract_words.find (concrete_word) == concrete_to_abstract_words.end ()) {
				concrete_to_abstract_words.insert (pair <string, unsigned int> {concrete_word, cn});
			}
		}
	}
}


static void write_concrete_to_abstract_words ()
{
	ofstream out {"/var/lib/vinayaka/concrete-to-abstract-words.csv"};
	
	for (auto concrete_and_abstract_word: concrete_to_abstract_words) {
		out << "\"" << escape_csv (concrete_and_abstract_word.first) << "\",";
		out << "\"" << concrete_and_abstract_word.second << "\",";
		out << endl;
	}
}


static void write_abstract_user_words ()
{
	ofstream out {"/var/lib/vinayaka/abstract-user-words.csv"};
	
	for (auto user_and_words: users_and_words) {
		out << "\"" << escape_csv (user_and_words.host) << "\",";
		out << "\"" << escape_csv (user_and_words.user) << "\",";
		for (auto concrete_word: user_and_words.words) {
			if (concrete_to_abstract_words.find (concrete_word) != concrete_to_abstract_words.end ()) {
				unsigned int abstract_word = concrete_to_abstract_words.at (concrete_word);
				out << "\"" << abstract_word << "\",";
			}
		}
		out << endl;
	}
}


int main (int argc, char **argv)
{
	get_users_and_words ();
	get_words_to_speakers ();
	get_abstract_words ();
	write_concrete_to_abstract_words ();
	write_abstract_user_words ();
}


void AbstractWord::update_diameter ()
{
	string a;
	string b = words.at (0);
	unsigned int iteration = 4 + max (0.0, log (words.size ()) / log (2));

	for (unsigned int cn = 0; cn < iteration; cn ++) {
		string max_distance_word = b;
		unsigned int max_distance = 0;
		
		for (auto word: words) {
			unsigned int this_distance = distance (b, word);
			if (max_distance < this_distance) {
				max_distance = this_distance;
				max_distance_word = word;
			}
		}
		
		a = b;
		b = max_distance_word;
	}
	
	diameter = distance (a, b);
	extreme_a = a;
	extreme_b = b;
}


void AbstractWord::divide (AbstractWord &a, AbstractWord &b) const
{
	a.label = extreme_a;
	a.words.clear ();
	b.label = extreme_b;
	b.words.clear ();

	for (auto word: words) {
		unsigned int distance_a = distance (word, extreme_a);
		unsigned int distance_b = distance (word, extreme_b);
		if (distance_a < distance_b) {
			a.words.push_back (word);
		} else {
			b.words.push_back (word);
		}
	}
}


