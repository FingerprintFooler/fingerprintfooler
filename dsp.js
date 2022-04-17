const READY_STR = "Ready! Press Play on The Original Tune Or on The Beepy Tune!"

class DSP {
    constructor(audio, origAudioPlayer, beepyAudio, beepyAudioPlayer) {
        this.audio = audio;
        this.origAudioPlayer = origAudioPlayer;
        this.beepyAudio = beepyAudio;
        this.beepyAudioPlayer = beepyAudioPlayer;

        this.win = 2048;
        this.hop = 1024;
        this.maxBin = 256;
        this.timeWin = 8;
        this.freqWin = 5;
        this.useWindow = true;
        this.plotSpectrogram = false;
        this.S = null;
        this.maxTimes = null;
        this.maxFreqs = null;

        this.initializeMenus();
    }

    initializeMenus() {
        const that = this;
        this.gui = new dat.GUI();
        const gui = this.gui;
        gui.add(this, "win", 128, 8192).onChange(
            function(v) {
                let val = Math.round(Math.log(v)/Math.log(2));
                val = Math.pow(2, val);
                that.win = val;
            }
        );
        gui.add(this, "hop", 128, 8192).onChange(
            function(v) {
                let val = Math.round(Math.log(v)/Math.log(2));
                val = Math.pow(2, val);
                that.hop = val;
            }
        );
        gui.add(this, "maxBin", 10, 4096).onChange(
            function(v) {
                let maxVal = Math.floor(that.win/2);
                if (v > maxVal) {
                    that.maxBin = maxVal;
                }
            }
        );
        gui.add(this, "timeWin", 1, 40);
        gui.add(this, "freqWin", 1, 40);
        gui.add(this, "plotSpectrogram");
        gui.add(this, "computeAudioFeatures");
        gui.close();
    }

    computeAudioFeatures() {
        this.audio.connectAudioPlayer(this.origAudioPlayer);
        const that = this;
        new Promise((resolve, reject) => {
            const worker = new Worker("dspworker.js");
            let payload = {samples:that.audio.samples, sr:that.audio.sr, win:that.win, hop:that.hop, maxBin:that.maxBin, timeWin:that.timeWin, freqWin:that.freqWin, useWindow:that.useWindow, plotSpectrogram:that.plotSpectrogram};
            worker.postMessage(payload);
            worker.onmessage = function(event) {
                if (event.data.type == "newTask") {
                    progressBar.loadString = event.data.taskString;
                }
                else if (event.data.type == "error") {
                    that.progressBar.setLoadingFailed(event.data.taskString);
                    reject();
                }
                else if (event.data.type == "debug") {
                    console.log("Debug: " + event.data.taskString);
                }
                else if (event.data.type == "end") {
                    that.beepyAudio.setSamples(event.data.y, that.audio.sr);
                    that.beepyAudio.connectAudioPlayer(that.beepyAudioPlayer);
                    if (that.plotSpectrogram) {
                        that.S = event.data.S;
                        that.maxTimes = event.data.maxTimes;
                        that.maxFreqs = event.data.maxFreqs;
                    }
                    resolve();
                }
            }
        }).then(() => {
            progressBar.changeToReady();
            progressBar.changeMessage(READY_STR);
        }).catch(reason => {
            progressBar.setLoadingFailed(reason);
        });
        progressBar.startLoading();
    }
}