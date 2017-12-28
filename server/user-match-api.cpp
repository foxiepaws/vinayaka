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


class Profile {
public:
	string screen_name;
	string bio;
	string avatar;
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
	double similarity = static_cast <double> (intersection.size ());
	a_intersection = intersection;
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


static set <string> get_words_of_listener (vector <string> toots, vector <ModelTopology> models)
{
	set <string> words;
	for (auto model: models) {
		vector <string> words_in_a_model = get_words_from_toots (toots, model.word_length, model.vocabulary_size);
		words.insert (words_in_a_model.begin (), words_in_a_model.end ());
	}
	return words;
};


static map <User, set <string>> get_words_of_speakers ()
{
	map <User, set <string>> users_to_words;
	string filename {"/var/lib/vinayaka/user-words.csv"};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File not found: " << filename << endl;
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
			cerr << "ParseException " << e.line << " " << filename << endl;
		}
	}
	return users_to_words;
}


static map <User, Profile> read_profiles ()
{
	FILE * in = fopen ("/var/lib/vinayaka/user-profiles.json", "rb");
	if (in == nullptr) {
		return map <User, Profile> {};
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

	map <User, Profile> users_to_profile;

	for (auto user_value: array) {
		auto user_object = user_value.get <picojson::object> ();
		string host;
		if (user_object.find (string {"host"}) != user_object.end ()) {
			host = user_object.at (string {"host"}).get <string> ();
		}
		string user;
		if (user_object.find (string {"user"}) != user_object.end ()) {
			user = user_object.at (string {"user"}).get <string> ();
		}
		string screen_name;
		if (user_object.find (string {"screen_name"}) != user_object.end ()) {
			screen_name = user_object.at (string {"screen_name"}).get <string> ();
		}
		string bio;
		if (user_object.find (string {"bio"}) != user_object.end ()) {
			bio = user_object.at (string {"bio"}).get <string> ();
		}
		string avatar;
		if (user_object.find (string {"avatar"}) != user_object.end ()) {
			avatar = user_object.at (string {"avatar"}).get <string> ();
		}
		Profile profile;
		profile.screen_name = screen_name;
		profile.bio = bio;
		profile.avatar = avatar;
		users_to_profile.insert (pair <User, Profile> {User {host, user}, profile});
	}
	return users_to_profile;
}


static bool charactor_for_url (char c)
{
	return
		('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| ('0' <= c && c <= '9')
		|| c == ':'
		|| c == '/'
		|| c == '.'
		|| c == '-'
		|| c == '_'
		|| c == '%';
}


static bool safe_url (string url)
{
	for (auto c: url) {
		if (! charactor_for_url (c)) {
			return false;
		}
	}
	return true;
}


static string format_result
	(vector <UserAndSimilarity> speakers_and_similarity,
	map <User, set <string>> speaker_to_intersection,
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
		set <string> intersection_set = speaker_to_intersection.at (User {speaker.host, speaker.user});
		vector <string> intersection {intersection_set.begin (), intersection_set.end ()};
		out
			<< "{"
			<< "\"host\":\"" << escape_json (speaker.host) << "\","
			<< "\"user\":\"" << escape_json (speaker.user) << "\","
			<< "\"similarity\":" << speaker.similarity << ",";
		bool blacklisted = (blacklisted_users.find (User {speaker.host, speaker.user}) != blacklisted_users.end ());
		out << "\"blacklisted\":" << (blacklisted? "true": "false") << ",";

		if (users_to_profile.find (User {speaker.host, speaker.user}) == users_to_profile.end ()) {
			out
				<< "\"screen_name\":\"\","
				<< "\"bio\":\"\","
				<< "\"avatar\":\"\",";
		} else {
			Profile profile = users_to_profile.at (User {speaker.host, speaker.user});
			out
				<< "\"screen_name\":\"" << escape_json (profile.screen_name) << "\","
				<< "\"bio\":\"\",";
			if (safe_url (profile.avatar)) {
				out << "\"avatar\":\"" << escape_json (profile.avatar) << "\",";
			} else {
				out << "\"avatar\":\"\",";
			}
		}

		out << "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				out << ",";
			}
			out << "\"" << escape_json (escape_utf8_fragment (intersection [cn_intersection])) << "\"";
		}
		out << "]";

		out << "}";
	}
	out << "]";
	return out.str ();
}


static void add_to_cache (string host, string user, string result)
{
	ofstream out {"/var/lib/vinayaka/match-cache.csv", std::ofstream::out | std::ofstream::app};
	out << "\"" << escape_csv (host) << "\",";
	out << "\"" << escape_csv (user) << "\",";
	out << "\"" << escape_csv (result) << "\"" << endl;
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
	get_profile (true /* pagenation */, host, user, screen_name, bio, toots);
	toots.push_back (screen_name);
	toots.push_back (bio);

	vector <ModelTopology> models = {
		ModelTopology {6, 400},
		ModelTopology {9, 400},
		ModelTopology {12, 400},
	};
	
	set <string> words_of_listener = get_words_of_listener (toots, models);
	map <User, set <string>> speaker_to_words = get_words_of_speakers ();

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

	map <User, Profile> users_to_profile = read_profiles ();
	set <User> blacklisted_users = get_blacklisted_users ();

	string result = format_result (speakers_and_similarity, speaker_to_intersection, users_to_profile, blacklisted_users);
	cout << "Content-Type: application/json" << endl << endl;
	cout << result;
	
	add_to_cache (host, user, result);
}



