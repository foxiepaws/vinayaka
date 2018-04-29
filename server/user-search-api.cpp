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
#include "distsn.h"


using namespace std;


class SearchResult {
public:
	string host;
	string user;
	string at;
	string text;
	string avatar;
public:
	SearchResult () {};
	SearchResult (string a_host, string a_user, string a_at, string a_text):
		host (a_host),
		user (a_user),
		at (a_at),
		text (a_text)
		{};
	string encode_to_json () const {
		string json;
		json += string {"{"};
		json += string {"\"host\":\""} + escape_json (host) + string {"\","};
		json += string {"\"user\":\""} + escape_json (user) + string {"\","};
		json += string {"\"at\":\""} + escape_json (at) + string {"\","};
		json += string {"\"text\":\""} + escape_json (text) + string {"\","};
		json += string {"\"avatar\":\""} + escape_json (avatar) + string {"\""};
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

	vector <SearchTarget> search_targets;

	{
		vector <vector <string>> table;
		FILE *in = fopen ("/var/lib/vinayaka/user-record.csv", "r");
		if (in != nullptr) {
			table = parse_csv (in);
			fclose (in);
		}
		for (auto row: table) {
			if (1 < row.size ()) {
				search_targets.push_back (SearchTarget {row.at (0), row.at (1), string {"user_name"}, row.at (1)});
			}
		}
	}

	{
		vector <vector <string>> table;
		FILE *in = fopen ("/var/lib/vinayaka/user-profile-record-screen-name.csv", "r");
		if (in != nullptr) {
			table = parse_csv (in);
			fclose (in);
		}
		for (auto row: table) {
			if (2 < row.size ()) {
				search_targets.push_back (SearchTarget {row.at (0), row.at (1), string {"screen_name"}, row.at (2)});
			}
		}
	}

	{
		vector <vector <string>> table;
		FILE *in = fopen ("/var/lib/vinayaka/user-profile-record-bio.csv", "r");
		if (in != nullptr) {
			table = parse_csv (in);
			fclose (in);
		}
		for (auto row: table) {
			if (2 < row.size ()) {
				search_targets.push_back (SearchTarget {row.at (0), row.at (1), string {"bio"}, row.at (2)});
			}
		}
	}

	vector <SearchResult> search_results;

	for (auto search_target: search_targets) {
		if (search_target.hit (keyword)) {
			search_results.push_back (SearchResult {search_target.host, search_target.user, search_target.at, search_target.text});
		}
	}

	{
		vector <vector <string>> table;
		FILE *in = fopen ("/var/lib/vinayaka/user-profile-record-avatar.csv", "r");
		if (in != nullptr) {
			table = parse_csv (in);
			fclose (in);
		}
		map <User, string> users_to_avatar;
		for (auto row: table) {
			if (2 < row.size ()) {
				users_to_avatar.insert (pair <User, string> {User {row.at (0), row.at (1)}, row.at (2)});
			}
		}
		for (auto & result: search_results) {
			User user {result.host, result.user};
			if (users_to_avatar.find (user) != users_to_avatar.end ()) {
				result.avatar = users_to_avatar.at (user);
			}
		}
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

