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


class UserAndAbstractWords {
public:
	string host;
	string user;
	vector <string> words;
	vector <string> abstract_words;
};


class AbstractWord {
public:
	string label;
	vector <string> words;
	unsigned int diameter;
	string extreme_a;
	string extreme_b;
public:
	void update_diameter ();
};


static vector <UserAndAbstractWords> users_and_words;
static map <string, vector <unsigned int>> words_to_speakers;


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
			UserAndAbstractWords user_and_words;
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


int main (int argc, char **argv)
{
	get_users_and_words ();
	get_words_to_speakers ();
	
	AbstractWord root;
	for (auto word_and_speakers: words_to_speakers) {
		root.words.push_back (word_and_speakers.first);
	}
	root.update_diameter ();
	cout << root.diameter << endl;
	cout << root.extreme_a << endl;
	cout << root.extreme_b << endl;
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


