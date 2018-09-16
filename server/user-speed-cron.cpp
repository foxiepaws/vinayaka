#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <set>

#include <socialnet-1.h>

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


static void for_host (shared_ptr <socialnet::Host> host)
{
	/* Apply forgetting rate to memo. */
	const string storage_filename = string {"/var/lib/vinayaka/user-speed/"} + host->host_name;
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

	/* Get timeline. */
	vector <socialnet::Status> toots = host->get_local_timeline (60 * 60 * 3);

	/* Count occupancy */
	map <string, unsigned int> occupancy;

	for (auto toot: toots) {
		if (host->host_name == toot.host_name) {
			string username = toot.user_name;
			if (occupancy.find (username) == occupancy.end ()) {
				occupancy.insert (pair <string, unsigned int> (username, 1));
			} else {
				occupancy.at (username) ++;
			}
		}
	}

	/* Start time */
	time_t start_time;
	time (& start_time);

	time_t top_time = toots.front ().timestamp;
	time_t bottom_time = toots.back ().timestamp;

	double duration = max (start_time, top_time) - bottom_time;
	
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


int main (int argc, char **argv)
{
	auto hosts = socialnet::get_hosts ();

	for (auto host: hosts) {
		cerr << host->host_name << endl;
		try {
			for_host (host);
		} catch (socialnet::HttpException e) {
			cerr << "socialnet::HttpException" << " " << e.line << endl;
		} catch (socialnet::HostException e) {
			cerr << "socialnet::HostException" << " " << e.line << endl;
		}
	}

	return 0;
}

