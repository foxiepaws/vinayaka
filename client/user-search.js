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
		'm-mastodon-pleroma-user-search': 'マストドン/Pleromaユーザー検索',
		'm-search': '検索',
		'm-new': '新規',
		'm-active': '流速',
		'm-optout': 'オプトアウト',
		'm-description': 'ユーザー名、スクリーンネーム、プロフィールを検索します。',
		'anti-harassment-message': 'ボット、スパム、ハラスメントを通報するには <a href="https://github.com/distsn/vinayaka/blob/master/server/blacklisted_users.csv" target="_blank">blacklisted_users.csv</a> にプルリクエストを送ってください。'
	}
	for (var id in messages) {
		var placeholder = document.getElementById (id);
		if (placeholder) {
			placeholder.innerHTML = messages[id]
		}
	}
	document.getElementById　('search-button').value = '検索'
}


function search () {
	var keyword = document.getElementById ('keyword-input').value;
	if (0 < keyword.length) {
		search_impl (keyword);
	} else {
		document.getElementById ('placeholder').innerHTML =
			'<strong>' +
			(g_language === 'ja'? '検索キーワードが入力されていません。':
			'Keyword is not available.') +
			'</strong>'
	}
}


function search_impl (keyword) {
	var url = '/cgi-bin/vinayaka-user-search-api.cgi?' +
		encodeURIComponent (keyword);
	var request = new XMLHttpRequest;
	request.open ('GET', url);
	request.onload = function () {
		if (request.readyState === request.DONE) {
			document.getElementById ('search-button').removeAttribute ('disabled');
			if (request.status === 200) {
				var response_text = request.responseText;
				try {
					var users = JSON.parse (response_text);
					show_users (users);
					document.getElementById ('anti-harassment-message').removeAttribute ('style');
				} catch (e) {
					document.getElementById ('placeholder').innerHTML =
						'<strong>' +
						(g_language === 'ja'? '絶望と仲良くなろうよ。':
						'Sorry.') +
						'</strong>'
				}
			} else {
				document.getElementById ('placeholder').innerHTML =
					'<strong>' +
					(g_language === 'ja'? '情報を取得できませんでした。':
					'Sorry.') +
					'</strong>'
			}
		}
	}
	document.getElementById ('anti-harassment-message').setAttribute ('style', 'display:none;');
	document.getElementById ('placeholder').innerHTML =
		'<strong>' +
		(g_language === 'ja'? 'お待ちください。':
		'Searching...') +
		'</strong>'
	document.getElementById ('search-button').setAttribute ('disabled', 'disabled');
	request.send ();
}


window.addEventListener ('load', function () {
document.getElementById ('search-button').addEventListener ('click', function () {
	search ()
}, false)

document.getElementById ('keyword-input').addEventListener ('keydown', function (event) {
	if (event.key === 'Enter') {
		search ();
	}
}, false)
}, false) /* window.addEventListener ('load', function () { */


function escapeHtml (text) {
        text = text.replace (/\&/g, '&amp;');
        text = text.replace (/\</g, '&lt;');
        text = text.replace (/\>/g, '&gt;');
        return text;
}


function show_users (users) {
	if (users.length <= 100) {
		show_users_impl (users)
	} else {
		var message =
			(g_language === 'ja'? '検索結果は ' + users.length + ' 件です。表示してよろしいですか？':
				users.length + ' users found.')
		var reply = window.confirm (message)
		if (reply) {
			show_users_impl (users)
		} else {
			var placeholder = document.getElementById ('placeholder');
			var html =
				'<p>' +
				(g_language === 'ja'? users.length + ' ユーザー':
					users.length + ' users found') +
				'</p>'
			placeholder.innerHTML = html
		}
	}
}


function show_users_impl (users) {
var m_bot = 'Bot'
var placeholder = document.getElementById ('placeholder');
var html = '';

for (cn = 0; cn < users.length; cn ++) {
	var user;
	user = users [cn];
	var user_html = '';
	user_html += '<p>'
	if (user.blacklisted) {
		user_html +=
			'<img class="avatar" src="blacklisted.png">' +
			'<span class="headline">' +
			'<a href="' +
			'https://' + encodeURIComponent (user.host) + '/users/' + encodeURIComponent (user.user) +
			'" target="vinayaka-external-user-profile">' +
			escapeHtml (user.user) + '@<wbr>' + escapeHtml (user.host) +
			'</a>' + ' ' +
			'<a class="icon" href="javascript:openBlacklistExplanation()">?</a>' +
			'</span>'
	} else {
		user_html +=
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
			'</a>'
	}
	user_html +=
		(user.type === 'Service'? '<br><strong>' + m_bot + '</strong>': '') +
		'<br>' + escapeHtml (user.text) +
		'</p>'
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.openBlacklistExplanation = function () {
	alert ('Suspicion of bot, spam or harassment.');
}


