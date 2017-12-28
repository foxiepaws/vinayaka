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


double diameter_tolerance = 0.25;
static const unsigned int minimum_size_of_abstract_word = 2;


static vector <UserAndWords> users_and_words;
static map <string, vector <unsigned int>> words_to_speakers;
static map <string, string> concrete_to_abstract_words;


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
		cerr << "size: " << abstract_words.size () << endl;
		AbstractWord divided;
		bool found = false;
		for (auto &abstract_word: abstract_words) {
			if (diameter_tolerance < abstract_word.diameter) {
				found = true;
				divided = abstract_word;
				break;
			}
		}
		if (! found) {
			break;
		}
		cerr << "diameter: " << divided.diameter << endl;

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
	
	ofstream out {"/var/lib/vinayaka/abstract-words.csv"};

	for (auto abstract_word: abstract_words) {
		out << "\"" << escape_csv (escape_utf8_fragment (abstract_word.label)) << "\",";
		for (auto word: abstract_word.words) {
			out << "\"" << escape_csv (escape_utf8_fragment (word)) << "\",";
		}
		out << endl;
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
	ofstream out {"/var/lib/vinayaka/concrete-to-abstract-words.csv"};
	
	for (auto concrete_and_abstract_word: concrete_to_abstract_words) {
		out << "\"" << escape_csv (concrete_and_abstract_word.first) << "\",";
		out << "\"" << escape_csv (concrete_and_abstract_word.second) << "\",";
		out << endl;
	}
}


static void write_abstract_user_words ()
{
	ofstream out {"/var/lib/vinayaka/abstract-user-words.csv"};
	
	for (auto user_and_words: users_and_words) {
		out << "\"" << escape_csv (user_and_words.host) << "\",";
		out << "\"" << escape_csv (user_and_words.user) << "\",";
		set <string> abstract_words;
		for (auto concrete_word: user_and_words.words) {
			if (concrete_to_abstract_words.find (concrete_word) == concrete_to_abstract_words.end ()) {
				abstract_words.insert (concrete_word);
			} else {
				string abstract_word = concrete_to_abstract_words.at (concrete_word);
				abstract_words.insert (abstract_word);
			}
		}
		for (auto word: abstract_words) {
			out << "\"" << escape_csv (word) << "\",";
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

	for (auto word: words) {
		double distance_a = distance (word, extreme_a);
		double distance_b = distance (word, extreme_b);
		if (distance_a < distance_b) {
			a.words.push_back (word);
		} else {
			b.words.push_back (word);
		}
	}
}


