// quick_example.cpp
#include <emscripten/bind.h>

using namespace emscripten;

#include <complex>
#include <vector>
#include <math.h>

typedef std::complex<float> cfloat;
#define FFT_FORWARD 0
#define FFT_INVERSE 1
#define PI 3.1415926535897932384626433832795

class DSP {
	private:
		cfloat*** W;//Cache the complex coefficients for the FFT
		float* hannwindow;
		int fftsize;

		/**
		 * Initialize the complex coefficients for the FFT
		 * @param fftsize The length of the fft
		 */
		void initCoeffs(int fftsize);

		/**
		 * Initialize the coefficients in Hann window
		 * @param fftsize The length of the fft
		 */
		void initWindow(int fftsize);

		/**
		 * Perform an in-place Cooley-Tukey FFT
		 * @param toReturn Array that holds FFT coefficients
		 * @param N Length of array (assumed to be power of 2)
		 * @param inverse Whether this is a forward or inverse FFT
		 */
		void performfft(cfloat* toReturn, int N, int inverse);
	
	public:
		cfloat fftres;
		DSP(int fftsize);
		~DSP();

		/**
		 * Implement the dft directly from the definition (used for speed comparison)
		 * @param sig Complex signal on which to compute dft
		 * @param toReturn The array that will hold the fourier coefficients
		 * @param N Length of signal
		 * @return Complex DFT coefficients
		 */
		void dft(cfloat* sig, cfloat* toReturn, int N);

		/**
		 * Perform the FFT on a complex signal
		 * @param sig The signal
		 * @param toReturn The array that will hold the fourier coefficients
		 * @param N Length of the signal (assumed to be power of 2)
		 * @return An N-length array with FFT coefficients
		 */
		void fft(cfloat* sig, cfloat* toReturn, int N);
	
		/**
		 * Perform the inverse FFT on an array of complex FFT coefficients
		 * @param sig The FFT coefficients
		 * @param toReturn The array that will hold the complex time series
		 * @param N Length of the FFT coefficients (assumed to be power of 2)
		 * @return An N-length array with FFT coefficients
		 */
		void ifft(cfloat* sig, cfloat* toReturn, int N);
		
		/**
		 * Helper function to create a complex array out of an array of 
		 * real amplitude samples
		 * @param data An array of floats for the audio data
		 * @param res Array holding the result
		 * @param N Total number of samples in data
		 * @param start Index to start in the array
		 * @param win Length of the window
		 * @param useWindow Whether to use the window
		 */
		void toWindowedComplexArray(float* data, cfloat* res, int N, int start, int win, bool useWindow);

		/**
		 * Perform a float-time fourier transform on a bunch of samples
		 * @param sig Samples in the signal
		 * @param N Length of signal
		 * @param win Window length
		 * @param hop Hop length
		 * @param useWindow Whether to use the window
		 * @param NWin Number of windows (returned by reference)
		 * @return An NWin x win 2D array of complex floats
		 */
		cfloat** stft(float* sig, int N, int win, int hop, bool useWindow, int* NWin);

		/**
		 * Perform a magnitude float-time fourier transform on a bunch of samples
		 * @param sig Samples in the signal
		 * @param N Length of signal
		 * @param win Window length
		 * @param hop Hop length
		 * @param useWindow Whether to use the window
		 * @param NWin Number of windows (returned by reference)
		 * @return A win x NWin 2D array of complex floats
		 */
		float** spectrogram(float* sig, int N, int win, int hop, int maxBin, bool useWindow, int* NWin);
};

/**
 * Free the memory associated to an STFT
 * @param S STFT
 * @param NWin Window length
 */
void deleteSTFT(cfloat** S, int NWin);

/**
 * Free the memory associated to a spectrogram
 * @param S Spectrogram
 * @param win Window length
 */
void deleteSpectrogram(float** S, int win);


/**
 * Compute the closest power of 2 greater than or equal
 * to some number
 * @param a The number
 * @return The closest power of 2 >= a
 */
int getClosestPowerOf2(int a) {
	float lg = log((float)a) / log(2.0);
	int power = (int)lg;
	if ((float)((int)lg) < lg) {
		power++;
	}
	return power;
}

/**
 * Compute a version of x which is bit-reversed
 * @param x A 32-bit int to reverse
 * @param length Length of bits
 * @return Bit-reversed version of x
 */
int bitReverse(int x, int length) {
	int toReturn = 0;
	int mirror = length / 2;
	for (int mask = 1; mask <= length; mask <<= 1, mirror >>= 1) {
		if ((mask & x) > 0)
			toReturn |= mirror;
	}
	return toReturn;
}


/**
 * Rearrange the terms in-place so that they're sorted by the least
 * significant bit (this is the order in which the terms are accessed
 * in the FFT)
 * @param a An array of complex numbers
 * @param N Number of elements in the array
 */
void rearrange(cfloat* a, int N) {
	for (int i = 0; i < N; i++) {
		int j = bitReverse(i, N);
		if (j > i) { //Don't waste effort swapping two mirrored
		//elements that have already been swapped
			cfloat temp = a[j];
			a[j] = a[i];
			a[i] = temp;
		}
	}
}

/**
 * Initialize the complex coefficients for the FFT
 * @param fftsize The length of the fft
 */
void DSP::initCoeffs(int fftsize) {
	int maxlevel = getClosestPowerOf2(fftsize) + 1;
	W = new cfloat**[maxlevel+1];
	for (int level = 1; level <= maxlevel; level++) {
		int FFTSize = 1 << level;
		W[level] = new cfloat*[2];
		W[level][0] = new cfloat[FFTSize >> 1];
		W[level][1] = new cfloat[FFTSize >> 1];
		for (int i = 0; i < FFTSize >> 1; i++) {
			float iangle = (float)i * 2.0 * PI / (float)FFTSize;
			float fangle = (-1.0) * iangle;
			W[level][FFT_FORWARD][i] = cfloat(cos(fangle), sin(fangle));
			W[level][FFT_INVERSE][i] = cfloat(cos(iangle), sin(iangle)); 
		}
	}
}

/**
 * Initialize the coefficients in Hann window
 * @param fftsize Lenght of fft
 */
void DSP::initWindow(int fftsize) {
	int N = fftsize;
	hannwindow = new float[N];
	for (int n = 0; n < N; n++) {
		float angle = 2.0*PI * n / (float)(N - 1);
		//Do a hann window for now
		hannwindow[n] = 0.54 - 0.46*cos(angle);
	}
}

/**
 * Perform an in-place Cooley-Tukey FFT
 * @param toReturn Array that holds FFT coefficients
 * @param N Length of array (assumed to be power of 2)
 * @param inverse Whether this is a forward or inverse FFT
 */
void DSP::performfft(cfloat* toReturn, int N, int inverse) {
	rearrange(toReturn, N);
	//Do the trivial FFT size of 2 first
	for (int i = 0; i < N; i += 2) {
		cfloat temp = toReturn[i];
		toReturn[i] = temp + toReturn[i + 1];
		toReturn[i + 1] = temp - toReturn[i + 1];
	}
	int Mindex = 2;//Index used to access the cached complex
	//coefficients
	for (int level = 2; level < N; level <<= 1) {
		int FFTSize = level << 1;
		for (int start = 0; start < N; start += FFTSize) {
			//This is a little chunk of an FFT of size "FFTSize"
			//to do in-place with the merging algorithm
			//NOTE: "level" gives the length between mirrored terms
			for (int i = 0; i < level; i++) {
				cfloat coeff = W[Mindex][inverse][i];
				cfloat first = toReturn[start + i];
				cfloat second = coeff*toReturn[start + i + level];
				toReturn[start + i] = first + second;
				toReturn[start + i + level] = first - second;
			}
		}
		Mindex++;
	}
}
	

DSP::DSP(int fftsize) {
	this->fftsize = fftsize;
	this->initCoeffs(fftsize);
	this->initWindow(fftsize);
}
DSP::~DSP() {
	int maxlevel = getClosestPowerOf2(fftsize) + 1;
	// Clean up FFT coefficients
	for (int level = 1; level <= maxlevel; level++) {
		for (int type = 0; type < 2; type++) {
			delete[] W[level][type];
		}
		delete[] W[level];
	}
	delete[] W;
	// Clean up window coefficients
	delete[] hannwindow;
}

/**
 * Implement the dft directly from the definition (used for speed comparison)
 * @param sig Complex signal on which to compute dft
 * @param toReturn The array that will hold the fourier coefficients
 * @param N Length of signal
 * @return Complex DFT coefficients
 */
void DSP::dft(cfloat* sig, cfloat* toReturn, int N) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			float angle = -2.0 * PI * (float)i * (float)j / (float)N;
			cfloat coeff(cos(angle), sin(angle));
			toReturn[i] = coeff*sig[i];
		}
	}
}

/**
 * Perform the FFT on a complex signal
 * @param sig The signal
 * @param toReturn The array that will hold the fourier coefficients
 * @param N Length of the signal (assumed to be power of 2)
 * @return An N-length array with FFT coefficients
 */
void DSP::fft(cfloat* sig, cfloat* toReturn, int N) {
	for (int i = 0; i < N; i++) {
		toReturn[i] = sig[i];
	}
	performfft(toReturn, N, FFT_FORWARD);	
}

/**
 * Perform the inverse FFT on an array of complex FFT coefficients
 * @param sig The FFT coefficients
 * @param toReturn The array that will hold the complex time series
 * @param N Length of the FFT coefficients (assumed to be power of 2)
 * @return An N-length array with FFT coefficients
 */
void DSP::ifft(cfloat* sig, cfloat* toReturn, int N) {
	for (int i = 0; i < N; i++) {
		toReturn[i] = sig[i];
		//Scale by 1/N for inverse FFT
		toReturn[i] *= cfloat(1.0/(float)N, 0);
	}
	performfft(toReturn, N, FFT_INVERSE);
}

/**
 * Helper function to create a complex array out of an array of 
 * real amplitude samples
 * @param data An array of floats for the audio data
 * @param res Array holding the result
 * @param N Total number of samples in data
 * @param start Index to start in the array
 * @param win Length of the window
 * @param useWindow Whether to use the window
 */
void DSP::toWindowedComplexArray(float* data, cfloat* res, int N, int start, int win, bool useWindow) {
	//Make a complex array out of the real array
	for (int i = 0; i < win; i++) {
		if (start+i < N) {
			res[i] = cfloat(data[start + i], 0.0);
			if (useWindow) {
				res[i] *= hannwindow[i];
			}
		}
		else {
			//Zero pad if not a power of 2
			res[i] = cfloat(0.0, 0.0);
		}
	}
}

/**
 * Perform a short-time fourier transform on a bunch of samples
 * @param sig Samples in the signal
 * @param N Length of signal
 * @param win Window length (Assumed to be a power of 2)
 * @param hop Hop length
 * @param useWindow Whether to use the window
 * @param NWin Number of windows (returned by reference)
 * @return An NWin x win 2D array of complex floats
 */
cfloat** DSP::stft(float* sig, int N, int win, int hop, bool useWindow, int* NWin) {
	*NWin = 1 + round((N-win)/(float)hop);
	cfloat** S = new cfloat*[*NWin];
	for (int i = 0; i < *NWin; i++) {
		S[i] = new cfloat[win];
	}
	cfloat* ffti = new cfloat[win];
	for (int i = 0; i < *NWin; i++) {
		toWindowedComplexArray(sig, S[i], N, i*hop, win, useWindow);
		fft(S[i], ffti, win);
		for (int j = 0; j < win; j++) {
			S[i][j] = ffti[j];
		}
	}
	delete[] ffti;
	return S;
}

/**
 * Perform a magnitude short-time fourier transform on a bunch of samples
 * @param sig Samples in the signal
 * @param N Length of signal
 * @param win Window length of STFT (Assumed to be a power of 2)
 * @param hop Hop length
 * @param useWindow Whether to use the window
 * @param NWin Number of windows (returned by reference)
 * @return A NWin x maxBin 2D array of floats
 */
float** DSP::spectrogram(float* sig, int N, int win, int hop, int maxBin, bool useWindow, int* NWin) {
	cfloat** SComplex = stft(sig, N, win, hop, useWindow, NWin);
	float** S = new float*[*NWin];
	for (int i = 0; i < *NWin; i++) {
		S[i] = new float[maxBin];
		for (int j = 0; j < maxBin; j++) {
			S[i][j] = abs(SComplex[i][j]);
		}
	}
	deleteSTFT(SComplex, *NWin);
	return S;
}

/**
 * Free the memory associated to an STFT
 * @param S Spectrogram
 * @param win Window length
 */
void deleteSTFT(cfloat** S, int NWin) {
	for (int i = 0; i < NWin; i++) {
		delete[] S[i];
	}
	delete[] S;
}

/**
 * Free the memory associated to a spectrogram
 * @param S Spectrogram
 * @param win Window length
 */
void deleteSpectrogram(float** S, int win) {
	for (int i = 0; i < win; i++) {
		delete[] S[i];
	}
	delete[] S;
}


void clearVectorVector(std::vector<std::vector<float> >& M) {
    for (size_t i = 0; i < M.size(); i++) {
        M[i].clear();
    }
    M.clear();
}

void clearVectorVectorInt(std::vector<std::vector<int> >& M) {
    for (size_t i = 0; i < M.size(); i++) {
        M[i].clear();
    }
    M.clear();
}

void clearVector(std::vector<float>& M) {
	M.clear();
}

void jsGetSpectrogram(std::vector<float> sig, std::vector<std::vector<float>>& SOut, int win, int hop, int maxBin, bool useWindow) {
  DSP dsp(win);
  int NWin;
  int N = (int)sig.size();
  float** S = dsp.spectrogram(&sig[0], N, win, hop, maxBin, useWindow, &NWin);
  for (int i = 0; i < NWin; i++) {
	std::vector<float> window;
    for (int j = 0; j < maxBin; j++) {
      window.push_back(S[i][j]);
    }
	SOut.push_back(window);
  }
  deleteSpectrogram(S, NWin);
}

bool isMax(std::vector<std::vector<float>>& S, int i, int j, int timeWin, int freqWin) {
	for (int di = -timeWin; di <= timeWin; di++) {
		int i2 = i + di;
		if (i2 >= 0 && i2 < (int)S.size()) {
			for (int dj = -freqWin; dj <= freqWin; dj++) {
				int j2 = j + dj;
				if (j2 >= 0 && j2 < (int)S[i].size()) {
					if (S[i2][j2] > S[i][j]) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

void jsGetMaxes(std::vector<std::vector<float>> S, std::vector<std::vector<int>>& maxes, int timeWin, int freqWin) {
	for (size_t i = 0; i < S.size(); i++) {
		for (size_t j = 0; j < S[i].size(); j++) {
			if (isMax(S, i, j, timeWin, freqWin)) {
				std::vector<int> maxLoc(2);
				maxLoc[0] = i;
				maxLoc[1] = j;
				maxes.push_back(maxLoc);
			}
		}
	}
}

EMSCRIPTEN_BINDINGS(stl_wrappers) {
    emscripten::register_vector<float>("VectorFloat");
    emscripten::register_vector<std::vector<float>>("VectorVectorFloat");
	emscripten::register_vector<std::vector<int>>("VectorVectorInt");
}


EMSCRIPTEN_BINDINGS(my_module) {
    function("jsGetSpectrogram", &jsGetSpectrogram);
	function("jsGetMaxes", &jsGetMaxes);
	function("clearVector", &clearVector);
	function("clearVectorVector", &clearVectorVector);
	function("clearVectorVectorInt", &clearVectorVectorInt);
}