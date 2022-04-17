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
let origAudioPlayer = document.getElementById("origAudioPlayer");
let beepyAudioPlayer = document.getElementById("beepyAudioPlayer");
let audio = new SampledAudio();

/********************************************************
 *                    TUNE MENUS                        *
 ********************************************************/

function EMVecVec2Array(XEm) {
    let ret = [];
    for (let i = 0; i < XEm.size(); i++) {
        let row = [];
        for (let j = 0; j < XEm.get(i).size(); j++) {
            row.push(XEm.get(i).get(j));
        }
        ret.push(row);
    }
    return ret;
}

function doDSP(samples) {
    let win = 2048;
    let hop = 1024;
    let maxBin = 256;
    let useWindow = true;
    let timeWin = 8;
    let freqWin = 5;


    let sig = new Module.VectorFloat();
    for (let i = 0; i < samples.length; i++) {
        sig.push_back(samples[i]);
    }
    console.log(sig.size());
    let SEm = new Module.VectorVectorFloat();
    Module.jsGetSpectrogram(sig, SEm, win, hop, maxBin, useWindow);
    let S = EMVecVec2Array(SEm);
    console.log(S);

    let MaxesEm = new Module.VectorVectorInt();
    Module.jsGetMaxes(SEm, MaxesEm, timeWin, freqWin);
    let Maxes = EMVecVec2Array(MaxesEm);
    console.log(Maxes);

    
    Module.clearVector(sig);
    Module.clearVectorVector(SEm);
    Module.clearVectorVectorInt(MaxesEm);
}

let exampleTuneMenu = document.getElementById("ExampleTunes");
exampleTuneMenu.addEventListener('change', function(e){
    audio.loadFile(e.target.value).then(function(){
        audio.connectAudioPlayer(origAudioPlayer);
        progressBar.changeToReady("Finished loading audio");
        doDSP(audio.samples);
    });
    progressBar.loadString = "Loading audio";
    progressBar.startLoading();
});
APPLE_LOGO_URL = "Apple_Music_logo.svg";
let appleMusic = new AppleMusic("appleMusicDiv", audio, function() {
    audio.connectAudioPlayer(origAudioPlayer);
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
            audio.connectAudioPlayer(origAudioPlayer);
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
