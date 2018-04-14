#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include <sstream>
#include "distsn.h"


using namespace std;


class UserAndFirstToot {
public:
	string host;
	string user;
	time_t first_toot_timestamp;
	string first_toot_url;
	bool blacklisted;
public:
	UserAndFirstToot () {};
	UserAndFirstToot
		(string a_host, string a_user, time_t a_first_toot_timestamp, string a_first_toot_url):
		host (a_host),
		user (a_user),
		first_toot_timestamp (a_first_toot_timestamp),
		first_toot_url (a_first_toot_url),
		blacklisted (false)
		{};
};


static vector <UserAndFirstToot> read_storage (FILE *in)
{
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
	auto json_array = json_value.get <picojson::array> ();
	
	vector <UserAndFirstToot> users_and_first_toots;
	
	for (auto user_value: json_array) {
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		string first_toot_timestamp_string = user_object.at (string {"first_toot_timestamp"}).get <string> ();
		time_t first_toot_timestamp;
		stringstream {first_toot_timestamp_string} >> first_toot_timestamp;
		string first_toot_url = user_object.at (string {"first_toot_url"}).get <string> ();
		users_and_first_toots.push_back (UserAndFirstToot {host, user, first_toot_timestamp, first_toot_url});
	}
	
	return users_and_first_toots;
}


static bool by_timestamp (const UserAndFirstToot &a, const UserAndFirstToot &b)
{
	return b.first_toot_timestamp < a.first_toot_timestamp;
}


static vector <UserAndFirstToot> get_users_and_first_toots (unsigned int limit)
{
	vector <UserAndFirstToot> users_in_all_hosts;
	time_t now = time (nullptr);
	set <string> hosts = get_international_hosts ();
	for (auto host: hosts) {
		cerr << host << endl;
		const string filename = string {"/var/lib/vinayaka/user-first-toot/"} + host + string {".json"};

		FILE *in = fopen (filename.c_str (), "r");
		if (in != nullptr) {
			vector <UserAndFirstToot> users_and_first_toots = read_storage (in);
			fclose (in);
			for (auto user_and_first_toot: users_and_first_toots) {
				if (static_cast <time_t> (now - limit) <= user_and_first_toot.first_toot_timestamp) {
					users_in_all_hosts.push_back (user_and_first_toot);
				}
			}
		}
	}

	sort (users_in_all_hosts.begin (), users_in_all_hosts.end (), by_timestamp);

	set <User> blacklisted_users = get_blacklisted_users ();
	for (auto & users_and_first_toot: users_in_all_hosts) {
		if (blacklisted_users.find
			(User {users_and_first_toot.host, users_and_first_toot.user}) != blacklisted_users.end ())
		{
			users_and_first_toot.blacklisted = true;
		}
	}

	return users_in_all_hosts;
}


int main (int argc, char **argv)
{
	unsigned int limit = 7 * 24 * 60 * 60;
	if (1 < argc) {
		stringstream {argv [1]} >> limit;
	}

	vector <UserAndFirstToot> users_and_first_toots = get_users_and_first_toots (limit);
	map <User, Profile> users_to_profile = read_profiles ();
	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < users_and_first_toots.size (); cn ++) {
		if (0 < cn) {
			cout << "," << endl;
		}
		auto user = users_and_first_toots.at (cn);
		cout
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"first_toot_timestamp\":\"" << user.first_toot_timestamp << "\","
			<< "\"first_toot_url\":\"" << user.first_toot_url << "\","
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ",";
			if (users_to_profile.find (User {user.host, user.user}) == users_to_profile.end ()) {
				cout
					<< "\"screen_name\":\"\","
					<< "\"avatar\":\"\"";
			} else {
				Profile profile = users_to_profile.at (User {user.host, user.user});
				cout << "\"screen_name\":\"" << escape_json (profile.screen_name) << "\",";
				if (safe_url (profile.avatar)) {
					cout << "\"avatar\":\"" << escape_json (profile.avatar) << "\"";
				} else {
					cout << "\"avatar\":\"\"";
				}
			}
		cout
			<< "}";
	}
	cout << "]";
	return 0;
}


