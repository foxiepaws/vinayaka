#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include "picojson.h"
#include "distsn.h"


using namespace std;


static map <string, double> read_storage (FILE *in)
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
	auto object = json_value.get <picojson::object> ();
	
	map <string, double> memo;
	
	for (auto user: object) {
		string username = user.first;
		double speed = user.second.get <double> ();
		memo.insert (pair <string, double> (username, speed));
	}
	
	return memo;
}


static bool by_speed (const UserAndSpeed &a, const UserAndSpeed b)
{
	return b.speed < a.speed;
}


set <User> get_blacklisted_users ()
{
	const string filename {"/etc/vinayaka/blacklisted_users.csv"};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File " << filename << " not found." << endl;
		return set <User> {};
	}
	set <User> users;
	vector <vector <string>> table = parse_csv (in);
	for (auto row: table) {
		if (1 < row.size ()) {
			string host = row.at (0);
			string user = row.at (1);
			users.insert (User {host, user});
		}
	}
	return users;
}


vector <UserAndSpeed> get_users_and_speed ()
{
	return get_users_and_speed (0.1 / (24.0 * 60.0 * 60.0));
}


vector <UserAndSpeed> get_users_and_speed (double limit)
{
	set <User> blacklisted_users = get_blacklisted_users ();

	set <string> hosts = get_international_hosts ();

	vector <UserAndSpeed> users;

	for (auto host: hosts) {
		map <string, double> speeds;
		{
			const string storage_filename = string {"/var/lib/vinayaka/user-speed/"} + host;
			FILE * storage_file_in = fopen (storage_filename.c_str (), "r");
			if (storage_file_in != nullptr) {
				speeds = read_storage (storage_file_in);
				fclose (storage_file_in);
			}
		}
		for (auto i: speeds) {
			string username = i.first;
			double speed = i.second;
			bool blacklisted
				= (blacklisted_users.find (User {host, username}) != blacklisted_users.end ())
				|| (blacklisted_users.find (User {host, string {"*"}}) != blacklisted_users.end ());
			UserAndSpeed user {host, username, speed, blacklisted};
			users.push_back (user);
		}
	}
	
	vector <UserAndSpeed> active_users;
	for (auto i: users) {
		if (limit <= i.speed) {
			active_users.push_back (i);
		}
	}

	sort (active_users.begin (), active_users.end (), by_speed);
	
	return active_users;
}

