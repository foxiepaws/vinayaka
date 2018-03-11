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


static void write_to_storage (map <User, set <string>> users_to_toots, ofstream &out)
{
	for (auto user_to_toots: users_to_toots) {
		auto user = user_to_toots.first;
		auto toots = user_to_toots.second;
		for (auto toot: toots) {
			out
				<< "\"" << escape_csv (user.host) << "\","
				<< "\"" << escape_csv (user.user) << "\","
				<< "\"" << escape_csv (toot) << "\"" << endl;
		}
	}
}


static const unsigned int history_variations = 24;

	
static vector <User> get_users ()
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < 5000; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


static map <User, set <string>> read_users_to_toots (string filename)
{
	map <User, set <string>> users_to_toots;
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		throw (ModelException {__LINE__});
	}
	vector <vector <string>> table;
	try {
		table = parse_csv (in);
	} catch (ParseException e) {
		throw (ModelException {__LINE__});
	}
	for (auto row: table) {
		if (2 < row.size ()) {
			User user {row.at (0), row.at (1)};
			string toot = row.at (2);
			if (users_to_toots.find (user) == users_to_toots.end ()) {
				users_to_toots.insert (pair <User, set <string>> {user, set <string> {toot}});
			} else {
				users_to_toots.at (user).insert (toot);
			}
		}
	}
	return users_to_toots;
}


static void save_union_of_history ()
{
	map <User, set <string>> users_to_toots;

	auto users = get_users ();
	for (auto user: users) {
		users_to_toots.insert (pair <User, set <string>> {user, set <string> {}});
	}

	for (unsigned int cn = 0; cn < history_variations; cn ++) {
		stringstream filename;
		filename << "/var/lib/vinayaka/raw-toots/" << cn << ".csv";
		try {
			map <User, set <string>> users_to_toots_point = read_users_to_toots (filename.str ());
			for (auto user_to_toots_point: users_to_toots_point) {
				User user {user_to_toots_point.first};
				set <string> toots {user_to_toots_point.second};
				if (users_to_toots.find (user) != users_to_toots.end ()) {
					users_to_toots.at (user).insert (toots.begin (), toots.end ());
				}
			}
		} catch (ModelException e) {
			cerr << "ModelException " << e.line << " " << endl;
		}
	}
	ofstream out {"/var/lib/vinayaka/raw-toots/all.csv"};
	write_to_storage (users_to_toots, out);
}


int main (int argc, char **argv)
{
	save_union_of_history ();
}


