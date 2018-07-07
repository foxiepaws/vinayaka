#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <cassert>
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


string get_id (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"id"}) == properties.end ()) {
		throw (TootException {});
	}
	auto id_object = properties.at (string {"id"});
	string id_string;
	if (id_object.is <double> ()) {
		double id_double = id_object.get <double> ();
		stringstream s;
		s << static_cast <unsigned int> (id_double);
		id_string = s.str ();
	} else if (id_object.is <string> ()) {
		id_string = id_object.get <string> ();
	} else {
		throw (TootException {});
	}
	return id_string;
}


vector <picojson::value> get_timeline (string host)
{
	Http http;
	vector <picojson::value> timeline;

	{
		string reply = http.perform (string {"https://"} + host + string {"/api/v1/timelines/public?local=true&limit=40"});

		picojson::value json_value;
		string error = picojson::parse (json_value, reply);
		if (! error.empty ()) {
			throw (HostException {});
		}
		if (! json_value.is <picojson::array> ()) {
			throw (HostException {});
		}
	
		vector <picojson::value> toots = json_value.get <picojson::array> ();
		timeline.insert (timeline.end (), toots.begin (), toots.end ());
	}
	
	if (timeline.size () < 1) {
		throw (HostException {});
	}
	
	for (unsigned int cn = 0; ; cn ++) {
		if (1000 <= cn) {
			throw (HostException {__LINE__});
		}
		
		time_t top_time;
		time_t bottom_time;
		try {
			top_time = get_time (timeline.front ());
			bottom_time = get_time (timeline.back ());
		} catch (TootException e) {
			throw (HostException {});
		}
		if (60 * 60 * 3 <= top_time - bottom_time && 40 <= timeline.size ()) {
			break;
		}

		string bottom_id;
		try {
			bottom_id = get_id (timeline.back ());
		} catch (TootException e) {
			throw (HostException {});
		}
		string query
			= string {"https://"}
			+ host
			+ string {"/api/v1/timelines/public?local=true&limit=40&max_id="}
			+ bottom_id;
		string reply = http.perform (query);

		picojson::value json_value;
		string error = picojson::parse (json_value, reply);
		if (! error.empty ()) {
			throw (HostException {});
		}
		if (! json_value.is <picojson::array> ()) {
			throw (HostException {});
		}
	
		vector <picojson::value> toots = json_value.get <picojson::array> ();
		if (toots.empty ()) {
			break;
		}
		timeline.insert (timeline.end (), toots.begin (), toots.end ());
	}

	return timeline;
}


static int writer (char * data, size_t size, size_t nmemb, std::string * writerData)
{
	if (writerData == nullptr) {
		return 0;
	}
	writerData->append (data, size * nmemb);
	return size * nmemb;
}


HttpGlobal::HttpGlobal ()
{
	curl_global_init (CURL_GLOBAL_ALL);
}


Http::Http ()
{
	curl = curl_easy_init ();
	if (! curl) {
		throw (HttpException {__LINE__});
	}
}


string Http::perform (string url)
{
	return perform (url, vector <string> {});
}


string Http::perform (string url, vector <string> headers)
{
	CURLcode res;

	struct curl_slist * list = nullptr;
	for (auto &header: headers) {
		list = curl_slist_append (list, header.c_str ());
	}

	curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
	curl_easy_setopt (curl, CURLOPT_HTTPHEADER, list);
	string reply_1;
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, & reply_1);
	curl_easy_setopt (curl,  CURLOPT_CONNECTTIMEOUT, 60);
	curl_easy_setopt (curl,  CURLOPT_TIMEOUT, 60);
	res = curl_easy_perform (curl);
	long response_code;
	if (res == CURLE_OK) {
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, & response_code);
		if (response_code != 200) {
			cerr << "HTTP response: " << response_code << endl;
			throw (HttpException {__LINE__});
		}
	} else {
		throw (HttpException {__LINE__});
	}
	return reply_1;
}


string Http::endure (string url)
{
	CURLcode res;

	curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
	string reply_1;
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, & reply_1);
	curl_easy_setopt (curl,  CURLOPT_CONNECTTIMEOUT, 120);
	res = curl_easy_perform (curl);
	long response_code;
	if (res == CURLE_OK) {
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, & response_code);
		if (response_code != 200) {
			cerr << "HTTP response: " << response_code << endl;
			throw (HttpException {__LINE__});
		}
	} else {
		throw (HttpException {__LINE__});
	}
	return reply_1;
}


Http::~Http ()
{
	curl_easy_cleanup (curl);
}


time_t get_time (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"created_at"}) == properties.end ()) {
		throw (TootException {});
	}
	auto time_object = properties.at (string {"created_at"});
	if (! time_object.is <string> ()) {
		throw (TootException {});
	}
	auto time_s = time_object.get <string> ();
	return str2time (time_s);
}


time_t str2time (string s)
{
	struct tm tm;
	strptime (s.c_str (), "%Y-%m-%dT%H:%M:%S", & tm);
	return timegm (& tm);
}


string escape_json (string in)
{
	string out;
	for (auto c: in) {
		if (c == '\n') {
			out += string {"\\n"};
		} else if (0x00 <= c && c < 0x20) {
			out += string {"�"};
		} else if (c == '"'){
			out += string {"\\\""};
		} else if (c == '\\'){
			out += string {"\\\\"};
		} else {
			out.push_back (c);
		}
	}
	return out;
}


static string remove_html (string in)
{
	string out;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '<') {
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == '>') {
				state = 0;
			}
			break;
		default:
			abort ();
		}
	}
	return out;
}


static string remove_url (string in)
{
	string out;
	unsigned int state = 0;
	string cache;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == 'h') {
				cache.push_back (c);
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == 't') {
				cache.push_back (c);
				state = 2;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 2:
			if (c == 't') {
				cache.push_back (c);
				state = 3;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 3:
			if (c == 'p') {
				cache.push_back (c);
				state = 4;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 4:
			if (c == 's') {
				cache.push_back (c);
				state = 5;
			} else if (c == ':') {
				cache.push_back (c);
				state = 6;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 5:
			if (c == ':') {
				cache.push_back (c);
				state = 6;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 6:
			if (c == '/') {
				cache.push_back (c);
				state = 7;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 7:
			if (c == '/') {
				cache.clear ();
				state = 8;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 8:
			if (0x20 < c && c < 0x7f) {
				/* Do notthing. */
			} else {
				out.push_back (c);
				state = 0;
			}
			break;
		default:
			abort ();
		}
	}
	out += cache;
	return out;
}


static string remove_character_reference (string in)
{
	string out;
	string code;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '&') {
				code.push_back (c);
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == ';') {
				code.push_back (c);
				if (code == string {"&amp;"}) {
					out += string {"&"};
				} else if (code == string {"&lt;"}) {
					out += string {"<"};
				} else if (code == string {"&gt;"}) {
					out += string {">"};
				} else if (code == string {"&quot;"}) {
					out += string {"\""};
				} else if (code == string {"&apos;"}) {
					out += string {"'"};
				} else if (code == string {"&#39;"}) {
					out += string {"'"};
				} else {
					out += code;
				}
				code.clear ();
				state = 0;
			} else {
				code.push_back (c);
			}
			break;
		default:
			assert (false);
		}
	}
	out += code;
	return out;
}


static bool character_for_username (char c)
{
	return ('a' <= c && c <= 'z')
	|| ('A' <= c && c <= 'Z')
	|| ('0' <= c && c <= '9')
	|| c == '@'
	|| c == '_'
	|| c == '-'
	|| c == '.';
}


static string remove_mention (string in)
{
	string out;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '@') {
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (! character_for_username (c)) {
				out.push_back (c);
				state = 0;
			}
			break;
		default:
			assert (false);
		}
	}
	return out;
}


static string small_letterize (string in)
{
	string out;
	for (auto c: in) {
		if ('A' <= c && c <= 'Z') {
			out.push_back (c - 'A' + 'a');
		} else {
			out.push_back (c);
		}
	}
	return out;
}


class OccupancyCount {
public:
	string word;
	double count;
public:
	OccupancyCount (): count (0) { /* Do nothing. */};
	OccupancyCount (string a_word, double a_count) {
		word = a_word;
		count = a_count;
	};
};


static bool by_count_desc (const OccupancyCount &a, const OccupancyCount &b)
{
	return b.count < a.count || (a.count == b.count && b.word < a.word);
}


static bool starts_with_utf8_codepoint_boundary (string word)
{
	unsigned int first_octet = static_cast <unsigned char> (word.at (0));
	return (! (0x80 <= first_octet && first_octet < 0xC0));
}


static bool is_hiragana (string codepoint)
{
	return (string {"ぁ"} <= codepoint && codepoint <= string {"ゟ"})
		|| codepoint == string {"、"}
		|| codepoint == string {"。"}
		|| codepoint == string {"？"}
		|| codepoint == string {"！"}
		|| codepoint == string {"思"};
}


static bool is_katakana (string codepoint)
{
	/* 長音「ー」を含む。 */
	return (string {"ァ"} <= codepoint && codepoint <= string {"ヿ"})
		|| codepoint == string {"「"}
		|| codepoint == string {"」"};
}


static bool all_kana (string word)
{
	for (unsigned int cn = 0; cn * 3 + 2 < word.size (); cn ++) {
		string codepoint = word.substr (cn * 3, 3);
		if (! (is_hiragana (codepoint) || is_katakana (codepoint))) {
			return false;
		}
	}
	return true;
}


static bool all_hiragana (string word)
{
	for (unsigned int cn = 0; cn * 3 + 2 < word.size (); cn ++) {
		string codepoint = word.substr (cn * 3, 3);
		if (! is_hiragana (codepoint)) {
			return false;
		}
	}
	return true;
}


static bool is_space (char c)
{
	return c == ' ' || c == '\n';
}


static unsigned int number_of_spaces (string s)
{
	unsigned int cn = 0;
	for (auto c: s) {
		if (is_space (c)) {
			cn ++;
		}
	}
	return cn;
}


static bool starts_with_alphabet (string word)
{
	char c = word.at (0);
	return ('a' <= c && c <= 'z')
		||  ('A' <= c && c <= 'Z')
		||  ('0' <= c && c <= '9')
		||  (! (0x00 <= c && c <= 0x7f));
}


static bool is_hashtag (string word)
{
	return word.at (0) == '#';
}


static unsigned int length_of_first_word (string word)
{
	for (unsigned int cn = 0; cn < word.size (); cn ++) {
		auto c = word.at (cn);
		if (is_space (c)) {
			return cn;
		}
	}
	return word.size ();
}


static bool valid_word (string word)
{
	bool invalid
		= (! starts_with_utf8_codepoint_boundary (word))
		|| (! starts_with_alphabet (word))
		|| length_of_first_word (word) < 4
		|| is_hashtag (word)
		|| (word.size () < 9 && all_kana (word))
		|| (word.size () < 12 && all_hiragana (word))
		|| (word.size () < 9 && 2 <= number_of_spaces (word))
		|| (word.size () < 12 && 3 <= number_of_spaces (word))
		|| (word.size () < 15 && 4 <= number_of_spaces (word));
	return ! invalid;
}


static bool enquete_toot (string toot)
{
	return toot.find (string {"friends.nico"}) != string::npos
		&& toot.find (string {"アンケート"}) != string::npos;
}


static bool posted_by_third_party (string toot)
{
	return toot.find ("https://shindanmaker.com/") != string::npos
		|| toot.find ("http://mystery.distsn.org/") != string::npos
		|| toot.find ("https://mystery.distsn.org/") != string::npos
		|| toot.find ("https://vote.thedesk.top/") != string::npos;
}


static bool valid_toot (string toot)
{
	return (! enquete_toot (toot)) && (! posted_by_third_party (toot));
}


static bool is_alphabet (char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}


static bool separate_at_alphabet (string toot, unsigned int offset)
{
	return
		0 < offset
		&& offset < toot.size ()
		&& is_alphabet (toot.at (offset - 1))
		&& is_alphabet (toot.at (offset));
}


static bool valid_position (string toot, unsigned int offset)
{
	return ! separate_at_alphabet (toot, offset);
}


vector <string> get_words_from_toots (vector <string> toots, unsigned int word_length, unsigned int vocabulary_size)
{
	return get_words_from_toots (toots, word_length, vocabulary_size, map <string, unsigned int> {}, 1);
}


vector <string> get_words_from_toots
	(vector <string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size,
	map <string, unsigned int> words_to_occupancy,
	unsigned int minimum_occupancy)
{
	map <string, unsigned int> occupancy_count_map;
	for (auto raw_toot: toots) {
		string toot = remove_html (raw_toot);
		toot = remove_url (toot);
		toot = remove_mention (toot);
		toot = remove_character_reference (toot);
		toot = small_letterize (toot);
		if (valid_toot (toot) && word_length <= toot.size ()) {
			for (unsigned int offset = 0; offset <= toot.size () - word_length; offset ++) {
				string word = toot.substr (offset, word_length);
				if (valid_position (toot, offset) && valid_word (word)) {
					if (occupancy_count_map.find (word) == occupancy_count_map.end ()) {
						occupancy_count_map.insert (pair <string, unsigned int> {word, 1});
					} else {
						occupancy_count_map.at (word) ++;
					}
				}
			}
		}
	}

	vector <OccupancyCount> occupancy_count_vector;
	for (auto i: occupancy_count_map) {
		string word {i.first};
		unsigned int occupancy_in_this_user {i.second};
		unsigned int occupancy_in_all_users = minimum_occupancy;
		if (words_to_occupancy.find (word) != words_to_occupancy.end ()) {
			occupancy_in_all_users = words_to_occupancy.at (word);
		}
		double rarity = get_rarity (occupancy_in_all_users);
		double score = static_cast <double> (occupancy_in_this_user) * rarity;
		occupancy_count_vector.push_back (OccupancyCount {word, score});
	}
	sort (occupancy_count_vector.begin (), occupancy_count_vector.end (), by_count_desc);
	
	vector <string> words;
	for (unsigned int cn = 0; cn < occupancy_count_vector.size () && cn < vocabulary_size; cn ++) {
		words.push_back (occupancy_count_vector.at (cn).word);
	}
	
	return words;
}


double get_rarity (unsigned int occupancy)
{
	return static_cast <double> (1.0) / static_cast <double> (occupancy);
}


static void get_profile_impl (bool pagenation, string atom_query, string &a_screen_name, string &a_bio, vector <string> &a_toots)
{
	try {
		Http http;
		vector <string> timeline;

		cerr << atom_query << endl;
		string atom_reply = http.perform (atom_query);
		
		XMLDocument atom_document;
		XMLError atom_parse_error = atom_document.Parse (atom_reply.c_str ());
		if (atom_parse_error != XML_SUCCESS) {
			throw (UserException {__LINE__});
		}
		XMLElement * root_element = atom_document.RootElement ();
		if (root_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * feed_element = root_element;

		XMLElement * author_element = feed_element->FirstChildElement ("author");
		if (author_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * displayname_element = author_element->FirstChildElement ("poco:displayName");
		a_screen_name = string {};
		if (displayname_element != nullptr) {
			const char * displayname_text = displayname_element->GetText ();
			if (displayname_text != nullptr) {
				a_screen_name = string {displayname_text};
			}
		}
		a_bio = string {};
		XMLElement * note_element = author_element->FirstChildElement ("poco:note");
		if (note_element != nullptr) {
			const char * note_text = note_element->GetText ();
			if (note_text != nullptr) {
				a_bio = string {note_text};
			}
		}
		
		string next_url;
		if (pagenation) {
			for (XMLElement * link_element = feed_element->FirstChildElement ("link");
				link_element != nullptr;
				link_element = link_element->NextSiblingElement ("link"))
			{
				if (link_element->Attribute ("rel", "next") && link_element->Attribute ("href")) {
					next_url = link_element->Attribute ("href");
				}
			}
		}
		
		vector <string> toots;
		for (XMLElement * entry_element = feed_element->FirstChildElement ("entry");
			entry_element != nullptr;
			entry_element = entry_element->NextSiblingElement ("entry"))
		{
			XMLElement * verb_element = entry_element->FirstChildElement ("activity:verb");
			if (verb_element == nullptr) {
				throw (UserException {__LINE__});
			}
			const char * verb_text = verb_element->GetText ();
			if (verb_text == nullptr) {
				throw (UserException {__LINE__});
			}
			if (string {verb_text} == string {"http://activitystrea.ms/schema/1.0/post"}) {
				XMLElement * content_element = entry_element->FirstChildElement ("content");
				if (content_element != nullptr) {
					const char * content_text = content_element->GetText ();
					if (content_text != nullptr) {
						toots.push_back (string {content_text});
					}
				}
			}
		}
		
		timeline.insert (timeline.end (), toots.begin (), toots.end ());

		for (unsigned int cn = 0; cn < 10; cn ++) {
			if (next_url.empty ()) {
				break;
			}
			cerr << next_url << endl;
			string atom_reply = http.perform (next_url);
		
			XMLDocument atom_document;
			XMLError atom_parse_error = atom_document.Parse (atom_reply.c_str ());
			if (atom_parse_error != XML_SUCCESS) {
				throw (UserException {__LINE__});
			}
			XMLElement * root_element = atom_document.RootElement ();
			if (root_element == nullptr) {
				throw (UserException {__LINE__});
			}
			XMLElement * feed_element = root_element;

			next_url.clear ();
			for (XMLElement * link_element = feed_element->FirstChildElement ("link");
				link_element != nullptr;
				link_element = link_element->NextSiblingElement ("link"))
			{
				if (link_element->Attribute ("rel", "next") && link_element->Attribute ("href")) {
					next_url = link_element->Attribute ("href");
				}
			}
		
			vector <string> toots;
			for (XMLElement * entry_element = feed_element->FirstChildElement ("entry");
				entry_element != nullptr;
				entry_element = entry_element->NextSiblingElement ("entry"))
			{
				XMLElement * verb_element = entry_element->FirstChildElement ("activity:verb");
				if (verb_element == nullptr) {
					throw (UserException {__LINE__});
				}
				const char * verb_text = verb_element->GetText ();
				if (verb_text == nullptr) {
					throw (UserException {__LINE__});
				}
				if (string {verb_text} == string {"http://activitystrea.ms/schema/1.0/post"}) {
					XMLElement * content_element = entry_element->FirstChildElement ("content");
					if (content_element != nullptr) {
						const char * content_text = content_element->GetText ();
						if (content_text != nullptr) {
							toots.push_back (string {content_text});
						}
					}
				}
			}
			timeline.insert (timeline.end (), toots.begin (), toots.end ());
		}

		a_toots = timeline;
	} catch (HttpException e) {
		throw (UserException {__LINE__});
	}
}


void get_profile (bool pagenation, string host, string user, string &a_screen_name, string &a_bio, vector <string> &a_toots)
{
	try {
		string query = string {"https://"} + host + string {"/users/"} + user + string {".atom"};
		get_profile_impl (pagenation, query, a_screen_name, a_bio, a_toots);
	} catch (UserException e) {
		string query = string {"https://"} + host + string {"/users/"} + user + string {"/feed.atom"};
		get_profile_impl (pagenation, query, a_screen_name, a_bio, a_toots);
	}
}


void get_profile (string host, string user, string &a_screen_name, string &a_bio, string & a_avatar, string & a_type)
{
	Http http;
	get_profile (host, user, a_screen_name, a_bio, a_avatar, a_type, http);
}


void get_profile (string host, string user, string &a_screen_name, string &a_bio, string & a_avatar, string & a_type, Http & http)
{
	string query = string {"https://"} + host + string {"/users/"} + user;
	vector <string> headers {string {"Accept: application/activity+json"}};
	cerr << query << endl;
	string reply = http.perform (query, headers);
        picojson::value reply_value;
	string error = picojson::parse (reply_value, reply);
	if (! error.empty ()) {
		cerr << __LINE__ << " " << error << endl;
		throw (UserException {__LINE__});
	}
	if (! reply_value.is <picojson::object> ()) {
		throw (UserException {__LINE__});
	}
	auto reply_object = reply_value.get <picojson::object> ();

	if (reply_object.find (string {"name"}) != reply_object.end ()
		&& reply_object.at (string {"name"}).is <string> ())
	{
		string name_string = reply_object.at (string {"name"}).get <string> ();
		a_screen_name = name_string;
	}

	if (reply_object.find (string {"summary"}) != reply_object.end ()
		&& reply_object.at (string {"summary"}).is <string> ())
	{
		string summary_string = reply_object.at (string {"summary"}).get <string> ();
		a_bio = summary_string;
	}

	if (reply_object.find (string {"type"}) != reply_object.end ()
		&& reply_object.at (string {"type"}).is <string> ())
	{
		string type_string = reply_object.at (string {"type"}).get <string> ();
		a_type = type_string;
	}

	if (reply_object.find (string {"icon"}) != reply_object.end ()
		&& reply_object.at (string {"icon"}).is <picojson::object> ())
	{
		auto icon_object = reply_object.at (string {"icon"}).get <picojson::object> ();
		if (icon_object.find (string {"url"}) != icon_object.end ()
			&& icon_object.at (string {"url"}).is <string> ())
		{
			string url_string = icon_object.at (string {"url"}).get <string> ();
			a_avatar = url_string;
		}
	}
}


static bool elder_impl (string atom_query)
{
	time_t first_toot_timestamp = numeric_limits <time_t>::max ();

	try {
		Http http;
		vector <string> timeline;

		cerr << atom_query << endl;
		string atom_reply = http.perform (atom_query);
		
		XMLDocument atom_document;
		XMLError atom_parse_error = atom_document.Parse (atom_reply.c_str ());
		if (atom_parse_error != XML_SUCCESS) {
			throw (UserException {__LINE__});
		}
		XMLElement * root_element = atom_document.RootElement ();
		if (root_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * feed_element = root_element;

		for (XMLElement * entry_element = feed_element->FirstChildElement ("entry");
			entry_element != nullptr;
			entry_element = entry_element->NextSiblingElement ("entry"))
		{
			XMLElement * verb_element = entry_element->FirstChildElement ("activity:verb");
			if (verb_element == nullptr) {
				throw (UserException {__LINE__});
			}
			const char * verb_text = verb_element->GetText ();
			if (verb_text == nullptr) {
				throw (UserException {__LINE__});
			}
			if (string {verb_text} == string {"http://activitystrea.ms/schema/1.0/post"}) {
				XMLElement * published_element = entry_element->FirstChildElement ("published");
				if (published_element != nullptr) {
					const char * published_text = published_element->GetText ();
					if (published_text != nullptr) {
						time_t timestamp = str2time (string {published_text});
						if (timestamp < first_toot_timestamp) {
							first_toot_timestamp = timestamp;
						}
					}
				}
			}
		}
	} catch (HttpException e) {
		throw (UserException {__LINE__});
	}

	return first_toot_timestamp <= time (nullptr) - (1 * 24 * 60 * 60);
}


bool elder (string host, string user)
{
	bool elder_bool = false;
	try {
		string query = string {"https://"} + host + string {"/users/"} + user + string {".atom"};
		elder_bool = elder_impl (query);
	} catch (UserException e) {
		string query = string {"https://"} + host + string {"/users/"} + user + string {"/feed.atom"};
		elder_bool = elder_impl (query);
	}
	return elder_bool;
}


vector <string> get_words (bool pagenation, string host, string user, unsigned int word_length, unsigned int vocabulary_size)
{
	cerr << user << "@" << host << endl;
	string screen_name;
	string bio;
	vector <string> toots;
	get_profile (pagenation, host, user, screen_name, bio, toots);
	toots.push_back (screen_name);
	toots.push_back (bio);
	vector <string> words = get_words_from_toots (toots, word_length, vocabulary_size);
	return words;
}


vector <UserAndWords> read_storage (string filename)
{
	FILE * in = fopen (filename.c_str (), "rb");
	if (in == nullptr) {
		throw (ModelException {__LINE__});
	}
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
        auto array = json_value.get <picojson::array> ();
        
        vector <UserAndWords> memo;
        
        for (auto user_value: array) {
        	auto user_object = user_value.get <picojson::object> ();
        	string user = user_object.at (string {"user"}).get <string> ();
        	string host = user_object.at (string {"host"}).get <string> ();
        	auto words_array = user_object.at (string {"words"}).get <picojson::array> ();
        	vector <string> words;
        	for (auto word_value: words_array) {
        		string word = word_value.get <string> ();
        		words.push_back (word);
        	}
        	UserAndWords user_and_words;
        	user_and_words.user = user;
        	user_and_words.host = host;
        	user_and_words.words = words;
        	memo.push_back (user_and_words);
        }
	return memo;
}


vector <vector <string>> parse_csv (FILE *in)
{
	string s;
	for (; ; ) {
	char b [1024];
		auto fgets_return = fgets (b, 1024, in);
		if (fgets_return == nullptr) {
			break;
		}
		s += string {b};
	}

	unsigned int state = 0;
	string cell_cache;
	vector <string> row_cache;
	vector <vector <string>> table_cache;

	unsigned int line = 1;

	for (auto c: s) {
		if (c == '\n') {
			line ++;
		}
		switch (state) {
		case 0:
			if (c == '"') {
				state = 1;
			} else if (c == '\r') {
				/* Do nothing, */
			} else if (c == '\n') {
				table_cache.push_back (row_cache);
				row_cache.clear ();
			} else {
				cerr << "Line: " << line << endl;
				cerr << "Character: " << c << endl;
				throw ParseException {__LINE__};
			}
			break;
		case 1:
			if (c == '"') {
				state = 2;
			} else {
				cell_cache.push_back (c);
			}
			break;
		case 2:
			if (c == '"') {
				cell_cache.push_back ('"');
				state = 1;
			} else if (c == ',') {
				row_cache.push_back (cell_cache);
				cell_cache.clear ();
				state = 0;
			} else if (c == '\r') {
				/* Do nothing, */
			} else if (c == '\n') {
				row_cache.push_back (cell_cache);
				cell_cache.clear ();
				table_cache.push_back (row_cache);
				row_cache.clear ();
				state = 0;
			} else {
				cerr << "Line: " << line << endl;
				cerr << "Character: " << c << endl;
				throw ParseException {__LINE__};
			}
			break;
		default:
			throw ParseException {__LINE__};
	}
}

return table_cache;
}


string escape_csv (string in)
{
	string out;
	for (auto c: in) {
		if (c == '"') {
			out.push_back ('"');
			out.push_back ('"');
		} else {
			out.push_back (c);
		}
	}
	return out;
}


string escape_utf8_fragment (string in) {
	string out;
	for (unsigned int cn = 0; cn < in.size (); cn ++) {
		unsigned int c = static_cast <unsigned char> (in.at (cn));
		if ((0x00 <= c && c < 0x20) || c == ' ') {
			out += "�";
		} else if ((0xc2 <= c && c <= 0xdf && in.size () - cn < 2)
			|| (0xe0 <= c && c <= 0xef && in.size () - cn < 3)
			|| (0xf0 <= c && c <= 0xf7 && in.size () - cn < 4)
			|| (0xf8 <= c && c <= 0xfb && in.size () - cn < 5)
			|| (0xfc <= c && c <= 0xfd && in.size () - cn < 6))
		{
			for (unsigned int x = 0; x < in.size () - cn; x ++) {
				out += "�";
			}
			break;
		} else {
			out.push_back (c);
		}
	}
	return out;
}


set <string> get_international_hosts ()
{
	Http http;
	const string url {"http://distsn.org/cgi-bin/instances-api.cgi"};
	string reply = http.endure (url);

	picojson::value reply_value;
	string error = picojson::parse (reply_value, reply);
	if (! error.empty ()) {
		cerr << __LINE__ << " " << error << endl;
		return set <string> {};
	}
	if (! reply_value.is <picojson::array> ()) {
		cerr << __LINE__ << endl;
		return set <string> {};
	}
	auto instances_array = reply_value.get <picojson::array> ();
	
	set <string> hosts;
	
	for (auto instance_value: instances_array) {
		if (instance_value.is <string> ()) {
			string instance_string = instance_value.get <string> ();
			hosts.insert (instance_string);
		}
	}

	/* Manually add. */
	hosts.insert (string {"switter.at"});

	return hosts;
}


map <User, Profile> read_profiles ()
{
	FILE * in = fopen ("/var/lib/vinayaka/user-profiles.json", "rb");
	if (in == nullptr) {
		return map <User, Profile> {};
	}
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
	auto array = json_value.get <picojson::array> ();

	map <User, Profile> users_to_profile;

	for (auto user_value: array) {
		auto user_object = user_value.get <picojson::object> ();
		string host;
		if (user_object.find (string {"host"}) != user_object.end ()) {
			host = user_object.at (string {"host"}).get <string> ();
		}
		string user;
		if (user_object.find (string {"user"}) != user_object.end ()) {
			user = user_object.at (string {"user"}).get <string> ();
		}
		string screen_name;
		if (user_object.find (string {"screen_name"}) != user_object.end ()) {
			screen_name = user_object.at (string {"screen_name"}).get <string> ();
		}
		string bio;
		if (user_object.find (string {"bio"}) != user_object.end ()) {
			bio = user_object.at (string {"bio"}).get <string> ();
		}
		string avatar;
		if (user_object.find (string {"avatar"}) != user_object.end ()) {
			avatar = user_object.at (string {"avatar"}).get <string> ();
		}
		string type;
		if (user_object.find (string {"type"}) != user_object.end ()) {
			type = user_object.at (string {"type"}).get <string> ();
		}
		Profile profile;
		profile.screen_name = screen_name;
		profile.bio = bio;
		profile.avatar = avatar;
		profile.type = type;
		users_to_profile.insert (pair <User, Profile> {User {host, user}, profile});
	}
	return users_to_profile;
}


static bool charactor_for_url (char c)
{
	return
		('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| ('0' <= c && c <= '9')
		|| c == ':'
		|| c == '/'
		|| c == '.'
		|| c == '-'
		|| c == '_'
		|| c == '%'
		|| c == '@';
}


bool safe_url (string url)
{
	for (auto c: url) {
		if (! charactor_for_url (c)) {
			return false;
		}
	}
	return true;
}


string fetch_cache (string a_host, string a_user, bool & a_hit)
{
	a_hit = false;

	vector <vector <string>> table;
	FILE * in = fopen ("/var/lib/vinayaka/match-cache.csv", "rb");
	if (in != nullptr) {
		table = parse_csv (in);
		fclose (in);
	}

	time_t now = time (nullptr);

	for (auto row: table) {
		if (3 < row.size ()
			&& row.at (0) == a_host
			&& row.at (1) == a_user)
		{
			stringstream timestamp_sstream {row.at (3)};
			time_t timestamp_time_t;
			timestamp_sstream >> timestamp_time_t;
			if (difftime (now, timestamp_time_t) < 60 * 60) {
				string result {row.at (2)};
				a_hit = true;
				return result;
			}
		}
	}

	return string {};
}


static set <string> get_friends_pleroma (string host, string user)
{
	Http http;
	string url = string {"https://"} + host + string {"/api/statuses/friends.json?user_id="} + user;
	cerr << url << endl;
	string reply_string = http.perform (url);
	picojson::value reply_value;
	string error = picojson::parse (reply_value, reply_string);
	if (! error.empty ()) {
		cerr << error << endl;
		throw (UserException {__LINE__});
	}
	if (! reply_value.is <picojson::array> ()) {
		throw (UserException {__LINE__});
	}
	auto friends_array = reply_value.get <picojson::array> ();
	set <string> friends_string;
	for (auto friend_value: friends_array) {
		if (friend_value.is <picojson::object> ()) {
			auto friend_object = friend_value.get <picojson::object> ();
			auto screen_name_value = friend_object.at (string {"screen_name"});
			if (screen_name_value.is <string> ()) {
				string screen_name_string = screen_name_value.get <string> ();
				friends_string.insert (screen_name_string);
			}
		}
	}
	return friends_string;
}


static void parse_literal (string a_code, unsigned int a_offset, unsigned int & a_eaten, bool & a_ok, string a_literal)
{
	if (a_code.size () < a_offset + a_literal.size ()) {
		a_eaten = 0;
		a_ok = false;
		return;
	}
	for (unsigned int cn = 0; cn < a_literal.size (); cn ++) {
		if (a_code.at (a_offset + cn) != a_literal.at (cn)) {
			a_eaten = 0;
			a_ok = false;
			return;
		}
	}
	a_eaten = a_literal.size ();
	a_ok = true;
}

static bool character_for_name (char c)
{
	return
		('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| ('0' <= c && c <= '9')
		|| c == '.'
		|| c == '-'
		|| c == '_';
}


static string parse_name (string a_code, unsigned int a_offset, unsigned int & a_eaten, bool & a_ok)
{
	string name;
	unsigned int cn;
	for (cn = 0; cn + a_offset < a_code.size (); cn ++) {
		char c = a_code.at (cn + a_offset);
		if (! character_for_name (c)) {
			break;
		}
		name.push_back (c);
	}
	a_eaten = cn;
	a_ok = true;
	return name;
}


static void parse_friend_impl
	(string a_code, unsigned int a_offset, unsigned int & a_eaten, bool & a_ok,
	string & a_host, string & a_user)
{
	unsigned int accumulator = 0;
	{
		unsigned int eaten;
		bool ok;
		parse_literal (a_code, a_offset + accumulator, eaten, ok, string {"https://"});
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	{
		unsigned int eaten;
		bool ok;
		a_host = parse_name (a_code, a_offset + accumulator, eaten, ok);
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	{
		unsigned int eaten;
		bool ok;
		parse_literal (a_code, a_offset + accumulator, eaten, ok, string {"/"});
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	{
		unsigned int eaten;
		bool ok;
		parse_name (a_code, a_offset + accumulator, eaten, ok); /* user or account */
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	{
		unsigned int eaten;
		bool ok;
		parse_literal (a_code, a_offset + accumulator, eaten, ok, string {"/"});
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	{
		unsigned int eaten;
		bool ok;
		a_user = parse_name (a_code, a_offset + accumulator, eaten, ok);
		if (! ok) {
			a_ok = false;
			return;
		}
		accumulator += eaten;
	}
	a_eaten = accumulator;
	a_ok = true;
}


static void parse_friend (string a_code, bool & a_ok, string & a_host, string & a_user)
{
	unsigned int offset = 0;
	unsigned int eaten = 0;
	bool ok = false;
	string host;
	string user;
	parse_friend_impl (a_code, offset, eaten, ok, host, user);
	if (ok && eaten == a_code.size ()) {
		a_ok = true;
		a_host = host;
		a_user = user;
	}
}


static set <string> get_friends_mastodon (string host, string user)
{
	Http http;
	set <string> friends;
	for (unsigned int page = 1; page < 1000; page ++) {
		stringstream url;
		url << string {"https://"} << host << string {"/users/"} << user << string {"/following.json?page="} << page;
		cerr << url.str () << endl;
		string reply_string = http.perform (url.str ());
		picojson::value reply_value;
		string error = picojson::parse (reply_value, reply_string);
		if (! error.empty ()) {
			cerr << error << endl;
			throw (UserException {__LINE__});
		}
		if (! reply_value.is <picojson::object> ()) {
			throw (UserException {__LINE__});
		}
		auto reply_object = reply_value.get <picojson::object> ();
		if (reply_object.find (string {"orderedItems"}) == reply_object.end ()) {
			throw (UserException {__LINE__});
		}
		auto orderd_items_value = reply_object.at (string {"orderedItems"});
		if (! orderd_items_value.is <picojson::array> ()) {
			throw (UserException {__LINE__});
		}
		auto orderd_items_array = orderd_items_value.get <picojson::array> ();
		for (auto item_value: orderd_items_array) {
			if (item_value.is <string> ()) {
				string item_string = item_value.get <string> ();
				string friend_host;
				string friend_user;
				bool ok = false;
				parse_friend (item_string, ok, friend_host, friend_user);
				if (ok) {
					friends.insert (friend_user + string {"@"} + friend_host);
				}
			}
		}
		if (reply_object.find (string {"next"}) == reply_object.end ()) {
			break;
		}
	}
	return friends;
}


set <string> get_friends (string host, string user)
{
	set <string> friends;
	try {
		friends = get_friends_pleroma (host, user);
		return friends;
	} catch (ExceptionWithLineNumber e) {
		cerr << "get_friends_pleroma failed: " << e.line << endl;
	}
	try {
		friends = get_friends_mastodon (host, user);
	} catch (ExceptionWithLineNumber e) {
		cerr << "get_friends_mastodon failed: " << e.line << endl;
	}
	return friends;
}


bool following (string a_host, string a_user, set <string> a_friends)
{
	return a_friends.find (a_user + string {"@"} + a_host) != a_friends.end ()
	|| a_friends.find (a_user) != a_friends.end ();
}


set <User> get_optouted_users ()
{
	map <User, Profile> users_to_profile = read_profiles ();
	
	set <User> optouted_users;
	for (auto user_to_profile: users_to_profile) {
		User user = user_to_profile.first;
		Profile profile = user_to_profile.second;
		string bio = profile.bio;
		bool optouted = (bio.find ("㊙️") != bio.npos);
		if (optouted) {
			optouted_users.insert (user);
		}
	}
	
	return optouted_users;
}


