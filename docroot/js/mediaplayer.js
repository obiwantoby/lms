// @license magnet:?xt=urn:btih:1f739d935676111cfff4b4693e3816e664797050&dn=gpl-3.0.txt GPL-v3-or-Later

var LMS = LMS || {};

LMS.mediaplayer = function () {

	var _root = {};
	var _elems = {};
	var _offset = 0;
	var _duration = 0;
	var _audioSrc;

	var _updateControls = function() {
		if (_elems.audio.paused) {
			_elems.playpause.classList.remove("fa-pause");
			_elems.playpause.classList.add("fa-play");
			_elems.progress.classList.remove("active");
		}
		else {
			_elems.playpause.classList.remove("fa-play");
			_elems.playpause.classList.add("fa-pause");
			_elems.progress.classList.add("active");
		}
	}

	var _durationToString = function (duration) {
			var minutes = parseInt(duration / 60, 10);
			var seconds = parseInt(duration, 10) % 60;

			var res = "";

			res += minutes + ":";
			res += (seconds  < 10 ? "0" + seconds : seconds);

			return res;
	}

	var _playTrack = function() {
		var playPromise = _elems.audio.play();

		if (playPromise !== undefined) {
			playPromise.then(_ => {
				// Automatic playback started
			})
			.catch(error => {
				// Auto-play was prevented
			});
		}
	}

	var _requestPreviousTrack = function() {
		Wt.emit(_root, "playNext");
	}

	var _requestNextTrack = function() {
		Wt.emit(_root, "playNext");
	}

	var init = function(root) {
		_root = root;

		_elems.audio = document.getElementById("lms-mp-audio");
		_elems.playpause = document.getElementById("lms-mp-playpause");
		_elems.previous = document.getElementById("lms-mp-previous");
		_elems.next = document.getElementById("lms-mp-next");
		_elems.progress = document.getElementById("lms-mp-progress");
		_elems.seek = document.getElementById("lms-mp-seek");
		_elems.curtime = document.getElementById("lms-mp-curtime");
		_elems.duration = document.getElementById("lms-mp-duration");
		_elems.volume = document.getElementById("lms-mp-volume");
		_elems.volumeslider = document.getElementById("lms-mp-volume-slider");

		_elems.playpause.addEventListener("click", function() {
			if (_elems.audio.paused) {
				if (_elems.audio.hasAttribute("src"))
					_elems.audio.play();
			}
			else
				_elems.audio.pause();
		});

		_elems.previous.addEventListener("click", function() {
			_requestPreviousTrack();
		});
		_elems.next.addEventListener("click", function() {
			_requestNextTrack();
		});
		_elems.seek.addEventListener("change", function() {
			if (!_elems.audio.hasAttribute("src"))
				return;

			_offset = parseInt(_elems.seek.value, 10);
			_elems.audio.src = _audioSrc + "&offset=" + _offset;
			_elems.audio.load();
			_playTrack();

		});

		_elems.audio.addEventListener("play", _updateControls);
		_elems.audio.addEventListener("playing", _updateControls);
		_elems.audio.addEventListener("pause", _updateControls);

		_elems.audio.addEventListener("timeupdate", function() {
			_elems.progress.style.width = "" + ((_offset + _elems.audio.currentTime) / _duration) * 100 + "%";
			_elems.curtime.innerHTML = _durationToString(_offset + _elems.audio.currentTime);
		});

		_elems.audio.addEventListener("ended", function() {
			Wt.emit(_root, "playbackEnded");
		});

		_elems.audio.volume = _elems.volumeslider.value;
		_elems.lastvolume = _elems.volumeslider.value;
		_elems.volumeslider.addEventListener("input", function() {
			_elems.audio.volume = _elems.volumeslider.value;
			_elems.lastvolume = _elems.volumeslider.value;
		});

		_elems.volume.addEventListener("click", function () {
			if (_elems.audio.volume != 0)
			{
				_elems.audio.volume = 0;
				_elems.volumeslider.value = 0;
			}
			else
			{
				_elems.audio.volume = _elems.lastvolume;
				_elems.volumeslider.value = _elems.lastvolume;
			}
		});
		_elems.audio.addEventListener("volumechange", function() {
			if (_elems.audio.volume != 0)
			{
				_elems.volume.classList.remove("fa-volume-off");
				_elems.volume.classList.add("fa-volume-up");
			}
			else
			{
				_elems.volume.classList.remove("fa-volume-up");
				_elems.volume.classList.add("fa-volume-off");
			}
		});

		if ('mediaSession' in navigator) {
			navigator.mediaSession.setActionHandler("previoustrack", function() {
				_requestPreviousTrack();
			});
			navigator.mediaSession.setActionHandler("nexttrack", function() {
				_requestNextTrack();
			});
		}

	}

	var loadTrack = function(params, autoplay) {
		_offset = 0;
		_duration = params.duration;
		_audioSrc = params.resource;

		_elems.seek.max = _duration;
		_elems.audio.src = _audioSrc;

		_elems.curtime.innerHTML = _durationToString(_offset);
		_elems.duration.innerHTML = _durationToString(_duration);

		if (autoplay)
			_playTrack();

		if ('mediaSession' in navigator) {
			navigator.mediaSession.metadata = new MediaMetadata({
				title: params.title,
				artist: params.artist,
				album: params.release,
				artwork: params.artwork,
			});
		}
	}

	var stop = function() {
		_elems.audio.pause();
	}

	return {
		init: init,
		loadTrack: loadTrack,
		stop: stop,
	};
}();

// @license-end
