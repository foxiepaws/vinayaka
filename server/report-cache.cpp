#include <iostream>
#include "unistd.h"
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	picojson::array results;
	FILE * in = fopen ("/var/lib/vinayaka/match-cache.json", "rb");
	if (in != nullptr) {
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
		results = json_value.get <picojson::array> ();
		fclose (in);
	}
	cout << "Content-Type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < results.size (); cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto result = results.at (cn);
		auto result_object = result.get <picojson::object> ();
		string host = result_object.at (string {"host"}).get <string> ();
		string user = result_object.at (string {"user"}).get <string> ();
		cout << "{"
			<< "\"host\":\"" << escape_json (host) << "\","
			<< "\"user\":\"" << escape_json (user) << "\""
			<< "}";
	}
	cout << "]";
}


