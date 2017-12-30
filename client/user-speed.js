/* Follow recommendation */


window.addEventListener ('load', function () {
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
}, false); /* window.addEventListener ('load', function () { */


function escapeHtml (text) {
        text = text.replace (/\&/g, '&amp;');
        text = text.replace (/\</g, '&lt;');
        text = text.replace (/\>/g, '&gt;');
        return text;
};


function show_users (users) {
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
			'<span class="headline">' + user.username + '@<wbr>' + user.host + ' ' +
			'<a class="icon" href="javascript:openBlacklistExplanation()">?</a>' +
			'</span>' +
			'<br>' +
			(user.speed * 60 * 60 * 24).toFixed (1) + ' TPD' + ' ' +
			'(' + (cn + 1).toFixed (0) + ')' +
			'</p>';
	} else {
		user_html +=
			'<p>' +
			'<a href="' +
			'https://' + user.host + '/users/' + user.username +
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
			'<a href="' +
			'https://' + user.host + '/users/' + user.username +
			'" target="vinayaka-external-user-profile" class="headline">' +
			user.username + '@<wbr>' + user.host +
			'</a>' +
			'<br>' +
			(user.speed * 60 * 60 * 24).toFixed (1) + ' TPD' + ' ' +
			'(' + (cn + 1).toFixed (0) + ')' +
			'</p>';
	}
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


window.openBlacklistExplanation = function () {
	alert ('Suspicion of bot, spam or harassment.');
};


