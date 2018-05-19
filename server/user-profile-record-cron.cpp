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


static map <User, set <string>> users_to_screen_names;
static map <User, set <string>> users_to_bios;
static map <User, string> users_to_avatar;
static map <User, string> users_to_type;


static vector <User> read_storage (FILE *in)
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
	
	vector <User> users;
	
	for (auto user_value: json_array) {
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		users.push_back (User {host, user});
	}
	
	return users;
}


static vector <User> get_users_in_all_hosts (set <string> hosts)
{
	vector <User> users;
	for (auto host: hosts) {
		cerr << host << endl;
		const string filename = string {"/var/lib/vinayaka/user-first-toot/"} + host + string {".json"};

		FILE *in = fopen (filename.c_str (), "r");
		if (in != nullptr) {
			vector <User> users_in_host = read_storage (in);
			fclose (in);
			for (auto user: users_in_host) {
				users.push_back (user);
			}
		}
	}
	return users;
}


static void get_profile_for_all_users (set <User> users)
{
	for (auto user: users) {
		string screen_name;
		string bio;
		string avatar;
		string type;
		cerr << user.user << "@" << user.host << endl;
		try {
			get_profile (user.host, user.user, screen_name, bio, avatar, type);
		} catch (ExceptionWithLineNumber e) {
			cerr << e.line << endl;
		}
		
		if (users_to_screen_names.find (user) == users_to_screen_names.end ()) {
			users_to_screen_names.insert (pair <User, set <string>> {user, set <string> {screen_name}});
		} else {
			users_to_screen_names.at (user).insert (screen_name);
		}
		
		if (users_to_bios.find (user) == users_to_bios.end ()) {
			users_to_bios.insert (pair <User, set <string>> {user, set <string> {bio}});
		} else {
			users_to_bios.at (user).insert (bio);
		}

		if (users_to_avatar.find (user) == users_to_avatar.end ()) {
			users_to_avatar.insert (pair <User, string> {user, avatar});
		}

		if (users_to_type.find (user) == users_to_type.end ()) {
			users_to_type.insert (pair <User, string> {user, type});
		}
	}
}


static map <User, set <string>> read_user_profile_record (FILE *in)
{
	map <User, set <string>> users_to_profiles;
	auto table = parse_csv (in);
	for (auto row: table) {
		if (2 < row.size ()) {
			User user {row.at (0), row.at (1)};
			string profile = row.at (2);
			if (users_to_profiles.find (user) == users_to_profiles.end ()) {
				users_to_profiles.insert (pair <User, set <string>> {user, set <string> {profile}});
			} else {
				users_to_profiles.at (user).insert (profile);
			}
		}
	}
	return users_to_profiles;
}


static set <User> read_user_record (FILE *in)
{
	set <User> users;
	auto table = parse_csv (in);
	for (auto row: table) {
		if (1 < row.size ()) {
			User user {row.at (0), row.at (1)};
			users.insert (user);
		}
	}
	return users;
}


static void write_user_profile_record (ofstream &out, map <User, set <string>> users_to_profiles)
{
	for (auto user_to_profiles: users_to_profiles) {
		auto user = user_to_profiles.first;
		auto profiles = user_to_profiles.second;
		for (auto profile: profiles) {
			out << "\"" << escape_csv (user.host) << "\",";
			out << "\"" << escape_csv (user.user) << "\",";
			out << "\"" << escape_csv (profile) << "\"" << endl;
		}
	}
}


static void write_user_profile_record (ofstream &out, map <User, string> users_to_profile)
{
	for (auto user_to_profile: users_to_profile) {
		auto user = user_to_profile.first;
		auto profile = user_to_profile.second;
		out << "\"" << escape_csv (user.host) << "\",";
		out << "\"" << escape_csv (user.user) << "\",";
		out << "\"" << escape_csv (profile) << "\"" << endl;
	}
}


static void write_user_record (ofstream &out, set <User> users)
{
	for (auto user: users) {
		out << "\"" << escape_csv (user.host) << "\",";
		out << "\"" << escape_csv (user.user) << "\"," << endl;
	}
}


int main (int argc, char **argv)
{

	set <string> hosts = get_international_hosts ();
	// set <string> hosts = {"3.distsn.org"};

	vector <User> users_vector = get_users_in_all_hosts (hosts);

	set <User> users;
	{
		FILE *in = fopen ("/var/lib/vinayaka/user-record.csv", "r");
		if (in != nullptr) {
			users = read_user_record (in);
			fclose (in);
		}
	}

	users.insert (users_vector.begin (), users_vector.end ());

	{
		ofstream out {"/var/lib/vinayaka/user-record.csv"};
		write_user_record (out, users);
	}


	{
		FILE *in = fopen ("/var/lib/vinayaka/user-profile-record-screen-name.csv", "r");
		if (in != nullptr) {
			users_to_screen_names = read_user_profile_record (in);
			fclose (in);
		}
	}

	{
		FILE *in = fopen ("/var/lib/vinayaka/user-profile-record-bio.csv", "r");
		if (in != nullptr) {
			users_to_bios = read_user_profile_record (in);
			fclose (in);
		}
	}

	get_profile_for_all_users (users);

	{
		ofstream out {"/var/lib/vinayaka/user-profile-record-screen-name.csv"};
		write_user_profile_record (out, users_to_screen_names);
	}

	{
		ofstream out {"/var/lib/vinayaka/user-profile-record-bio.csv"};
		write_user_profile_record (out, users_to_bios);
	}

	{
		ofstream out {"/var/lib/vinayaka/user-profile-record-avatar.csv"};
		write_user_profile_record (out, users_to_avatar);
	}

	{
		ofstream out {"/var/lib/vinayaka/user-profile-record-type.csv"};
		write_user_profile_record (out, users_to_type);
	}

	return 0;
}

