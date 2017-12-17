#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <set>
#include "picojson.h"
#include "distsn.h"


using namespace std;


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


static string get_username (const picojson::value &toot)
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
	if (account_map.find (string {"username"}) == account_map.end ()) {
		throw (TootException {});
	}
	auto username = account_map.at (string {"username"});
	if (! username.is <string> ()) {
		throw (TootException {});
	}
	auto username_s = username.get <string> ();
	if (! valid_username (username_s)) {
		throw (TootException {});
	}
	return username_s;
}


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


static void write_storage (FILE *out, map <string, double> memo)
{
	fprintf (out, "{");
	for (auto user: memo) {
		string username = user.first;
		double speed = user.second;
		fprintf (out, "\"%s\":%e,", username.c_str (), speed);
	}
	fprintf (out, "}");
}


static void for_host (string host)
{
	/* Apply forgetting rate to memo. */
	const string storage_filename = string {"/var/lib/vinayaka/user-speed/"} + host;
	map <string, double> memo;

	FILE * storage_file_in = fopen (storage_filename.c_str (), "r");
	if (storage_file_in != nullptr) {
		memo = read_storage (storage_file_in);
		fclose (storage_file_in);
	}

	const double forgetting_rate = 1.0 / 8.0;

	for (auto &user_memo: memo) {
		user_memo.second *= (1.0 - forgetting_rate);
	}

	/* Save memo. */
	{
		FILE * storage_file_out = fopen (storage_filename.c_str (), "w");
		if (storage_file_out != nullptr) {
			write_storage (storage_file_out, memo);
			fclose (storage_file_out);
		}
	}

	/* Start time */
	time_t start_time;
	time (& start_time);

	/* Get timeline. */
	vector <picojson::value> toots = get_timeline (host);
	if (toots.size () < 40) {
		throw (HostException {});
	}

	const picojson::value &top_toot = toots.at (0);
	const picojson::value &bottom_toot = toots.at (toots.size () - 1);
	time_t top_time;
	time_t bottom_time;
	try {
		top_time = get_time (top_toot);
		bottom_time = get_time (bottom_toot);
	} catch (TootException e) {
		throw (HostException {});
	}

	double duration = max (start_time, top_time) - bottom_time;
	if (! (1.0 < duration && duration < 60 * 60 * 24 * 365)) {
		throw (HostException {});
	}

	map <string, unsigned int> occupancy;

	for (auto toot: toots) {
		string username = get_username (toot);
		if (occupancy.find (username) == occupancy.end ()) {
			occupancy.insert (pair <string, unsigned int> (username, 1));
		} else {
			occupancy.at (username) ++;
		}
	}
	
	/* Update memo. */
	for (auto user_occupancy: occupancy) {
		double speed = static_cast <double> (user_occupancy.second) / duration;
		auto user_memo = memo.find (user_occupancy.first);
		if (user_memo == memo.end ()) {
			memo.insert (pair <string, double> (user_occupancy.first, speed));
		} else {
			memo.at (user_occupancy.first) += forgetting_rate * speed;
		}
	}
	
	/* Save memo again. */
	{
		FILE * storage_file_out = fopen (storage_filename.c_str (), "w");
		if (storage_file_out != nullptr) {
			write_storage (storage_file_out, memo);
			fclose (storage_file_out);
		}
	}
}


static set <string> get_international_hosts_impl ()
{
	return set <string> {};
}


static set <string> get_international_hosts ()
{
	set <string> hosts = get_international_hosts_impl ();
	hosts.insert (string {"pleroma.soykaf.com"});
	hosts.insert (string {"pleroma.knzk.me"});
	hosts.insert (string {"ketsuben.red"});
	hosts.insert (string {"plrm.ht164.jp"});
	hosts.insert (string {"pleroma.vocalodon.net"});
	hosts.insert (string {"plero.ma"});
	hosts.insert (string {"2.distsn.org"});
	return hosts;
}


int main (int argc, char **argv)
{
	set <string> hosts = get_international_hosts ();
	for (auto host: hosts) {
		cerr << host << endl;
	}

	for (auto host: hosts) {
		try {
			for_host (string {host});
		} catch (HttpException e) {
			/* Nothing. */
		} catch (HostException e) {
			/* Nothing. */
		}
	}

	return 0;
}

