#include <iostream>
#include "unistd.h"
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

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
	for (auto result: results) {
		auto result_object = result.get <picojson::object> ();
		if (result_object.at (string {"host"}).get <string> () == host
			&& result_object.at (string {"user"}).get <string> () == user)
		{
			cout << "Content-Type: application/json" << endl << endl;
			cout << result_object. at (string {"result"}).serialize ();
			return 0;
		}
	}
	execv ("/usr/local/bin/vinayaka-user-match-resource-guard", argv);
}


