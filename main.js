/**
 * Code copyright Christopher J. Tralie, 2021
 * Attribution-NonCommercial-ShareAlike 4.0 International
 */

/** The file that puts everything together for running */
(function() {
    var requestAnimationFrame = window.requestAnimationFrame || window.mozRequestAnimationFrame ||
                                window.webkitRequestAnimationFrame || window.msRequestAnimationFrame;
    window.requestAnimationFrame = requestAnimationFrame;
  })();

let progressBar = new ProgressBar();
let audioPlayer = document.getElementById("audioPlayer");
let audio = new SampledAudio();
audio.connectAudioPlayer(audioPlayer);

/********************************************************
 *                    TUNE MENUS                        *
 ********************************************************/

let exampleTuneMenu = document.getElementById("ExampleTunes");
exampleTuneMenu.addEventListener('change', function(e){
    audio.loadFile(e.target.value).then(function(){
        console.log(audio.samples);
        progressBar.changeToReady("Finished loading audio");
    });
    progressBar.loadString = "Loading audio";
    progressBar.startLoading();
});
APPLE_LOGO_URL = "Apple_Music_logo.svg";
let appleMusic = new AppleMusic("appleMusicDiv", audio, function() {
    progressBar.changeToReady("Finished loading audio");
},
function() {
    progressBar.setLoadingFailed("Failed to load audio from Apple Music 😿");
});

let tuneInput = document.getElementById('tuneInput');
tuneInput.addEventListener('change', function(e) {
    let reader = new FileReader();
    reader.onload = function(e) {
        audio.setSamplesAudioBuffer(e.target.result).then(function(){
            progressBar.changeToReady("Finished loading audio");
        });
    }
    reader.readAsArrayBuffer(tuneInput.files[0]);
    progressBar.loadString = "Loading audio";
    progressBar.startLoading();
});


$('.audioTable').hide();
$('.toggle-audio').on('click',function() {					
  $(this).text(function(_,currentText){
    return currentText == "▼ Choose Tune 🎵" ? "▲ Choose Tune 🎵" : "▼ Choose Tune 🎵";
  });
  $('.audioTable').toggle('slow');
});
