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

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static vector <User> get_users ()
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed_impl (0.05 / (24.0 * 60.0 * 60.0));
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size (); cn ++) {
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
			<< "\"avatar\":\"" << escape_json (profile.avatar) << "\","
			<< "\"type\":\"" << escape_json (profile.type) << "\","
			<< "\"url\":\"" << escape_json (profile.url) << "\","
			<< "\"implementation\":\"" << socialnet::format (profile.implementation) << "\""
			<< "}";
	}
	out << "]";
}


int main (int argc, char **argv)
{
	auto users = get_users ();
	vector <pair <User, Profile>> users_and_profiles;

	socialnet::Hosts hosts;
	hosts.initialize ();
	
	unsigned int peaceful_age_count = 0;

	unsigned int cn = 0;

	for (auto user: users) {
		cerr << cn << " " << user.user << "@" << user.host << endl;
		cn ++;

		string screen_name;
		string bio;
		string avatar;
		string type;
		string url = string {"https://"} + user.host + string {"/users/"} + user.user;
		auto implementation = socialnet::eImplementation::UNKNOWN;
		
		if (peaceful_age_count < 16) {
			try {
				auto socialnet_user = hosts.make_user (user.host, user.user);
				if (! socialnet_user) {
					throw (socialnet::UserException {__LINE__});
				}
				url = socialnet_user->url ();
				implementation = socialnet_user->host->implementation ();
				socialnet_user->get_profile (screen_name, bio, avatar, type);
			} catch (socialnet::PeacefulAgeException e) {
				peaceful_age_count ++;
			} catch (socialnet::ExceptionWithLineNumber e) {
				cerr << "Error " << e.line << endl;
			};
		}

		Profile profile;
		profile.screen_name = screen_name;
		profile.bio = bio;
		profile.avatar = avatar;
		profile.type = type;
		profile.url = url;
		profile.implementation = implementation;
		users_and_profiles.push_back (pair <User, Profile> {user, profile});
	};
	string filename {"/var/lib/vinayaka/user-profiles.json"};
	ofstream out {filename};
	write_to_storage (users_and_profiles, out);
}


