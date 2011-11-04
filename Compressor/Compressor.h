/*
*	File:		Compressor.h
*	
*	Version:	1.0
* 
*	Created:	4/5/10
*	
*	Copyright:  Copyright © 2010 Tony Audio, All Rights Reserved
* 
*/
#include "AUEffectBase.h"
#include "CompressorVersion.h"

#if AU_DEBUG_DISPATCHER
	#include "AUDebugDispatcher.h"
#endif


#ifndef __Compressor_h__
#define __Compressor_h__


#pragma mark ____Compressor Parameters

// parameters

//adjusts the signalgain prior to to the application of gain
static CFStringRef kParamInputGain= CFSTR("InputGain"); // provides the user interface name
static const float kMinimumInput = 0.0; //minimum value for InputGain
static const float kMaximumInput = 2.0; //max value for InputGain
static const float kDefaultInput = 1.0; //default value for InputGain

//adjusts the signalgain after to to the application of gain
static CFStringRef kParamOutputGain = CFSTR("OutputGain"); // provides the user interface name
static const float kMinimumOutput = 0.0; //minimum value for OutputGain
static const float kMaximumOutput = 2.0; //max value for OutputGain
static const float kDefaultOutput = 1.0; //default value for OutputGain

//sets the value which, if exceeded by the input signal, the compression
//	slope is applied
static CFStringRef kParamThreshold = CFSTR("Threshold"); // provides the user interface name
static const float kMinimumThresh = 0.0; //minimum value for Threshold
static const float kMaximumThresh = 1.0; //max value for Threshold
static const float kDefaultThresh = 0.5; //default value for Threshold

//Sets the slope of the compression after the signal exceededs the threshiold.
static CFStringRef kParamSlope = CFSTR("Slope"); // provides the user interface name
static const float kMinimumSlope = 0.0; //minimum value for Slope
static const float kMaximumSlope = 1.0; //max value for Slope
static const float kDefaultSlope = 0.5; //default value for Slope

enum {
	//set the ID number for each of the parameters
	kParameter_InputGain =0,
	kParameter_OutputGain = 1,
	kParameter_Threshold = 2,
	kParameter_Slope = 3,
	kNumberOfParameters=4
	
};

#pragma mark ____Compressor
class Compressor : public AUEffectBase
{
public:
	//Inputs: the audioUnit standard passed into this
	Compressor(AudioUnit component); 
#if AU_DEBUG_DISPATCHER
	virtual ~Compressor () { delete mDebugDispatcher; }
#endif
	
	//declare functions
	virtual AUKernelBase *		NewKernel() { return new CompressorKernel(this); }
	
	//gets strings for pop-up menu
	//Inputs: scope of audio unit, the parameter ID, array that holds the strings
	//Output: noErr if properly executed
	//	Required for compilation, but unused. It would allow drop-down menus for string named 
	//		parameters to be displayed
	virtual	ComponentResult		GetParameterValueStrings(AudioUnitScope			inScope,
														 AudioUnitParameterID		inParameterID,
														 CFArrayRef *			outStrings);

//	Compressor::GetParameterInfo
//	Initalizes and sets bounds for all parameters. called by the built in function GetParameter()
//	Inputs(all from host program): scope of audio unit, inputParameterID, output for parameter
//									info
//	Outputs: parameter info struct	
	virtual	ComponentResult		GetParameterInfo(AudioUnitScope			inScope,
												 AudioUnitParameterID	inParameterID,
												 AudioUnitParameterInfo	&outParameterInfo);
    
	// Declares GetPropertyInfo
	// This audio unit doesn't use any custom properties, so generic declaration is used here
	virtual ComponentResult		GetPropertyInfo(AudioUnitPropertyID		inID,
												AudioUnitScope			inScope,
												AudioUnitElement		inElement,
												UInt32 &			outDataSize,
												Boolean	&			outWritable );
	
		// Declares GetProperty
	// This audio unit doesn't use any custom properties, so generic declaration is used here
	virtual ComponentResult		GetProperty(AudioUnitPropertyID inID,
											AudioUnitScope 		inScope,
											AudioUnitElement 		inElement,
											void *			outData);
	
	// TailTime declares time for an audio signal to decay to silence, helps the host manage
	// the AU's latency.
	//because this Audio Unit does not use a clock, it is unnecessary to create a custom implementation
	// for tailTime
 	virtual	bool				SupportsTail () { return false; }
	
	/*! @method Version */
	virtual ComponentResult	Version() { return kCompressorVersion; }
	
    
	
protected:
	class CompressorKernel : public AUKernelBase		// most of the real work happens here
	{
public:
		CompressorKernel(AUEffectBase *inAudioUnit )
		: AUKernelBase(inAudioUnit)
	{
	}
		
		// *Required* overides for the process method for this effect
		// processes one channel of interleaved samples
		//Inputs: audio input stream pointer, audio output stream pointer, number of samples(frames) 
		//			to process in this stream,
		//			number of channels, boolean variable that is true if the input stream is silent
        virtual void 		Process(	const Float32 	*inSourceP,
										Float32		 	*inDestP,
										UInt32 			inFramesToProcess,
										 // for version 2 AudioUnits inNumChannels is always 1
										UInt32			inNumChannels, 
										bool			&ioSilence);
		
        virtual void		Reset();
		
		//populates array with gains to reference in processing the audio signal
		//Inputs: activation threshold, compression slope, array containing gains
		//Outputs: none
		
		virtual void		ReferenceTable(Float32 threshold, double slope, Float32 *gains);

		private: //state variables...
			enum {numOfGains = 1024};
			Float32 gains[numOfGains]; //array that holds all of the precalculated gain values
	};
};


#endif