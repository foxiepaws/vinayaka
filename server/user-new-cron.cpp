#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


class UserAndFirstToot {
public:
	string host;
	string user;
	time_t first_toot_timestamp;
	bool blacklisted;
	string screen_name;
	string bio;
	string avatar;
	string type;
	string url;
public:
	UserAndFirstToot () {};
	UserAndFirstToot
		(string a_host, string a_user, time_t a_first_toot_timestamp):
		host (a_host),
		user (a_user),
		first_toot_timestamp (a_first_toot_timestamp),
		blacklisted (false)
		{};
};


static bool valid_character (char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_');
}


static bool valid_username (string s)
{
	for (auto c: s) {
		if (! valid_character (c)) {
			return false;
		}
	}
	return true;
}


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
		users_and_first_toots.push_back (UserAndFirstToot {host, user, first_toot_timestamp});
	}
	
	return users_and_first_toots;
}


static void write_storage (ofstream &out, vector <UserAndFirstToot> users_and_first_toots)
{
	out << "[" << endl;
	for (unsigned int cn = 0; cn < users_and_first_toots.size (); cn ++) {
		if (0 < cn) {
			out << "," << endl;
		}
		auto user_and_first_toot = users_and_first_toots.at (cn);
		out << "{";
		out << "\"host\":\"" << escape_json (user_and_first_toot.host) << "\",";
		out << "\"user\":\"" << escape_json (user_and_first_toot.user) << "\",";
		out << "\"first_toot_timestamp\":\"" << user_and_first_toot.first_toot_timestamp << "\"";
		out << "}";
	}
	out << "]" << endl;
}


static void for_host (shared_ptr <socialnet::Host> socialnet_host)
{
	map <User, UserAndFirstToot> users_to_first_toot;

	const string filename = string {"/var/lib/vinayaka/user-first-toot/"} + socialnet_host->host_name + string {".json"};

	{
		FILE *in = fopen (filename.c_str (), "r");
		if (in != nullptr) {
			vector <UserAndFirstToot> users_and_first_toots = read_storage (in);
			fclose (in);
			for (auto user_and_first_toot: users_and_first_toots) {
				User user {user_and_first_toot.host, user_and_first_toot.user};
				users_to_first_toot.insert (pair <User, UserAndFirstToot> {user, user_and_first_toot});
			}
		}
	}

	auto toots = socialnet_host->get_local_timeline (3 * 60 * 60);

	for (auto toot: toots) {
		if (valid_username (toot.user_name)) {
			User user {toot.host_name, toot.user_name};
			UserAndFirstToot user_and_first_toot {toot.host_name, toot.user_name, toot.user_timestamp};
			if (users_to_first_toot.find (user) == users_to_first_toot.end ()) {
				users_to_first_toot.insert (pair <User, UserAndFirstToot> {user, user_and_first_toot});
			} else {
				if (toot.user_timestamp < users_to_first_toot.at (user).first_toot_timestamp) {
					users_to_first_toot.at (user) = user_and_first_toot;
				}
			}
		}
	}

	{
		vector <UserAndFirstToot> users_and_first_toots;
		for (auto user_to_first_toot: users_to_first_toot) {
			users_and_first_toots.push_back (user_to_first_toot.second);
		}
		ofstream out {filename};
		write_storage (out, users_and_first_toots);
	}
}


static bool by_timestamp (const UserAndFirstToot &a, const UserAndFirstToot &b)
{
	return b.first_toot_timestamp < a.first_toot_timestamp;
}


static vector <UserAndFirstToot> get_users_in_all_hosts (unsigned int limit, set <string> hosts)
{
	vector <UserAndFirstToot> users_in_all_hosts;
	time_t now = time (nullptr);
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
		if (blacklisted_users.find (User {users_and_first_toot.host, users_and_first_toot.user}) != blacklisted_users.end ()
			|| blacklisted_users.find (User {users_and_first_toot.host, string {"*"}}) != blacklisted_users.end ())
		{
			users_and_first_toot.blacklisted = true;
		}
	}

	return users_in_all_hosts;
}


static void get_profile_for_all_users (vector <UserAndFirstToot> &users_and_first_toots)
{
	auto http = make_shared <socialnet::Http> ();

	for (auto &user_and_first_toot: users_and_first_toots) {
		string host = user_and_first_toot.host;
		string user = user_and_first_toot.user;
		string screen_name;
		string bio;
		string avatar;
		string type;
		string url;
		cerr << user << "@" << host << endl;
		try {
			auto socialnet_user = socialnet::make_user (host, user, http);
			socialnet_user->get_profile (screen_name, bio, avatar, type);
			url = socialnet_user->url ();
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << e.line << endl;
		}
		user_and_first_toot.screen_name = screen_name;
		user_and_first_toot.bio = bio;
		user_and_first_toot.avatar = avatar;
		user_and_first_toot.type = type;
		user_and_first_toot.url = url;
	}
}


static void cache_sorted_result (set <string> hosts)
{
	unsigned int limit = 3 * 24 * 60 * 60;
	vector <UserAndFirstToot> newcomers_raw = get_users_in_all_hosts (limit, hosts);

	set <User> optouted_users = get_optouted_users ();
	vector <UserAndFirstToot> newcomers;
	for (auto newcomer: newcomers_raw) {
		if (optouted_users.find (User {newcomer.host, newcomer.user}) == optouted_users.end ()) {
			newcomers.push_back (newcomer);
		}
	}

	get_profile_for_all_users (newcomers);

	const string filename {"/var/lib/vinayaka/users-new-cache.json"};
	FileLock {filename};
	ofstream out {filename};

	out << "[";
	for (unsigned int cn = 0; cn < newcomers.size (); cn ++) {
		if (0 < cn) {
			out << "," << endl;
		}
		auto user = newcomers.at (cn);
		out
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"first_toot_timestamp\":\"" << user.first_toot_timestamp << "\",";
		if (safe_url (user.url)) {
			out << "\"url\":\"" << user.url << "\",";
		} else {
			out << "\"url\":\"\",";
		}
		out
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ","
			<< "\"screen_name\":\"" << escape_json (user.screen_name) << "\","
			<< "\"bio\":\"" << escape_json (user.bio) << "\",";
		if (safe_url (user.avatar)) {
			out << "\"avatar\":\"" << escape_json (user.avatar) << "\",";
		} else {
			out << "\"avatar\":\"\",";
		}
		out << "\"type\":\"" << escape_json (user.type) << "\"";
		out
			<< "}";
	}
	out << "]";
}


int main (int argc, char **argv)
{
	auto hosts = socialnet::get_hosts ();

	unsigned int peaceful_age_count = 0;

	for (auto host: hosts) {
		cerr << host->host_name << endl;
		try {
			for_host (host);
		} catch (socialnet::PeacefulAgeException e) {
			peaceful_age_count ++;
			if (16 < peaceful_age_count) {
				break;
			}
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << "Error " << e.line << endl;
		}
	}

	set <string> host_names;
	for (auto host: hosts) {
		host_names.insert (host->host_name);
	}

	cache_sorted_result (host_names);

	return 0;
}

