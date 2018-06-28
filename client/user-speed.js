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
		'm-donation': '寄付',
		'm-powerful-users': 'マストドン/Pleromaのヤベーやつら',
		'm-search': '検索',
		'm-new': '新規',
		'm-active': '流速',
		'm-optout': 'オプトアウト',
		'a-full': 'すべて',
		'm-tpd': 'TPD = トゥート/日',
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
	request.open ('GET', '/cgi-bin/vinayaka-user-speed-api.cgi?100');
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


window.load_1000 = function () {
	document.getElementById ('a-1000').removeAttribute ('href');
	var request = new XMLHttpRequest;
	request.open ('GET', '/cgi-bin/vinayaka-user-speed-api.cgi?1000');
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
};


window.load_full = function () {
	document.getElementById ('a-1000').removeAttribute ('href');
	document.getElementById ('a-full').removeAttribute ('href');
	var request = new XMLHttpRequest;
	request.open ('GET', '/cgi-bin/vinayaka-user-speed-api.cgi');
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
};


function escapeHtml (text) {
        text = text.replace (/\&/g, '&amp;');
        text = text.replace (/\</g, '&lt;');
        text = text.replace (/\>/g, '&gt;');
        return text;
};


function show_users (users) {
var m_bot = 'Bot'
var boilerplate = (window.location.search === '?boilerplate');
var placeholder = document.getElementById ('placeholder');
var html = '';
var cn;
for (cn = 0; cn < users.length; cn ++) {
	var user;
	user = users [cn];
	var score_s = (user.manual_score_available? user.manual_score.toFixed (1): '<b style="color: red">?</b>');
	var user_html = '';
	if (user.blacklisted) {
		user_html =
			'<p>' +
			'<img class="avatar" src="blacklisted.png">' +
			'<span class="headline">' + escapeHtml (user.username) + '@<wbr>' + escapeHtml (user.host) + ' ' +
			'<a class="icon" href="javascript:openBlacklistExplanation()">?</a>' +
			'</span>' +
			'<br>' +
			(user.speed * 60 * 60 * 24).toFixed (1) + ' TPD' + ' ' +
			'(' + (cn + 1).toFixed (0) + ')' +
			(user.type === 'Service'? '<br><strong>' + m_bot + '</strong>': '') +
			'</p>';
	} else {
		user_html +=
			'<p>' +
			'<a href="' +
			'https://' + encodeURIComponent (user.host) + '/users/' + encodeURIComponent (user.username) +
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
			'https://' + encodeURIComponent (user.host) + '/users/' + encodeURIComponent (user.username) +
			'" target="vinayaka-external-user-profile" class="headline">' +
			escapeHtml (user.username) + '@<wbr>' + escapeHtml (user.host) +
			'</a>' +
			'<br>' +
			(user.speed * 60 * 60 * 24).toFixed (1) + ' TPD' + ' ' +
			'(' + (cn + 1).toFixed (0) + ')' +
			(user.type === 'Service'? '<br><strong>' + m_bot + '</strong>': '') +
			'</p>';
	}
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.openBlacklistExplanation = function () {
	alert ('Suspicion of bot, spam or harassment.');
};


