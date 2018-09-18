#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static string get_advanced_api (string in, set <socialnet::HostNameAndUserName> friends)
{
	string out;

	picojson::value json_value;
	string json_parse_error = picojson::parse (json_value, in);
	if (! json_parse_error.empty ()) {
		cerr << json_parse_error << endl;
		exit (1);
	}

	auto users_array = json_value.get <picojson::array> ();
	
	out += string {"["};

	for (unsigned int cn = 0; cn < users_array.size (); cn ++) {
		if (0 < cn) {
			out += string {","};
		}
	
		auto user_value = users_array.at (cn);
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		double similarity = user_object.at (string {"similarity"}).get <double> ();
		bool blacklisted = user_object.at (string {"blacklisted"}).get <bool> ();
		string screen_name = user_object.at (string {"screen_name"}).get <string> ();
		string bio = user_object.at (string {"bio"}).get <string> ();
		string avatar = user_object.at (string {"avatar"}).get <string> ();
		bool following_bool = socialnet::following (host, user, friends);
		string type = user_object.at (string {"type"}).get <string> ();
		string url = user_object.at (string {"url"}).get <string> ();

		vector <string> intersection_vector;
		map <string, double> intersection_map;
		auto intersection_array = user_object.at (string {"intersection"}).get <picojson::array> ();
		for (auto word_and_score_value: intersection_array) {
			auto word_and_score_object = word_and_score_value.get <picojson::object> ();
			auto word_value = word_and_score_object.at (string {"word"});
			auto word_string = word_value.get <string> ();
			auto score_value = word_and_score_object.at (string {"rarity"});
			auto score_double = score_value.get <double> ();
			intersection_vector.push_back (word_string);
			intersection_map.insert (pair <string, double> {word_string, score_double});
		}
		
		stringstream out_user;
		out_user
			<< "{"
			<< "\"host\":\"" << escape_json (host) << "\","
			<< "\"user\":\"" << escape_json (user) << "\","
			<< "\"similarity\":" << scientific << similarity << ","
			<< "\"blacklisted\":" << (blacklisted? "true": "false") << ","
			<< "\"screen_name\":\"" << escape_json (screen_name) << "\","
			<< "\"bio\":\"" << escape_json (bio) << "\","
			<< "\"avatar\":\"" << escape_json (avatar) << "\","
			<< "\"type\":\"" << escape_json (type) << "\","
			<< "\"url\":\"" << escape_json (url) << "\",";
		out_user
			<< "\"intersection\":[";
		for (unsigned int cn_intersection = 0; cn_intersection < intersection_vector.size (); cn_intersection ++) {
			if (0 < cn_intersection) {
				out_user << ",";
			}
			string word = intersection_vector.at (cn_intersection);
			double score = intersection_map.at (word);
			out_user << "{";
			out_user << "\"word\":\"" << escape_json (escape_utf8_fragment (word)) << "\",";
			out_user << "\"rarity\":" << scientific << score;
			out_user << "}";
		}
		out_user
			<< "],";
		out_user
			<< "\"following\":" << (following_bool? "true": "false")
			<< "}";
		out += out_user.str ();
	}

	out += string {"]"};

	return out;
}


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	bool hit;
	string result = fetch_cache (host, user, hit);
	if (hit) {
		auto socialnet_user = socialnet::make_user (host, user, make_shared <socialnet::Http> ());
		auto friends = socialnet_user->get_friends ();
		cout << "Access-Control-Allow-Origin: *" << endl;
		cout << "Content-Type: application/json" << endl << endl;
		cout << get_advanced_api (result, friends);
	} else {
		pid_t pid = fork ();
		if (pid == 0) {
			execv ("/usr/local/bin/vinayaka-user-match-impl", argv);
		} else {
			auto socialnet_user = socialnet::make_user (host, user, make_shared <socialnet::Http> ());
			auto friends = socialnet_user->get_friends ();
			int status;
			waitpid (pid, &status, 0);
			string result_2 = fetch_cache (host, user, hit);
			cout << "Access-Control-Allow-Origin: *" << endl;
			cout << "Content-Type: application/json" << endl << endl;
			cout << get_advanced_api (result_2, friends);
		}
	}
}


