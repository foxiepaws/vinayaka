/* Follow recommendation */


window.addEventListener ('load', function () {
document.getElementById ('search-button').addEventListener ('click', function () {
	var user_and_host = document.getElementById ('user-input').value;
	var user_and_host_array = user_and_host.split ('@');
	if (1 < user_and_host_array.length &&
		0 < user_and_host_array[0].length &&
		0 < user_and_host_array[1].length)
	{
		var user = user_and_host_array[0];
		var host = user_and_host_array[1];
		var url = '/cgi-bin/vinayaka-user-match-api.cgi?' +
			encodeURIComponent (host) + '+' +
			encodeURIComponent (user);
		var request = new XMLHttpRequest;
		request.open ('GET', url);
		request.onload = function () {
			if (request.readyState === request.DONE) {
				if (request.status === 200) {
					var response_text = request.responseText;
					var users = JSON.parse (response_text);
					show_users (users);
				} else {
					document.getElementById ('placeholder').innerHTML =
						'<string>情報を取得できませんでした。</strong>';
				}
			}
		}
		document.getElementById ('placeholder').innerHTML =
			'<string>お待ちください。</strong>';
		request.send ();
	} else {
		document.getElementById ('placeholder').innerHTML =
			'<string>ユーザー名とホスト名が入力されていません。</strong>';
	}
}, false); /* document.getElementById ('search-button').addEventListener ('click', function () { */
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
	var user_html =
		'<p>' +
		'<a href="' +
		'https://' + user.host + '/users/' + user.user +
		'" target="vinayaka-external-user-profile">' +
		user.user + '@<wbr>' + user.host +
		'</a>' +
		'<br>' +
		'類似度 ' + (user.similarity * 100).toFixed (2) + ' %' +
		'</p>';
	html += user_html;
}
placeholder.innerHTML = html;
} /* function show_users (users) { */


