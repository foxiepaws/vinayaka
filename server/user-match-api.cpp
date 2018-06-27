#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <ctime>
#include <random>
#include "picojson.h"
#include "tinyxml2.h"
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


class UserAndSimilarity {
public:
	string host;
	string user;
	double similarity;
public:
	UserAndSimilarity () { };
	UserAndSimilarity (string a_host, string a_user, double a_similarity):
		host (a_host), user (a_user), similarity (a_similarity) { };
};


static bool by_similarity_desc (const UserAndSimilarity &a, const UserAndSimilarity &b)
{
	return b.similarity < a.similarity;
}


static double get_rarity (unsigned int occupancy)
{
	return 4.0 * pow (static_cast <double> (occupancy), - 0.25);
}


static double get_similarity
	(set <string> listener_set,
	set <string> speaker_set,
	map <string, double> & a_intersection,
	map <string, unsigned int> words_to_occupancy)
{
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));

	double similarity = 0;
	a_intersection.clear ();

	for (string word: intersection) {
		unsigned int occupancy = 4;
		if (words_to_occupancy.find (word) != words_to_occupancy.end ()) {
			occupancy = words_to_occupancy.at (word);
		}
		double rarity = get_rarity (occupancy);
		similarity += rarity;
		a_intersection.insert (pair <string, double> {word, rarity});
	}

	return similarity;
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


static set <string> get_words_of_listener
	(vector <string> toots,
	vector <ModelTopology> models)
{
	set <string> words;
	for (auto model: models) {
		vector <string> words_in_a_model
			= get_words_from_toots (toots, model.word_length, model.vocabulary_size);
		words.insert (words_in_a_model.begin (), words_in_a_model.end ());
	}
	return words;
};


static map <User, set <string>> get_words_of_speakers (string filename)
{
	map <User, set <string>> users_to_words;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
	} else {
		try {
			vector <vector <string>> table = parse_csv (in);
			fclose (in);
			set <User> users_set;
			for (auto row: table) {
				if (2 < row.size ()) {
					users_set.insert (User {row.at (0), row.at (1)});
				}
			}
			vector <User> users_vector {users_set.begin (), users_set.end ()};
			for (unsigned int cn = 0; cn < users_vector.size (); cn ++) {
				users_to_words.insert (pair <User, set <string>> {users_vector.at (cn), set <string> {}});
			}
			for (auto row: table) {
				if (2 < row.size ()) {
					User user {row.at (0), row.at (1)};
					string word {row.at (2)};
					if (users_to_words.find (user) != users_to_words.end ()) {
						users_to_words.at (user).insert (word);
					}
				}
			}
		} catch (ParseException e) {
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return users_to_words;
}


static map <string, unsigned int> get_words_to_occupancy (string filename)
{
	map <string, unsigned int> words_to_occupancy;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
	} else {
		try {
			vector <vector <string>> table = parse_csv (in);
			fclose (in);
			for (auto row: table) {
				if (1 < row.size ()) {
					string word = row.at (0);
					stringstream occupancy_stream {row.at (1)};
					unsigned int occupancy;
					occupancy_stream >> occupancy;
					if (4 < occupancy
						&& words_to_occupancy.find (word) == words_to_occupancy.end ())
					{
						words_to_occupancy.insert (pair <string, unsigned int> {word, occupancy});
					}
				}
			}
		} catch (ParseException e) {
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return words_to_occupancy;
}


static string format_result
	(vector <UserAndSimilarity> speakers_and_similarity,
	map <User, map <string, double>> speaker_to_intersection,
	map <User, Profile> users_to_profile,
	set <User> blacklisted_users)
{
	stringstream out;
	out << "[";
	for (unsigned int cn = 0; cn < speakers_and_similarity.size () && cn < 400; cn ++) {
		if (0 < cn) {
			out << ",";
		}
		auto speaker = speakers_and_similarity.at (cn);

		map <string, double> intersection_map;
		if (speaker_to_intersection.find (User {speaker.host, speaker.user}) != speaker_to_intersection.end ()) {
			intersection_map = speaker_to_intersection.at (User {speaker.host, speaker.user});
		}
		vector <string> intersection_vector;
		for (auto word_to_score: intersection_map) {
			intersection_vector.push_back (word_to_score.first);
		}

		out
			<< "{"
			<< "\"host\":\"" << escape_json (speaker.host) << "\","
			<< "\"user\":\"" << escape_json (speaker.user) << "\","
			<< "\"similarity\":" << speaker.similarity << ",";
		bool blacklisted
			= (blacklisted_users.find (User {speaker.host, speaker.user}) != blacklisted_users.end ())
			|| (blacklisted_users.find (User {speaker.host, string {"*"}}) != blacklisted_users.end ());
		out << "\"blacklisted\":" << (blacklisted? "true": "false") << ",";

		if (users_to_profile.find (User {speaker.host, speaker.user}) == users_to_profile.end ()) {
			out
				<< "\"screen_name\":\"\","
				<< "\"bio\":\"\","
				<< "\"avatar\":\"\","
				<< "\"type\":\"\",";
		} else {
			Profile profile = users_to_profile.at (User {speaker.host, speaker.user});
			out
				<< "\"screen_name\":\"" << escape_json (profile.screen_name) << "\","
				<< "\"bio\":\"" << escape_json (profile.bio) << "\",";
			if (safe_url (profile.avatar)) {
				out << "\"avatar\":\"" << escape_json (profile.avatar) << "\",";
			} else {
				out << "\"avatar\":\"\",";
			}
			out << "\"type\":\"" << escape_json (profile.type) << "\",";
		}

		out << "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection_vector.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				out << ",";
			}
			string word = intersection_vector.at (cn_intersection);
			double score = intersection_map.at (word);
			out << "{";
			out << "\"\":\"" << escape_json (escape_utf8_fragment (word)) << "\",";
			out << "\"\":" << scientific << score << "\"";
			out << "}";
		}
		out << "]";

		out << "}";
	}
	out << "]";
	return out.str ();
}


static void add_to_cache (string host, string user, string result)
{
	time_t now = time (nullptr);
	ofstream out {"/var/lib/vinayaka/match-cache.csv", std::ofstream::out | std::ofstream::app};
	out << "\"" << escape_csv (host) << "\",";
	out << "\"" << escape_csv (user) << "\",";
	out << "\"" << escape_csv (result) << "\",";
	out << "\"" << now << "\"" << endl;
}


int main (int argc, char **argv)
{
	if (argc < 3) {
		exit (1);
	}
	string host {argv [1]};
	string user {argv [2]};

	cerr << user << "@" << host << endl;

	cerr << "get_optouted_users" << endl;
	set <User> optouted_users = get_optouted_users ();
	if (optouted_users.find (User {host, user}) != optouted_users.end ()) {
		cerr << "optouted." << endl;

		vector <UserAndSimilarity> dummy_speakers_and_similarity;
		for (unsigned int cn = 0; cn < 400; cn ++) {
			dummy_speakers_and_similarity.push_back (UserAndSimilarity {"example.com", "example", 0.0});
		}
		map <User, map <string, double>> dummy_speaker_to_intersection;
		map <User, Profile> dummy_users_to_profile;
		set <User> dummy_blacklisted_users;

		string result = format_result
			(dummy_speakers_and_similarity,
			dummy_speaker_to_intersection,
			dummy_users_to_profile,
			dummy_blacklisted_users);
	
		add_to_cache (host, user, result);
		return 0;
	}
	
	string screen_name;
	string bio;
	vector <string> toots;
	cerr << "get_profile" << endl;
	get_profile (true /* pagenation */, host, user, screen_name, bio, toots);
	toots.push_back (screen_name);
	toots.push_back (bio);

	vector <ModelTopology> models = {
		ModelTopology {6, 800},
		ModelTopology {7, 800},
		ModelTopology {8, 800},
		ModelTopology {9, 800},
		ModelTopology {12, 800},
	};
	
	cerr << "get_words_of_listener" << endl;
	set <string> words_of_listener = get_words_of_listener (toots, models);
	
	cerr << "get_words_of_speakers" << endl;
	map <User, set <string>> speaker_to_words
		= get_words_of_speakers (string {"/var/lib/vinayaka/model/concrete-user-words.csv"});
	
	cerr << "get_words_to_occupancy" << endl;
	map <string, unsigned int> words_to_occupancy
		= get_words_to_occupancy (string {"/var/lib/vinayaka/model/occupancy.csv"});
		
	vector <UserAndSimilarity> speakers_and_similarity;
	map <User, map <string, double>> speaker_to_intersection;
	
	cerr << "get_similarity" << endl;
	unsigned int cn = 0;
	for (auto speaker_and_words: speaker_to_words) {
		cerr << cn << " ";
		cn ++;

		User speaker = speaker_and_words.first;
		set <string> words_of_speaker = speaker_and_words.second;
		map <string, double> intersection;
		double similarity = get_similarity (words_of_listener, words_of_speaker, intersection, words_to_occupancy);
		UserAndSimilarity speaker_and_similarity;
		speaker_and_similarity.user = speaker.user;
		speaker_and_similarity.host = speaker.host;
		speaker_and_similarity.similarity = similarity;
		speakers_and_similarity.push_back (speaker_and_similarity);
		speaker_to_intersection.insert (pair <User, map <string, double>> {speaker, intersection});
	}
	cerr << endl;

	cerr << "stable_sort" << endl;
	stable_sort (speakers_and_similarity.begin (), speakers_and_similarity.end (), by_similarity_desc);

	cerr << "read_profiles" << endl;
	map <User, Profile> users_to_profile = read_profiles ();
	cerr << "get_blacklisted_users" << endl;
	set <User> blacklisted_users = get_blacklisted_users ();

	cerr << "format_result" << endl;
	string result = format_result
		(speakers_and_similarity,
		speaker_to_intersection,
		users_to_profile,
		blacklisted_users);
	
	cerr << "add_to_cache" << endl;
	add_to_cache (host, user, result);
}



