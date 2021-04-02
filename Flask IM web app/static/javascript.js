document.addEventListener('DOMContentLoaded', function() {
  const session = document.querySelector('#chatName').innerHTML;
  console.log(session);
  var socket = io.connect(location.protocol + '//' + document.domain + ':' + location.port);
  if (!localStorage.getItem('displayname')) {
    const request = new XMLHttpRequest();
    request.open('GET', '/');
  }
  else {
    const body = document.querySelector('body');
    //body.style.animationPlayState = 'paused';
    let displayname = localStorage.getItem('displayname');
    document.querySelector('#displayname').innerHTML = displayname;
    socket.on('connect', () => {
      document.querySelector('#newMessageButton').onclick = () => {
        let newMessageText = document.querySelector('#newMessageField').value;
        let happyBirthday = false;
        if (newMessageText.length > 0) {
          if (newMessageText.includes('Happy B')) {
            happyBirthday = true;
          }
          socket.emit('new message', newMessageText, happyBirthday);
          document.querySelector('#newMessageField').value = '';
        }
      }
    })
    socket.on('message added', data => {
      if (data.session == session) {
        if (data.happyBirthday == true) {
          //Sets the background image to our Happy Birthday Picture and at first is barely visible.
          document.body.style.backgroundImage = "url('/static/62211797-happy-birthday-background-with-set-of-colorful-balloons-multicolored-pennants-and-confetti-on-white-.jpg')";
          document.body.style.backgroundBlendMode = "lighten";
          let opacity = .9;
          document.body.style.backgroundColor = "rgba(255,255,255," + opacity + ")";
          birthdayFadeIn(opacity);
        }
        const tr = document.createElement('tr');
        tr.innerHTML = "<td>" + data.displayname + "</td>" + "<td>" + data.timestamp + "</td>" + "<td>" + data.addedMessageText + "</td>";
        document.querySelector('#messages').append(tr);
      }
    })

  }
})

//Recursive call when Happy Birthday message is typed. Allows Background image to fade in and then stay for 3 seconds.
function birthdayFadeIn(opacity) {
  if (opacity > 0) {
    document.body.style.backgroundColor = "rgba(255,255,255," + opacity + ")";
    opacity = opacity - .1;
    setTimeout(function() {
      birthdayFadeIn(opacity);
    }, 100);

  } else {
    setTimeout(function() {
      birthdayFadeOut(opacity);
    }, 3000);
  }
}

//Recursive call for the Happy Birthday Image to slowly fade out
function birthdayFadeOut(opacity) {
  if (opacity < 1) {
    document.body.style.backgroundColor = "rgba(255,255,255," + opacity + ")";
    opacity = opacity + .1;

    //setTimeout(birthdayFadeOut {birthdayFadeOut(opacity); }, 100);
    setTimeout(function() {
      birthdayFadeOut(opacity);
    }, 100);

  } else {
    document.body.style.backgroundImage = "none";
  }

}
