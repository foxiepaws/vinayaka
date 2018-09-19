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

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


const unsigned int minimum_popularity = 64;


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


static double get_similarity
	(set <string> listener_set,
	set <string> speaker_set,
	map <string, double> & a_intersection,
	map <string, unsigned int> words_to_popularity)
{
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));

	double similarity = 0;
	a_intersection.clear ();

	for (string word: intersection) {
		unsigned int popularity = minimum_popularity;
		if (words_to_popularity.find (word) != words_to_popularity.end ()) {
			popularity = words_to_popularity.at (word);
		}
		double rarity = static_cast <double> (minimum_popularity) * 10.0 * get_rarity (popularity);
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
	vector <ModelTopology> models,
	map <string, unsigned int> words_to_popularity)
{
	set <string> words;
	for (auto model: models) {
		vector <string> words_in_a_model
			= get_words_from_toots (toots, model.word_length, model.vocabulary_size, words_to_popularity, minimum_popularity);
		words.insert (words_in_a_model.begin (), words_in_a_model.end ());
	}
	return words;
};


static map <User, set <string>> get_words_of_speakers (string filename)
{
	FileLock lock {filename, LOCK_SH};

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


static map <string, unsigned int> get_words_to_popularity (string filename)
{
	FileLock lock {filename, LOCK_SH};

	map <string, unsigned int> words_to_popularity;
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
					stringstream popularity_stream {row.at (1)};
					unsigned int popularity;
					popularity_stream >> popularity;
					if (minimum_popularity < popularity
						&& words_to_popularity.find (word) == words_to_popularity.end ())
					{
						words_to_popularity.insert (pair <string, unsigned int> {word, popularity});
					}
				}
			}
		} catch (ParseException e) {
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return words_to_popularity;
}


class WordAndRarity {
public:
	string word;
	double rarity;
public:
	WordAndRarity () { };
	WordAndRarity (string a_word, double a_rarity): word (a_word), rarity (a_rarity) { };
};


bool by_rarity_desc (const WordAndRarity &a, const WordAndRarity &b)
{
	return b.rarity < a.rarity;
};


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
		vector <WordAndRarity> intersection_vector;
		for (auto word_to_rarity: intersection_map) {
			intersection_vector.push_back (WordAndRarity {word_to_rarity.first, word_to_rarity.second});
		}
		stable_sort (intersection_vector.begin (), intersection_vector.end (), by_rarity_desc);

		out
			<< "{"
			<< "\"host\":\"" << escape_json (speaker.host) << "\","
			<< "\"user\":\"" << escape_json (speaker.user) << "\","
			<< "\"similarity\":" << scientific << speaker.similarity << ",";
		bool blacklisted
			= (blacklisted_users.find (User {speaker.host, speaker.user}) != blacklisted_users.end ())
			|| (blacklisted_users.find (User {speaker.host, string {"*"}}) != blacklisted_users.end ());
		out << "\"blacklisted\":" << (blacklisted? "true": "false") << ",";

		if (users_to_profile.find (User {speaker.host, speaker.user}) == users_to_profile.end ()) {
			out
				<< "\"screen_name\":\"\","
				<< "\"bio\":\"\","
				<< "\"avatar\":\"\","
				<< "\"type\":\"\","
				<< "\"url\":\"\","
				<< "\"implementation\":\"unknown\",";
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
			out << "\"url\":\"" << escape_json (profile.url) << "\",";
			out << "\"implementation\":\"" << socialnet::format (profile.implementation) << "\",";
		}

		out << "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection_vector.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				out << ",";
			}
			string word = intersection_vector.at (cn_intersection).word;
			double rarity = intersection_vector.at (cn_intersection).rarity;
			out << "{";
			out << "\"word\":\"" << escape_json (escape_utf8_fragment (word)) << "\",";
			out << "\"rarity\":" << scientific << rarity;
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
	string file_name {"/var/lib/vinayaka/match-cache.csv"};
	FileLock lock {file_name};
	ofstream out {file_name.c_str (), std::ofstream::out | std::ofstream::app};
	time_t now = time (nullptr);
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

	auto http = make_shared <socialnet::Http> ();
	auto socialnet_user = socialnet::make_user (host, user, http);

	string screen_name;
	string bio;
	string avatar;
	string type;
	cerr << "get_profile" << endl;
	socialnet_user->get_profile (screen_name, bio, avatar, type);

	vector <string> toots;
	toots.push_back (screen_name);
	toots.push_back (bio);

	vector <socialnet::Status> socialnet_statuses = socialnet_user->get_timeline (10);

	for (unsigned int cn = 0; cn < socialnet_statuses.size () && cn < 80; cn ++) {
		toots.push_back (socialnet_statuses.at (cn).content);
	}

	const unsigned int vocabulary_size {1600};
	vector <ModelTopology> models = {
		ModelTopology {6, vocabulary_size},
	};
	
	cerr << "get_words_to_popularity" << endl;
	map <string, unsigned int> words_to_popularity
		= get_words_to_popularity (string {"/var/lib/vinayaka/model/popularity.csv"});
		
	cerr << "get_words_of_listener" << endl;
	set <string> words_of_listener = get_words_of_listener (toots, models, words_to_popularity);
	
	cerr << "get_words_of_speakers" << endl;
	map <User, set <string>> speaker_to_words
		= get_words_of_speakers (string {"/var/lib/vinayaka/model/concrete-user-words.csv"});
	
	vector <UserAndSimilarity> speakers_and_similarity;
	map <User, map <string, double>> speaker_to_intersection;
	
	cerr << "get_similarity" << endl;
	unsigned int cn = 0;
	for (auto speaker_and_words: speaker_to_words) {
		if (cn % 100 == 0) {
			cerr << cn << " ";
		}
		cn ++;

		User speaker = speaker_and_words.first;
		set <string> words_of_speaker = speaker_and_words.second;
		map <string, double> intersection;
		double similarity = get_similarity (words_of_listener, words_of_speaker, intersection, words_to_popularity);
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



