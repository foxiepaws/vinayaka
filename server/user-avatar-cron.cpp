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


class Profile {
public:
	string screen_name;
	string bio;
	string avatar;
};


static vector <User> get_users ()
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < 30000; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


static void write_to_storage (vector <pair <User, Profile>> users_and_profiles, ofstream &out)
{
	out << "[";
	for (unsigned int a = 0; a < users_and_profiles.size (); a ++) {
		if (0 < a) {
		  out << "," << endl;
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
		cerr << user.user << "@" << user.host << endl;
		try {
			string screen_name;
			string bio;
			string avatar;
			get_profile (user.host, user.user, screen_name, bio, avatar);
			Profile profile;
			profile.screen_name = screen_name;
			profile.bio = bio;
			profile.avatar = avatar;
			users_and_profiles.push_back (pair <User, Profile> {user, profile});
		} catch (ExceptionWithLineNumber e) {
			cerr << "Error " << e.line << endl;
		};
	};
	string filename {"/var/lib/vinayaka/user-profiles.json"};
	ofstream out {filename};
	write_to_storage (users_and_profiles, out);
}


