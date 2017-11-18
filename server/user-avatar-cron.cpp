#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <random>
#include "picojson.h"
#include "tinyxml2.h"
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


class User {
public:
	string host;
	string user;
public:
	User () { /* Do nothing. */ };
	User (string a_host, string a_user) {
		host = a_host;
		user = a_user;
	};
	bool operator < (const User &r) const {
		return host < r.host || (host == r.host && user < r.user);
	};
};


class Profile {
public:
	string screen_name;
	string bio;
	string avatar;
};


static vector <User> get_users ()
{
	vector <User> users;
	string query = string {"http://distsn.org/cgi-bin/distsn-user-recommendation-api.cgi?10000"};
	cerr << query << endl;
	string reply = http_get (query);
	picojson::value json_value;
	string error = picojson::parse (json_value, reply);
	if (! error.empty ()) {
		cerr << error << endl;
		exit (1);
	}
	auto user_jsons = json_value.get <picojson::array> ();
	for (auto user_json: user_jsons) {
		auto user_object = user_json.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string username = user_object.at (string {"username"}).get <string> ();
		users.push_back (User {host, username});
	}
	return users;
}


static void write_to_storage (vector <pair <User, Profile>> users_and_profiles, ofstream &out)
{
	out << "[";
	for (unsigned int a = 0; a < users_and_profiles.size (); a ++) {
		if (0 < a) {
			out << ",";
		}
		auto user_and_profile = users_and_profiles.at (a);
		auto user = user_and_profile.first;
		auto profile = user_and_profile.second;
		out
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"screen_name\":\"" << escape_json (profile.screen_name) << "\","
			<< "\"bio\":\"" << escape_json (profile.bio) << "\","
			<< "\"avatar\":\"" << escape_json (profile.avatar) << "\""
			<< "}";
	}
	out << "]";
}


int main (int argc, char **argv)
{
	auto users = get_users ();
	vector <pair <User, Profile>> users_and_profiles;
	for (auto user: users) {
		string screen_name;
		string bio;
		vector <string> toots;
		string avatar;
		get_profile (user.host, user.user, screen_name, bio, toots, avatar);
		Profile profile;
		profile.screen_name = screen_name;
		profile.bio = bio;
		profile.avatar = avatar;
		users_and_profiles.push_back (pair <User, Profile> {user, profile});
	};
	string filename {"/var/lib/vinayaka/user-profiles.json"};
	ofstream out {filename};
	write_to_storage (users_and_profiles, out);
}


