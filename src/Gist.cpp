//=======================================================================
/** @file Gist.cpp
 *  @brief Implementation for all relevant parts of the 'Gist' audio analysis library
 *  @author Adam Stark
 *  @copyright Copyright (C) 2013  Adam Stark
 *
 * This file is part of the 'Gist' audio analysis library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//=======================================================================

#include "Gist.h"

//=======================================================================
template <class T>
Gist<T>::Gist (int frameSize_, int sampleRate_)
 :  fftConfigured (false),
    onsetDetectionFunction (frameSize_),
    yin (sampleRate_),
    mfcc (frameSize_, sampleRate_)
{
    setAudioFrameSize (frameSize_);
}

//=======================================================================
template <class T>
Gist<T>::~Gist()
{
    if (fftConfigured)
    {
        freeFFT();
    }
}

//=======================================================================
template <class T>
void Gist<T>::setAudioFrameSize (int frameSize_)
{
    frameSize = frameSize_;
    
    audioFrame.resize (frameSize);
    fftReal.resize (frameSize);
    fftImag.resize (frameSize);
    magnitudeSpectrum.resize (frameSize / 2);
    
    configureFFT();
    
    onsetDetectionFunction.setFrameSize (frameSize);
    mfcc.setFrameSize (frameSize);
}

//=======================================================================
template <class T>
void Gist<T>::processAudioFrame (std::vector<T> audioFrame_)
{
    audioFrame = audioFrame_;
    
    performFFT();
}

//=======================================================================
template <class T>
void Gist<T>::processAudioFrame (T* buffer, unsigned long numSamples)
{
    audioFrame.assign (buffer, buffer + numSamples);
    
    performFFT();
}

//=======================================================================
template <class T>
std::vector<T> Gist<T>::getMagnitudeSpectrum()
{
    return magnitudeSpectrum;
}

//=======================================================================
template <class T>
T Gist<T>::rootMeanSquare()
{
    return coreTimeDomainFeatures.rootMeanSquare (audioFrame);
}

//=======================================================================
template <class T>
T Gist<T>::peakEnergy()
{
    return coreTimeDomainFeatures.peakEnergy (audioFrame);
}

//=======================================================================
template <class T>
T Gist<T>::zeroCrossingRate()
{
    return coreTimeDomainFeatures.zeroCrossingRate (audioFrame);
}

//=======================================================================
template <class T>
T Gist<T>::spectralCentroid()
{
    return coreFrequencyDomainFeatures.spectralCentroid (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::spectralCrest()
{
    return coreFrequencyDomainFeatures.spectralCrest (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::spectralFlatness()
{
    return coreFrequencyDomainFeatures.spectralFlatness (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::spectralRolloff()
{
    return coreFrequencyDomainFeatures.spectralRolloff (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::spectralKurtosis()
{
    return coreFrequencyDomainFeatures.spectralKurtosis (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::energyDifference()
{
    return onsetDetectionFunction.energyDifference (audioFrame);
}

//=======================================================================
template <class T>
T Gist<T>::spectralDifference()
{
    return onsetDetectionFunction.spectralDifference (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::spectralDifferenceHWR()
{
    return onsetDetectionFunction.spectralDifferenceHWR (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::complexSpectralDifference()
{
    return onsetDetectionFunction.complexSpectralDifference (fftReal, fftImag);
}

//=======================================================================
template <class T>
T Gist<T>::highFrequencyContent()
{
    return onsetDetectionFunction.highFrequencyContent (magnitudeSpectrum);
}

//=======================================================================
template <class T>
T Gist<T>::pitch()
{
    return yin.pitchYin (audioFrame);
}

//=======================================================================
template <class T>
std::vector<T> Gist<T>::melFrequencySpectrum()
{
    return mfcc.melFrequencySpectrum (magnitudeSpectrum);
}

//=======================================================================
template <class T>
std::vector<T> Gist<T>::melFrequencyCepstralCoefficients()
{
    return mfcc.melFrequencyCepstralCoefficients (magnitudeSpectrum);
}

//=======================================================================
template <class T>
void Gist<T>::configureFFT()
{
    if (fftConfigured)
    {
        freeFFT();
    }
    
#ifdef USE_FFTW
    // ------------------------------------------------------
    // initialise the fft time and frequency domain audio frame arrays
    fftIn = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * frameSize);  // complex array to hold fft data
    fftOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * frameSize); // complex array to hold fft data
    
    // FFT plan initialisation
    p = fftw_plan_dft_1d (frameSize, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);
#endif /* END USE_FFTW */
    
#ifdef USE_KISS_FFT
    // ------------------------------------------------------
    // initialise the fft time and frequency domain audio frame arrays
    fftIn = new kiss_fft_cpx[frameSize];
    fftOut = new kiss_fft_cpx[frameSize];
    cfg = kiss_fft_alloc (frameSize, 0, 0, 0);
#endif /* END USE_KISS_FFT */
    
#ifdef USE_ACCELERATE_FFT
    accelerateFFT.setAudioFrameSize (frameSize);
#endif
    
    fftConfigured = true;
}

//=======================================================================
template <class T>
void Gist<T>::freeFFT()
{
#ifdef USE_FFTW
    // destroy fft plan
    fftw_destroy_plan (p);
    
    fftw_free (fftIn);
    fftw_free (fftOut);
#endif
    
#ifdef USE_KISS_FFT
    // free the Kiss FFT configuration
    free (cfg);
    
    delete[] fftIn;
    delete[] fftOut;
#endif
}

//=======================================================================
template <class T>
void Gist<T>::performFFT()
{
#ifdef USE_FFTW
    // copy samples from audio frame
    for (int i = 0; i < frameSize; i++)
    {
        fftIn[i][0] = (double)audioFrame[i];
        fftIn[i][1] = (double)0.0;
    }
    
    // perform the FFT
    fftw_execute (p);
    
    // store real and imaginary parts of FFT
    for (int i = 0; i < frameSize; i++)
    {
        fftReal[i] = (T)fftOut[i][0];
        fftImag[i] = (T)fftOut[i][1];
    }
#endif
    
#ifdef USE_KISS_FFT
    for (int i = 0; i < frameSize; i++)
    {
        fftIn[i].r = (double)audioFrame[i];
        fftIn[i].i = 0.0;
    }
    
    // execute kiss fft
    kiss_fft (cfg, fftIn, fftOut);
    
    // store real and imaginary parts of FFT
    for (int i = 0; i < frameSize; i++)
    {
        fftReal[i] = (T)fftOut[i].r;
        fftImag[i] = (T)fftOut[i].i;
    }
#endif
    
#ifdef USE_ACCELERATE_FFT
    
    T inputFrame[frameSize];
    T outputReal[frameSize];
    T outputImag[frameSize];
    
    for (int i = 0; i < frameSize; i++)
    {
        inputFrame[i] = audioFrame[i];
    }
    
    accelerateFFT.performFFT (inputFrame, outputReal, outputImag);
    
    for (int i = 0; i < frameSize; i++)
    {
        fftReal[i] = outputReal[i];
        fftImag[i] = outputImag[i];
    }
    
#endif
    
    // calculate the magnitude spectrum
    for (int i = 0; i < frameSize / 2; i++)
    {
        magnitudeSpectrum[i] = sqrt ((fftReal[i] * fftReal[i]) + (fftImag[i] * fftImag[i]));
    }
}

//===========================================================
template class Gist<float>;
template class Gist<double>;