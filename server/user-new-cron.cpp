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


class UserAndFirstToot {
public:
	string host;
	string user;
	time_t first_toot_timestamp;
	string first_toot_url;
	bool blacklisted;
	string screen_name;
	string bio;
	string avatar;
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


static string get_acct (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"account"}) == properties.end ()) {
		throw (TootException {});
	}
	auto account = properties.at (string {"account"});
	if (! account.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto account_map = account.get <picojson::object> ();

	if (account_map.find (string {"acct"}) == account_map.end ()) {
		throw (TootException {});
	}
	auto acct = account_map.at (string {"acct"});
	if (! acct.is <string> ()) {
		throw (TootException {});
	}
	auto acct_s = acct.get <string> ();
	return acct_s;
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
		string first_toot_url = user_object.at (string {"first_toot_url"}).get <string> ();
		users_and_first_toots.push_back (UserAndFirstToot {host, user, first_toot_timestamp, first_toot_url});
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
		out << "\"first_toot_timestamp\":\"" << user_and_first_toot.first_toot_timestamp << "\",";
		out << "\"first_toot_url\":\"" << escape_json (user_and_first_toot.first_toot_url) << "\"";
		out << "}";
	}
	out << "]" << endl;
}


static string get_url (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {__LINE__});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"url"}) == properties.end ()) {
		throw (TootException {__LINE__});
	}
	auto url_object = properties.at (string {"url"});
	if (! url_object.is <string> ()) {
		throw (TootException {__LINE__});
	}
	return url_object.get <string> ();
}


static string get_uri (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {__LINE__});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"uri"}) == properties.end ()) {
		throw (TootException {__LINE__});
	}
	auto uri_object = properties.at (string {"uri"});
	if (! uri_object.is <string> ()) {
		throw (TootException {__LINE__});
	}
	return uri_object.get <string> ();
}


static void get_host_and_user_from_acct (string a_acct, string &a_host, string &a_user)
{
	string host;
	string user;
	unsigned int state = 0;
	for (auto c: a_acct) {
		switch (state) {
		case 0:
			if (c == '@') {
				state = 1;
			} else {
				user.push_back (c);
			}
			break;
		case 1:
			host.push_back (c);
			break;
		default:
			abort ();
		}
	}
	a_host = host;
	a_user = user;
}


time_t get_user_registration_time (const picojson::value &toot_value)
{
	if (! toot_value.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto toot_object = toot_value.get <picojson::object> ();
	if (toot_object.find (string {"account"}) == toot_object.end ()) {
		throw (TootException {});
	}
	auto account_value = toot_object.at (string {"account"});
	if (! account_value.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto account_object = account_value.get <picojson::object> ();
	if (account_object.find (string {"created_at"}) == account_object.end ()) {
		throw (TootException {});
	}
	auto created_at_value = account_object.at (string {"created_at"});
	if (! created_at_value.is <string> ()) {
		throw (TootException {});
	}
	auto created_at_value_string = created_at_value.get <string> ();
	return str2time (created_at_value_string);
}


static void for_host (string host)
{
	map <User, UserAndFirstToot> users_to_first_toot;

	const string filename = string {"/var/lib/vinayaka/user-first-toot/"} + host + string {".json"};

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

	vector <picojson::value> toots = get_timeline (host);

	for (auto toot: toots) {
		try {
			string acct = get_acct (toot);
			string host_in_acct;
			string user_in_acct;
			get_host_and_user_from_acct (acct, host_in_acct, user_in_acct);
			if ((host_in_acct.empty () || host_in_acct == host)
				&& valid_username (user_in_acct))
			{
				User user {host_in_acct.empty ()? host: host_in_acct, user_in_acct};
				time_t timestamp = get_user_registration_time (toot);
				string url;
				try {
					url = get_url (toot);
				} catch (TootException) {
					url = get_uri (toot);
				}
				UserAndFirstToot user_and_first_toot {user.host, user.user, timestamp, url};

				if (users_to_first_toot.find (user) == users_to_first_toot.end ()) {
					users_to_first_toot.insert (pair <User, UserAndFirstToot> {user, user_and_first_toot});
				} else {
					if (timestamp < users_to_first_toot.at (user).first_toot_timestamp) {
						users_to_first_toot.at (user) = user_and_first_toot;
					}
				}
			}
		} catch (TootException e) {
			cerr << "TootException " << e.line << endl;
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
		if (blacklisted_users.find
			(User {users_and_first_toot.host, users_and_first_toot.user}) != blacklisted_users.end ())
		{
			users_and_first_toot.blacklisted = true;
		}
	}

	return users_in_all_hosts;
}


static void get_profile_for_all_users (vector <UserAndFirstToot> &users_and_first_toots)
{
	for (auto &user_and_first_toot: users_and_first_toots) {
		string host = user_and_first_toot.host;
		string user = user_and_first_toot.user;
		string screen_name;
		string bio;
		string avatar;
		cerr << user << "@" << host << endl;
		try {
			get_profile (host, user, screen_name, bio, avatar);
		} catch (ExceptionWithLineNumber e) {
			cerr << e.line << endl;
		}
		user_and_first_toot.screen_name = screen_name;
		user_and_first_toot.bio = bio;
		user_and_first_toot.avatar = avatar;
	}
}


static vector <UserAndFirstToot> get_newcomers (vector <UserAndFirstToot> users_and_first_toots)
{
	vector <UserAndFirstToot> newcomers;
	for (auto user: users_and_first_toots) {
		bool elder_bool = false;
		try {
			elder_bool = elder (user.host, user.user);
		} catch (ExceptionWithLineNumber e) {
			cerr << "elder (" << user.host << ", " << user.user << ") " << e.line << endl;
		}
		if (! elder_bool) {
			newcomers.push_back (user);
		}
	}
	return newcomers;
}


static void cache_sorted_result (set <string> hosts)
{
	unsigned int limit = 1 * 24 * 60 * 60;
	vector <UserAndFirstToot> users_and_first_toots = get_users_in_all_hosts (limit, hosts);
	vector <UserAndFirstToot> newcomers = get_newcomers (users_and_first_toots);
	get_profile_for_all_users (newcomers);

	const string filename {"/var/lib/vinayaka/users-new-cache.json"};
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
		if (safe_url (user.first_toot_url)) {
			out << "\"first_toot_url\":\"" << user.first_toot_url << "\",";
		} else {
			out << "\"first_toot_url\":\"\",";
		}
		out
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ","
			<< "\"screen_name\":\"" << escape_json (user.screen_name) << "\","
			<< "\"bio\":\"" << escape_json (user.bio) << "\",";
		if (safe_url (user.avatar)) {
			out << "\"avatar\":\"" << escape_json (user.avatar) << "\"";
		} else {
			out << "\"avatar\":\"\"";
		}
		out
			<< "}";
	}
	out << "]";
}


int main (int argc, char **argv)
{

	set <string> hosts = get_international_hosts ();
	// set <string> hosts = {"3.distsn.org", "theboss.tech"};

	for (auto host: hosts) {
		cerr << host << endl;
		try {
			for_host (host);
		} catch (HttpException e) {
			cerr << "Error " << e.line << endl;
		} catch (HostException e) {
			cerr << "Error " << e.line << endl;
		}
	}
	
	cache_sorted_result (hosts);

	return 0;
}

