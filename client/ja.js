/* Follow recommendation */


function search (detail) {
	var user_and_host = document.getElementById ('user-input').value;
	var match = /^https:\/\/([\w\.\-]+)\/@([\w]+)$/.exec (user_and_host);
	var user_and_host_array = user_and_host.split ('@');
	if (match) {
		var host = match[1];
		var user = match[2];
		search_impl (host, user, detail);
	} else if (1 < user_and_host_array.length &&
		0 < user_and_host_array[0].length &&
		0 < user_and_host_array[1].length)
	{
		var user = user_and_host_array[0];
		var host = user_and_host_array[1];
		search_impl (host, user, detail);
	} else {
		document.getElementById ('placeholder').innerHTML =
			'<strong>ユーザー名とホスト名が入力されていません。</strong>';
	}
}


function search_impl (host, user, detail) {
	save_inputs (host, user);
	var url = '/cgi-bin/vinayaka-user-match-api.cgi?' +
		encodeURIComponent (host) + '+' +
		encodeURIComponent (user);
	var request = new XMLHttpRequest;
	request.open ('GET', url);
	request.onload = function () {
		if (request.readyState === request.DONE) {
			document.getElementById ('search-button').removeAttribute ('disabled');
			document.getElementById ('detail-button').removeAttribute ('disabled');
			if (request.status === 200) {
				var response_text = request.responseText;
				try {
					var users = JSON.parse (response_text);
					if (typeof users === 'string') {
						document.getElementById ('placeholder').innerHTML =
							'<strong>' + escapeHtml (users) + '</strong>';
					} else {
						show_users (users, detail, host, user);
						activate_share_button (users, host, user);
					}
				} catch (e) {
					document.getElementById ('placeholder').innerHTML =
						'<strong>絶望と仲良くなろうよ。</strong>';
				}
			} else {
				document.getElementById ('placeholder').innerHTML =
					'<strong>情報を取得できませんでした。</strong>';
			}
		}
	}
	document.getElementById ('placeholder').innerHTML =
		'<strong>お待ちください。</strong>';
	document.getElementById ('search-button').setAttribute ('disabled', 'disabled');
	document.getElementById ('detail-button').setAttribute ('disabled', 'disabled');
	request.send ();
}


function save_inputs (host, user) {
	window.localStorage.setItem ('host', host);
	window.localStorage.setItem ('user', user);
}


window.addEventListener ('load', function () {
	var host = window.localStorage.getItem ('host');
	var user = window.localStorage.getItem ('user');
	if (host && user) {
		document.getElementById ('user-input').value = user + '@' + host;
	}
}, false); /* window.addEventListener ('load', function () { */



window.addEventListener ('load', function () {
document.getElementById ('search-button').addEventListener ('click', function () {
	document.getElementById ('explanation-detail').setAttribute ('style', 'display: none;');
	search (false);
}, false);

document.getElementById ('user-input').addEventListener ('keydown', function (event) {
	if (event.key === 'Enter') {
		document.getElementById ('explanation-detail').setAttribute ('style', 'display: none;');
		search (false);
	}
}, false);

document.getElementById ('detail-button').addEventListener ('click', function () {
	document.getElementById ('explanation-detail').removeAttribute ('style');
	search (true);
}, false);
}, false); /* window.addEventListener ('load', function () { */


function escapeHtml (text) {
        text = text.replace (/\&/g, '&amp;');
        text = text.replace (/\</g, '&lt;');
        text = text.replace (/\>/g, '&gt;');
        return text;
};


function show_users (users, detail, current_host, current_user) {
var placeholder = document.getElementById ('placeholder');
var html = '';
var cn;
var cn_intersection;
var limit;

if (detail) {
	limit = 1000;
} else {
	limit = 100;
}

for (cn = 0; cn < users.length && cn < limit; cn ++) {
	var user;
	user = users [cn];
	if (detail ||
		(! (user.host === current_host && user.user === current_user)))
	{
		var user_html =
			'<p>' +
			'<a href="' +
			'https://' + user.host + '/users/' + user.user +
			'" target="vinayaka-external-user-profile">' +
			'<img class="avatar" src="';
		if (user.avatar && 0 < user.avatar.length) {
			user_html += user.avatar;
		} else {
			user_html += 'missing.png';
		}
		user_html +=
			'">' +
			'</a>' +
			'<br>' +
			'<a href="' +
			'https://' + user.host + '/users/' + user.user +
			'" target="vinayaka-external-user-profile">' +
			user.user + '@<wbr>' + user.host +
			'</a>' +
			'<br>' +
			'類似度 ' + user.similarity.toFixed (0);
		if (detail) {
			user_html += '</p><p>';
			user_html += '<small>';
			for (cn_intersection = 0; cn_intersection < user.intersection.length; cn_intersection ++) {
				user_html += escapeHtml (user.intersection[cn_intersection]);
				user_html += " ";
			}
			user_html += '</small>';
		}
		user_html += '</p>';
		html += user_html;
	}
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.addEventListener ('load', function () {
document.getElementById ('user-input').addEventListener ('input', function () {
	var user_and_host = document.getElementById ('user-input').value;
	var user_and_host_array = user_and_host.split ('@');
	if (user_and_host_array.length == 2 && user_and_host_array[1].length == 0)
	{
		var user = user_and_host_array [0];
		var options =
			'<option value="' + user + '@' + 'mstdn.jp' +'"></option>' +
			'<option value="' + user + '@' + 'pixiv.net' +'"></option>' +
			'<option value="' + user + '@' + 'friends.nico' +'"></option>';
		document.getElementById ('completes').innerHTML = options;
	}
}, false); /* document.getElementById ('user-input').addEventListener ('input', function () { */
}, false); /* window.addEventListener ('load', function () { */


window.addEventListener ('load', function () {
	var match = /^\?([\w\.\-]+)\+(\w+)$/.exec (window.location.search);
	if (match) {
		var host = match [1];
		var user = match [2];
		document.getElementById ('user-input').value = user + '@' + host;
		search_impl (host, user, false);
	}
}, false); /* window.addEventListener ('load', function () { */


var g_share_intent = '';


function activate_share_button (users, current_host, current_user) {
	var intent = '';
	intent += '@' + current_user + '@' + current_host + " " +
		'に似ているユーザー' + "\n\n";
	for (var cn = 0; cn < 6 && cn < users.length; cn ++) {
		var user = users[cn];
		if (! (user.host === current_host && user.user === current_user)) {
			intent += '@' + user.user + '@' + user.host + "\n";
		}
	}
	intent += "\n";
	intent += 'マストドンユーザーマッチング' + "\n";
	intent += 'http://vinayaka.distsn.org' + " ";
	intent += '#vinayaka' + "\n";
	g_share_intent = intent;
	document.getElementById ('share-button').removeAttribute ('style');
};


window.addEventListener ('load', function () {
document.getElementById ('share-button').addEventListener ('click', function () {
	var intent = g_share_intent;
	var host = window.localStorage.getItem ('host');
	var url;
	if (host && 0 < host.length) {
		url = 'http://' + host + '/share?text=' + encodeURIComponent (intent);
	} else {
		url = 'https://masha.re/#' + encodeURIComponent (intent);
	}
	window.open (url);
}, false); /* document.getElementById ('share-button').addEventListener ('click', function () { */
}, false); /* window.addEventListener ('load', function () { */

