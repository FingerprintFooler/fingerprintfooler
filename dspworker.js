importScripts("dspem.js");

onmessage = function(event) {
    let samples = event.data.samples;
    let win = event.data.win;
    let hop = event.data.hop;
    let sr = event.data.sr;
    let maxBin = event.data.maxBin;
    let timeWin = event.data.timeWin;
    let freqWin = event.data.freqWin;
    let useWindow = event.data.useWindow;
    let plotSpectrogram = event.data.plotSpectrogram;

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
    
    function EMVec2Array(XEm) {
        let ret = [];
        for (let i = 0; i < XEm.size(); i++) {
            ret.push(XEm.get(i));
        }
        return ret;
    }

    // Step 1: Clear the vectors that will hold the output
    Module.onRuntimeInitialized = function() {
        // Step 1: Initialize vectors
        postMessage({"type":"newTask", "taskString":"Initializing STL vectors"});
        let ret = {};
        let sig = new Module.VectorFloat();
        for (let i = 0; i < samples.length; i++) {
            sig.push_back(samples[i]);
        }
        let SEm = new Module.VectorVectorFloat();
        let yEm = new Module.VectorFloat();
        
        // Step 2: Compute spectrogram
        postMessage({"type":"newTask", "taskString":"Computing spectrogram"});
        Module.jsGetSpectrogram(sig, SEm, win, hop, maxBin, useWindow);
        if (plotSpectrogram) {
            ret.S = EMVecVec2Array(SEm);
        }
        
        // Step 3: Return maxes if requested
        if (plotSpectrogram) {
            postMessage({"type":"newTask", "taskString":"Extracting maxes"});
            let maxTimesEm = new Module.VectorInt();
            let maxFreqsEm = new Module.VectorInt();
            Module.jsGetMaxes(SEm, maxTimesEm, maxFreqsEm, timeWin, freqWin);
            ret.maxTimes = EMVec2Array(maxTimesEm);
            ret.maxFreqs = EMVec2Array(maxFreqsEm);
            Module.clearVectorInt(maxTimesEm);
            Module.clearVectorInt(maxFreqsEm);
        }

        // Step 4: Extract beepy tune
        postMessage({"type":"newTask", "taskString":"Computing beepy tunes"});
        Module.jsGetBeepyTune(SEm, yEm, samples.length, timeWin, freqWin, win, hop, sr);
        ret.y = EMVec2Array(yEm);


        // Step 5: Free memory
        postMessage({"type":"newTask", "taskString":"Freeing memory"});
        Module.clearVector(yEm);
        Module.clearVector(sig);
        Module.clearVectorVector(SEm);

        // Return results
        ret.type = "end";
        postMessage(ret);
    }
    
}
