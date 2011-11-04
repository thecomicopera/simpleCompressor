/*
*	File:		Compressor.h
*	
*	Version:	1.0
* 
*	Created:	4/5/10
*	
*	Copyright:  Copyright © 2010 Tony Audio, All Rights Reserved
*/
/*=============================================================================
	Compressor.h
	
=============================================================================*/
#include "Compressor.h"
#include <math.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

COMPONENT_ENTRY(Compressor)


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::Compressor
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Compressor::Compressor(AudioUnit component)
	: AUEffectBase(component)
{
	CreateElements();
	Globals()->UseIndexedParameters(kNumberOfParameters);
	//initialize parameters in the user interface
	//SetParameter is inherited from the Core Audio SDK. It sets up the user interface.
	//Inputs: parameter, the default value
	//Outputs: 
	SetParameter(kParameter_InputGain, kDefaultInput ); //sets up first parameter view
	SetParameter(kParameter_OutputGain, kDefaultOutput );//sets up second parameter view
	SetParameter( kParameter_Threshold, kDefaultThresh );//sets up third parameter view
	SetParameter( kParameter_Slope,  kDefaultSlope);//sets up fourth parameter view
        
#if AU_DEBUG_DISPATCHER
	mDebugDispatcher = new AUDebugDispatcher (this);
#endif
	
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::GetParameterValueStrings
//	Required for compilation, but unused. It would allow drop-down menus for string named 
//		parameters to be displayed
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult		Compressor::GetParameterValueStrings(AudioUnitScope		inScope,
                                                                AudioUnitParameterID	inParameterID,
                                                                CFArrayRef *		outStrings)
{
        
    return kAudioUnitErr_InvalidProperty;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::GetParameterInfo
//	Initalizes and sets bounds for all parameters. called by the built in function GetParameter()
//	Inputs(all from host program): scope of audio unit, inputParameterID, output for parameter
//									info
//	Outputs: parameter info struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult		Compressor::GetParameterInfo(AudioUnitScope		inScope,
                                                        AudioUnitParameterID	inParameterID,
                                                        AudioUnitParameterInfo	&outParameterInfo )
{
	ComponentResult result = noErr;

	//These two flags indicate to the host program that all parameters are readable and writeable
	outParameterInfo.flags = 	kAudioUnitParameterFlag_IsWritable
						|		kAudioUnitParameterFlag_IsReadable;
    
	//if the host gives the sends a parameter control to a parameter scope, continue to set values for the
	// identified parameter.
	//	All parameters in this audio unit have global scope, so this should filter out invalid requests.
    if (inScope == kAudioUnitScope_Global) {
        switch(inParameterID)
        {
            case kParameter_InputGain:
				//defines how to represent the parameter in the interface
				// Inputs: output info structure, parameterID, unused boolean
				// Outputs: nothing
                AUBase::FillInParameterName (outParameterInfo, kParamInputGain, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 2.0;
                outParameterInfo.defaultValue = kDefaultInput;
                break;
            case kParameter_OutputGain:
                AUBase::FillInParameterName (outParameterInfo, kParamOutputGain, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 2.0;
                outParameterInfo.defaultValue = kDefaultOutput;
                break;
			case kParameter_Threshold:
                AUBase::FillInParameterName (outParameterInfo, kParamThreshold, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 1;
                outParameterInfo.defaultValue = kDefaultThresh;
                break;
			case kParameter_Slope:
                AUBase::FillInParameterName (outParameterInfo, kParamSlope, false);
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
                outParameterInfo.minValue = 0.0;
                outParameterInfo.maxValue = 1;
                outParameterInfo.defaultValue = kDefaultSlope;
                break;				
            default:
                result = kAudioUnitErr_InvalidParameter;
                break;
            }
	} else {
        result = kAudioUnitErr_InvalidParameter;
    }
    


	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::GetPropertyInfo
//	This audio unit doesn't use any custom properties, so generic code is used here
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult		Compressor::GetPropertyInfo (AudioUnitPropertyID	inID,
                                                        AudioUnitScope		inScope,
                                                        AudioUnitElement	inElement,
                                                        UInt32 &		outDataSize,
                                                        Boolean &		outWritable)
{
	return AUEffectBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::GetProperty4
//	This audio unit doesn't use any custom properties, so generic code is used here
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult		Compressor::GetProperty(	AudioUnitPropertyID inID,
                                                        AudioUnitScope 		inScope,
                                                        AudioUnitElement 	inElement,
                                                        void *			outData )
{
	return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
}


#pragma mark ____CompressorEffectKernel


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::CompressorKernel::Reset()
//	Puts all gains back to unity if the AU is reset.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		Compressor::CompressorKernel::Reset()
{
	for(int n = 0; n<=numOfGains; n++)
	{
		gains[n] = 1.0;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::CompressorKernel::referenceTable()

		//populates array with gains to reference in processing the audio signal
		//Inputs: activation threshold, compression slope
		//Outputs: *changes the gains array by reference*
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	Compressor::CompressorKernel::ReferenceTable(Float32 threshold, double slope, Float32 *gains)
{
		//multiplies the threshold by 1000 for calculation
		//There is a tradeoff between precision and speed, especially with very small
		// floating point values. I'm only making the gain calculation accurate to the 
		// 1/1000.
		
		//	multiply the threshold by 1000 in order to compare it to the counter
		//	so the compressor can find the right point to begin applying the 
		//  compression curve.
		Float32 comparisonThresh = threshold * 1000;
		//g represents the input signal * 1000
		for(int g = 0; g <= numOfGains; g++)
		{
			
			//if g hasn't passed the threshold, the compressor gain level maintains
			//	unity(or 1.0). Otherwise, apply the gain curve
			Float32 compgain = 1.0;
			if (g > comparisonThresh)
			{

				//change the input level to a float that is fractional of 1
				Float32 tableInput = g / 1000;
				gains[g] = (threshold + (slope * tableInput))/tableInput;
			
			}
			else //input is less than the threshold
			{
				//gain maintains unity
				gains[g] = compgain;
			}
		}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Compressor::CompressorKernel::Process
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Inputs: audio input stream pointer, audio output stream pointer, number of samples(frames) 
//			to process in this stream, number of channels, boolean variable that is true 
//			if the input stream is silent
void		Compressor::CompressorKernel::Process(	const Float32 	*inSourceP,
                                                    Float32		 	*inDestP,
                                                    UInt32 			inFramesToProcess,
                                                    // for version 2 AudioUnits inNumChannels is always 1
													UInt32			inNumChannels, 
                                                    bool			&ioSilence )
{
	if(!ioSilence)
	{
		//This code will pass-thru the audio data.
		//This is where you want to process data to produce an effect.

		
		//declaration and initialization
		UInt32 nSampleFrames = inFramesToProcess; //number of frames left to process
		const Float32 *sourceP = inSourceP; //audio stream input source
		Float32 *destP = inDestP;			//audio stream output destination
		//GetParameter() is inherited from AUBase
		//Inputs: the parameter ID
		//Outputs: the value of the parameter
		Float32 inputGain = GetParameter( kParameter_InputGain );	//gets the value of the inputGain
		Float32 outputGain = GetParameter( kParameter_OutputGain );	//gets the value of the outputGain
		double threshold = GetParameter( kParameter_Threshold );	//gets the value of the threshold
		double slope = GetParameter( kParameter_Slope );			//gets the value of the slope
		
	
		//populate list of gains
		ReferenceTable(threshold, slope, gains);

	
		//as long as the next sample is not the end of file
		while (nSampleFrames-- > 0) 
		{
			
			Float32 inputSample = *sourceP;
		
			sourceP += inNumChannels;	// advance to next frame

			//if the user changed threahold or slope, recalulate the gains
			if(threshold != GetParameter(kParameter_Threshold) || slope != GetParameter(kParameter_Slope))
			{
					threshold = GetParameter(kParameter_Threshold);
					slope = GetParameter(kParameter_Slope);
					ReferenceTable(threshold, slope, gains);
			}
			
			//apply inputGain to the audio Sample
			inputSample = inputSample * inputGain;
			
			//rounded off version of threshold
			//intentionally converting to an int, so the value may be used to reference an item in the array.
			int inputRounded = (ceil(fabs(inputSample)) * 1000);

			//get the gain level corresponding to the level(volume) of the sample.
			Float32 compgain = gains[inputRounded];
			
			// makes sure gain never exceeds unity.
			if(compgain > 1.0)
			{
				compgain = 1.0;
			}
			
			//apply compressor gain to the sample.
			Float32 outputSample = inputSample * compgain;

				
			// here's where you do your DSP work
			outputSample = inputSample * outputGain;
		
			//send out
			*destP = outputSample;
			destP += inNumChannels;
		}
	}
}

