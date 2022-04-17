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
let audio = new SampledAudio();
let origAudioPlayer = document.getElementById("origAudioPlayer");
let beepyAudio = new SampledAudio();
let beepyAudioPlayer = document.getElementById("beepyAudioPlayer");

let dsp = new DSP(audio, origAudioPlayer, beepyAudio, beepyAudioPlayer);

/********************************************************
 *                    TUNE MENUS                        *
 ********************************************************/

let exampleTuneMenu = document.getElementById("ExampleTunes");
exampleTuneMenu.addEventListener('change', function(e){
    audio.loadFile(e.target.value).then(function(){
        audio.connectAudioPlayer(origAudioPlayer);
        progressBar.changeToReady("Finished loading audio");
        dsp.computeAudioFeatures();
    });
    progressBar.loadString = "Loading audio";
    progressBar.startLoading();
});
APPLE_LOGO_URL = "Apple_Music_logo.svg";
let appleMusic = new AppleMusic("appleMusicDiv", audio, function() {
    audio.connectAudioPlayer(origAudioPlayer);
    progressBar.changeToReady("Finished loading audio");
    dsp.computeAudioFeatures();
},
function() {
    progressBar.setLoadingFailed("Failed to load audio from Apple Music 😿");
});

let tuneInput = document.getElementById('tuneInput');
tuneInput.addEventListener('change', function(e) {
    let reader = new FileReader();
    reader.onload = function(e) {
        audio.setSamplesAudioBuffer(e.target.result).then(function(){
            audio.connectAudioPlayer(origAudioPlayer);
            progressBar.changeToReady("Finished loading audio");
            dsp.computeAudioFeatures();
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
