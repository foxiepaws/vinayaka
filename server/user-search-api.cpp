#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <functional>
#include <tuple>
#include "picojson.h"
#include "distsn.h"


using namespace std;


class SearchResult {
public:
	string host;
	string user;
	string at;
	string text;
	string avatar;
	string type;
	bool blacklisted;
public:
	SearchResult ():
		blacklisted (false)
		{};
	SearchResult (string a_host, string a_user, string a_at, string a_text):
		host (a_host),
		user (a_user),
		at (a_at),
		text (a_text),
		blacklisted (false)
		{};
	bool operator < (const SearchResult &r) const {
		return make_tuple (host, user, at, text) < make_tuple (r.host, r.user, r.at, r.text);
	};
	string encode_to_json () const {
		string json;
		json += string {"{"};
		json += string {"\"host\":\""} + escape_json (host) + string {"\","};
		json += string {"\"user\":\""} + escape_json (user) + string {"\","};
		json += string {"\"at\":\""} + escape_json (at) + string {"\","};
		json += string {"\"text\":\""} + escape_json (text) + string {"\","};
		json += string {"\"avatar\":\""} + escape_json (avatar) + string {"\","};
		json += string {"\"type\":\""} + escape_json (type) + string {"\","};
		json += string {"\"blacklisted\":"} + (blacklisted? string {"true"}: string {"false"});
		json += string {"}"};
		return json;
	};
};


static string get_case_insensitive_string (string in) {
	string out;
	for (auto c: in) {
		out.push_back (('A' <= c && c <= 'Z')? c - 'A' + 'a': c);
	}
	return out;
}


class SearchTarget {
public:
	string host;
	string user;
	string at;
	string text;
public:
	SearchTarget () {};
	SearchTarget (string a_host, string a_user, string a_at, string a_text):
		host (a_host),
		user (a_user),
		at (a_at),
		text (a_text)
		{};
	bool hit (string keyword) const {
		string s = get_case_insensitive_string (text);
		return s.find (keyword) != s.npos;
	};
};


int main (int argc, char **argv)
{
	if (argc < 2) {
		exit (1);
	}
	string keyword = get_case_insensitive_string (string {argv [1]});

	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	map <User, Profile> users_to_profile = read_profiles ();

	vector <SearchTarget> search_targets;

	for (auto user_and_speed: users_and_speed) {
		string host = user_and_speed.host;
		string user = user_and_speed.username;
		search_targets.push_back (SearchTarget {host, user, string {"user_name"}, user});
		
		if (users_to_profile.find (User {host, user}) != users_to_profile.end ()) {
			string screen_name = users_to_profile.at (User {host, user}).screen_name;
			if (0 < screen_name.size ()) {
				search_targets.push_back (SearchTarget {host, user, string {"screen_name"}, screen_name});
			}
			string bio = users_to_profile.at (User {host, user}).bio;
			if (0 < bio.size ()) {
				search_targets.push_back (SearchTarget {host, user, string {"bio"}, bio});
			}
		}
	}

	vector <SearchResult> search_results;

	for (auto search_target: search_targets) {
		if (search_target.hit (keyword)) {
			search_results.push_back (SearchResult {search_target.host, search_target.user, search_target.at, search_target.text});
		}
	}

	sort (search_results.begin (), search_results.end ());

	for (auto & result: search_results) {
		User user {result.host, result.user};
		if (users_to_profile.find (user) != users_to_profile.end ()) {
			result.avatar = users_to_profile.at (user).avatar;
			result.type = users_to_profile.at (user).type;
		}
	}

	set <User> blacklisted_users = get_blacklisted_users ();
	for (auto &search_result: search_results) {
		bool blacklisted =
			(blacklisted_users.find (User {search_result.host, search_result.user}) != blacklisted_users.end ())
			|| (blacklisted_users.find (User {search_result.host, string {"*"}}) != blacklisted_users.end ());
		search_result.blacklisted = blacklisted;
	}

	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-type: application/json" << endl << endl;

	cout << "[";

	for (unsigned int cn = 0; cn < search_results.size (); cn ++) {
		if (0 < cn) {
			cout << "," << endl;
		}
		cout << search_results.at (cn).encode_to_json ();
	}

	cout << "]";

	return 0;
}

