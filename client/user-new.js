var g_language


window.addEventListener ('load', function () {
	if (window.navigator.language === 'ja' || window.navigator.language.startsWith ('ja-')) {
		g_language = 'ja'
		setJapaneseMessages ();
	} else {
		g_language = 'en'
	}
}, false)


function setJapaneseMessages () {
	var messages = {
		'm-matching': 'マッチング',
		'm-users': 'ユーザー',
		'm-instances': 'インスタンス',
		'm-mystery': 'ミステリーポスト',
		'm-code': 'コード',
		'm-newcomers': 'マストドン/Pleromaの新しいユーザー',
		'm-new': '新規',
		'm-active': '流速',
		'anti-harassment-message': 'ボット、スパム、ハラスメントを通報するには <a href="https://github.com/distsn/vinayaka/blob/master/server/blacklisted_users.csv" target="_blank">blacklisted_users.csv</a> にプルリクエストを送ってください。'
	}
	for (var id in messages) {
		var placeholder = document.getElementById (id);
		if (placeholder) {
			placeholder.innerHTML = messages[id]
		}
	}
}


window.addEventListener ('load', function () {
	var request = new XMLHttpRequest;
	request.open ('GET', '/cgi-bin/vinayaka-user-new-api.cgi');
	request.onload = function () {
		if (request.readyState === request.DONE) {
			if (request.status === 200) {
				var response_text = request.responseText;
				var users = JSON.parse (response_text);
				show_users (users);
				document.getElementById ('anti-harassment-message').removeAttribute ('style');
			}
		}
	}
	request.send ();
}, false); /* window.addEventListener ('load', function () { */


function escapeHtml (text) {
        text = text.replace (/\&/g, '&amp;');
        text = text.replace (/\</g, '&lt;');
        text = text.replace (/\>/g, '&gt;');
        return text;
};


function show_users (users) {
var placeholder = document.getElementById ('placeholder');
var html = '';
var cn;
for (cn = 0; cn < users.length; cn ++) {
	var user;
	user = users [cn];
	var user_html = '';
	if (user.blacklisted) {
		user_html =
			'<p>' +
			'<img class="avatar" src="blacklisted.png">' +
			'<span class="headline">' + escapeHtml (user.user) + '@<wbr>' + escapeHtml (user.host) + ' ' +
			'<a class="icon" href="javascript:openBlacklistExplanation()">?</a>' +
			'</span>' +
			'<br>' +
			'<small>' + (new Date (1000 * user.first_toot_timestamp)) + '</small>' +
			'</p>';
	} else {
		user_html +=
			'<p>' +
			'<a href="' +
			'https://' + encodeURIComponent (user.host) + '/users/' + encodeURIComponent (user.user) +
			'" target="vinayaka-external-user-profile">' +
			'<img class="avatar" src="';
		if (user.avatar && 0 < user.avatar.length) {
			user_html += encodeURI (user.avatar);
		} else {
			user_html += 'missing.png';
		}
		user_html +=
			'">' +
			'</a>' +
			'<a href="' +
			'https://' + encodeURIComponent (user.host) + '/users/' + encodeURIComponent (user.user) +
			'" target="vinayaka-external-user-profile" class="headline">' +
			escapeHtml (user.user) + '@<wbr>' + escapeHtml (user.host) +
			'</a>' +
			'<br>' +
			'<a href="' + encodeURI (user.first_toot_url) + '" target="distsn-preview">' +
			'<small>' + (new Date (1000 * user.first_toot_timestamp)) + '</small>' +
			'</a>' +
			'</p>';
	}
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.openBlacklistExplanation = function () {
	alert ('Suspicion of bot, spam or harassment.');
};

