
function mediaProxy (image) {
	return 'https://images.weserv.nl/?url=' +
		encodeURIComponent (image.replace (/^http(s)?\:\/\//, '')) +
		'&errorredirect=' + encodeURIComponent ('vinayaka.distsn.org/missing.png')
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


function escapeAttribute (text) {
        text = text.replace (/\"/g, '&quot;')
        return text
}


function show_users (users) {
var m_bot = 'Bot'
var placeholder = document.getElementById ('placeholder');
var html = '';
var cn;
for (cn = 0; cn < users.length; cn ++) {
	var user;
	user = users [cn];
	var user_html = '';
	if (! user.blacklisted) {
		user_html +=
			'<p>' +
			'<a href="' +
			escapeAttribute (user.url) +
			'" target="vinayaka-external-user-profile">' +
			'<img class="avatar" src="';
		if (user.avatar && 0 < user.avatar.length) {
			user_html += mediaProxy (user.avatar);
		} else {
			user_html += 'missing.png';
		}
		user_html +=
			'">' +
			'</a>' +
			'<a href="' +
			escapeAttribute (user.url) +
			'" target="vinayaka-external-user-profile" class="headline">' +
			escapeHtml (user.user) + '@<wbr>' + escapeHtml (user.host) +
			'</a>' +
			'<br>' +
			'<small>' + (new Date (1000 * user.first_toot_timestamp)) + '</small>' +
			(user.type === 'Service'? '<br><strong>' + m_bot + '</strong>': '') +
			'<br>' +
			escapeHtml (user.screen_name) +
			'</p>'
	}
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.openBlacklistExplanation = function () {
	alert ('Suspicion of bot, spam or harassment.');
};


