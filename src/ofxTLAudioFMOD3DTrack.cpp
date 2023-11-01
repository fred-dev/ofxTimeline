/**
 * ofxTimeline
 * openFrameworks graphical timeline addon
 *
 * Copyright (c) 2011-2012 James George
 * Development Supported by YCAM InterLab http://interlab.ycam.jp/en/
 * http://jamesgeorge.org + http://flightphase.com
 * http://github.com/obviousjim + http://github.com/flightphase
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ofxTLAudioFMOD3DTrack.h"

#ifdef TIMELINE_AUDIO_INCLUDED

#include "ofxTimeline.h"

ofxTLAudioFMOD3DTrack::ofxTLAudioFMOD3DTrack(){
	shouldRecomputePreview = false;
    soundLoaded = false;
    useEnvelope = true;
    dampening = .1;
	lastFFTPosition = -1;
	defaultSpectrumBandwidth = 1024;
	maxBinReceived = 0;
}

ofxTLAudioFMOD3DTrack::~ofxTLAudioFMOD3DTrack(){

}

bool ofxTLAudioFMOD3DTrack::loadSoundfile(string filepath){
	soundLoaded = false;
	if(player.load(filepath, false)){
    	soundLoaded = true;
		soundFilePath = filepath;
		shouldRecomputePreview = true;
        player.getSpectrum(defaultSpectrumBandwidth);
        setFFTLogAverages();
        averageSize = player.getAverages().size();
    }
	return soundLoaded;
}
 
string ofxTLAudioFMOD3DTrack::getSoundfilePath(){
	return soundFilePath;
}

bool ofxTLAudioFMOD3DTrack::isSoundLoaded(){
	return soundLoaded;
}

float ofxTLAudioFMOD3DTrack::getDuration(){
	return player.getDuration();
}

void ofxTLAudioFMOD3DTrack::update(){
	if(this == timeline->getTimecontrolTrack()){
		if(getIsPlaying()){
			if(player.getPosition() < lastPercent){
				ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
				ofNotifyEvent(events().playbackLooped, args);
			}
			lastPercent = player.getPosition();			
			//currently only supports timelines with duration == duration of player
			if(lastPercent < timeline->getInOutRange().min){

				player.setPosition( positionForSecond(timeline->getInTimeInSeconds())+.001 );
			}
			else if(lastPercent > timeline->getInOutRange().max){
				if(timeline->getLoopType() == OF_LOOP_NONE){
					player.setPosition( positionForSecond(timeline->getInTimeInSeconds()));
					stop();
				}
				else{
					player.setPosition( positionForSecond(timeline->getInTimeInSeconds()));
				}
			}
			
			timeline->setCurrentTimeSeconds(player.getPosition() * player.getDuration());
		}
	}
}
 
void ofxTLAudioFMOD3DTrack::draw(){
	
	if(!soundLoaded || player.getBuffer().size() == 0){
		ofPushStyle();
		ofSetColor(timeline->getColors().disabledColor);
		ofDrawRectangle(bounds);
		ofPopStyle();
		return;
	}
		
	if(shouldRecomputePreview || viewIsDirty){
//		cout << "recomputing waveform for audio file " << getSoundfilePath() << endl;
		recomputePreview();
	}

    ofPushStyle();
    ofSetColor(timeline->getColors().keyColor);
    ofNoFill();
    
    for(int i = 0; i < previews.size(); i++){
        ofPushMatrix();
        ofTranslate( normalizedXtoScreenX(computedZoomBounds.min, zoomBounds) - normalizedXtoScreenX(zoomBounds.min, zoomBounds), 0, 0);
        ofScale(computedZoomBounds.span()/zoomBounds.span(), 1, 1);
        previews[i].draw();
        ofPopMatrix();
    }
    ofPopStyle();
	

    
    ofPushStyle();
    
    //will refresh fft bins for other calls too
    vector<float>& bins = getFFT();
    float binWidth = bounds.width / bins.size();
    
    ofFill();
    ofSetColor(timeline->getColors().disabledColor, 120);
    for(int i = 0; i < bins.size(); i++){
        float height = MIN(bounds.height * bins[i], bounds.height);
        float y = bounds.y + bounds.height - height;
		ofDrawRectangle(i*binWidth, y, binWidth, height);
    }
    
    ofPopStyle();
}

float ofxTLAudioFMOD3DTrack::positionForSecond(float second){
	if(isSoundLoaded()){
		return ofMap(second, 0, player.getDuration(), 0, 1.0, true);
	}
	return 0;
}

void ofxTLAudioFMOD3DTrack::recomputePreview(){
	
	previews.clear();
	
//	cout << "recomputing view with zoom bounds of " << zoomBounds << endl;
	
	float normalizationRatio = timeline->getDurationInSeconds() / player.getDuration(); //need to figure this out for framebased...but for now we are doing time based
	float trackHeight = bounds.height/(1+player.getNumChannels());
	int numSamples = player.getBuffer().size() / player.getNumChannels();
	int pixelsPerSample = numSamples / bounds.width;
	int numChannels = player.getNumChannels();
	vector<short> & buffer  = player.getBuffer();

	for(int c = 0; c < numChannels; c++){
		ofPolyline preview;
		int lastFrameIndex = 0;
		preview.resize(bounds.width*2);  //Why * 2? Because there are two points per pixel, center and outside. 
		for(float i = bounds.x; i < bounds.x+bounds.width; i++){
			float pointInTrack = screenXtoNormalizedX( i ) * normalizationRatio; //will scale the screenX into wave's 0-1.0
			float trackCenter = bounds.y + trackHeight * (c+1);
			
            glm::vec3 * vertex = & preview.getVertices()[ (i - bounds.x) * 2];
			
			if(pointInTrack >= 0 && pointInTrack <= 1.0){
				//draw sample at pointInTrack * waveDuration;
				int frameIndex = pointInTrack * numSamples;					
				float losample = 0;
				float hisample = 0;
				for(int f = lastFrameIndex; f < frameIndex; f++){
					int sampleIndex = f * numChannels + c;
					float subpixelSample = buffer[sampleIndex]/32565.0;
					if(subpixelSample < losample) {
						losample = subpixelSample;
					}
					if(subpixelSample > hisample) {
						hisample = subpixelSample;
					}
				}
				
				if(losample == 0 && hisample == 0){
					//preview.addVertex(i, trackCenter);
					vertex->x = i;
					vertex->y = trackCenter;
					vertex++;
				}
				else {
					if(losample != 0){
//						preview.addVertex(i, trackCenter - losample * trackHeight);
						vertex->x = i;
						vertex->y = trackCenter - losample * trackHeight*.5;
						vertex++;
					}
					if(hisample != 0){
						//ofVertex(i, trackCenter - hisample * trackHeight);
//						preview.addVertex(i, trackCenter - hisample * trackHeight);
						vertex->x = i;
						vertex->y = trackCenter - hisample * trackHeight*.5;
						vertex++;
					}
				}
				
				while (vertex < & preview.getVertices()[ (i - bounds.x) * 2] + 2) {
					*vertex = *(vertex-1);
					vertex++;
				}

				lastFrameIndex = frameIndex;
			}
			else{
				*vertex++ = ofPoint(i,trackCenter);
				*vertex++ = ofPoint(i,trackCenter);
			}
		}
		preview.simplify();
		previews.push_back(preview);
	}
	computedZoomBounds = zoomBounds;
	shouldRecomputePreview = false;
}

bool ofxTLAudioFMOD3DTrack::mousePressed(ofMouseEventArgs& args, long millis){
	return false;
}

void ofxTLAudioFMOD3DTrack::mouseMoved(ofMouseEventArgs& args, long millis){
}

void ofxTLAudioFMOD3DTrack::mouseDragged(ofMouseEventArgs& args, long millis){
}

void ofxTLAudioFMOD3DTrack::mouseReleased(ofMouseEventArgs& args, long millis){
}

void ofxTLAudioFMOD3DTrack::keyPressed(ofKeyEventArgs& args){
}

void ofxTLAudioFMOD3DTrack::zoomStarted(ofxTLZoomEventArgs& args){
	ofxTLTrack::zoomStarted(args);
//	shouldRecomputePreview = true;
}

void ofxTLAudioFMOD3DTrack::zoomDragged(ofxTLZoomEventArgs& args){
	ofxTLTrack::zoomDragged(args);
	//shouldRecomputePreview = true;
}

void ofxTLAudioFMOD3DTrack::zoomEnded(ofxTLZoomEventArgs& args){
	ofxTLTrack::zoomEnded(args);
	shouldRecomputePreview = true;
}

void ofxTLAudioFMOD3DTrack::boundsChanged(ofEventArgs& args){
	shouldRecomputePreview = true;
}

void ofxTLAudioFMOD3DTrack::play(){

	if(!player.isPlaying()){
        
//		lastPercent = MIN(timeline->getPercentComplete() * timeline->getDurationInSeconds() / player.getDuration(), 1.0);
		player.setLoop(timeline->getLoopType() == OF_LOOP_NORMAL);
		player.play();
		if(timeline->getTimecontrolTrack() == this){
			if(player.getPosition() == 1.0 || timeline->getPercentComplete() > .99){
                timeline->setCurrentTimeSeconds(0);
            }
            
            player.setPosition(positionForSecond(timeline->getCurrentTime()));
            //cout << " setting time to  " << positionForSecond(timeline->getCurrentTime()) << " actual " << player.getPosition() << endl;
            
			ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
			ofNotifyEvent(events().playbackStarted, args);
		}
	}
}

void ofxTLAudioFMOD3DTrack::stop(){
	if(player.isPlaying()){
		
		player.setPaused(true);

		if(timeline->getTimecontrolTrack() == this){
			ofxTLPlaybackEventArgs args = timeline->createPlaybackEvent();
			ofNotifyEvent(events().playbackEnded, args);
		}
	}
}

void ofxTLAudioFMOD3DTrack::playbackStarted(ofxTLPlaybackEventArgs& args){
	ofxTLTrack::playbackStarted(args);
	if(isSoundLoaded() && this != timeline->getTimecontrolTrack()){
		//player.setPosition(timeline->getPercentComplete());
		float position = positionForSecond(timeline->getCurrentTime());
		if(position < 1.0){
			player.play();
		}
		player.setPosition( position );
	}
}

void ofxTLAudioFMOD3DTrack::playbackLooped(ofxTLPlaybackEventArgs& args){
	if(isSoundLoaded() && this != timeline->getTimecontrolTrack()){
		if(!player.isPlaying()){
			player.play();
		}
		player.setPosition( positionForSecond(timeline->getCurrentTime()) );
	}
}

void ofxTLAudioFMOD3DTrack::playbackEnded(ofxTLPlaybackEventArgs& args){
	if(isSoundLoaded() && this != timeline->getTimecontrolTrack()){
		player.stop();
	}
}

bool ofxTLAudioFMOD3DTrack::togglePlay(){
	if(getIsPlaying()){
		stop();
	}
	else {
		play();
	}
	return getIsPlaying();
}

bool ofxTLAudioFMOD3DTrack::getIsPlaying(){
    return player.isPlaying();
}

void ofxTLAudioFMOD3DTrack::setSpeed(float speed){
    player.setSpeed(speed);
}

float ofxTLAudioFMOD3DTrack::getSpeed(){
    return player.getSpeed();
}

void ofxTLAudioFMOD3DTrack::setVolume(float volume){
    player.setVolume(volume);
}

void ofxTLAudioFMOD3DTrack::setPan(float pan){
    player.setPan(pan);
}

void ofxTLAudioFMOD3DTrack::setUseFFTEnvelope(bool useFFTEnveolope){
    useEnvelope = useFFTEnveolope;
}

bool ofxTLAudioFMOD3DTrack::getUseFFTEnvelope(){
    return useEnvelope;
}

void ofxTLAudioFMOD3DTrack::setFFTDampening(float damp){
    dampening = damp;
}

float ofxTLAudioFMOD3DTrack::getFFTDampening(){
    return dampening;
}

void ofxTLAudioFMOD3DTrack::setFFTLogAverages(int minBandwidth, int bandsPerOctave){
    if(isSoundLoaded()){
        player.setLogAverages(minBandwidth, bandsPerOctave);
    }
}

int ofxTLAudioFMOD3DTrack::getLogAverageMinBandwidth(){
    return player.getMinBandwidth();
}

int ofxTLAudioFMOD3DTrack::getLogAverageBandsPerOctave(){
    return player.getBandsPerOctave();
}

int ofxTLAudioFMOD3DTrack::getFFTSize(){
    return averageSize;
}

//envelope and dampening approach from Marius Watz
//http://workshop.evolutionzone.com/2012/08/30/workshops-sept-89-sound-responsive-visuals-3d-printing-and-parametric-modeling/
vector<float>& ofxTLAudioFMOD3DTrack::getFFT(){
	float fftPosition = player.getPosition();
	if(isSoundLoaded() && lastFFTPosition != fftPosition){

        vector<float>& fftAverages = player.getAverages();
        averageSize = fftAverages.size();
        if(envelope.size() != averageSize){
            generateEnvelope(averageSize);
        }
        
        if(dampened.size() != averageSize){
            dampened.clear();
            dampened.resize(averageSize);
        }

        if(getUseFFTEnvelope()){
            for(int i = 0; i < fftAverages.size(); i++){
                fftAverages[i] *= envelope[i];
            }
        }
        
        float max = 0;
        for(int i = 0; i < fftAverages.size(); i++){
            max = MAX(max, fftAverages[i]);
        }
        if(max != 0){
            for(int i = 0; i < fftAverages.size(); i++){
                fftAverages[i] = ofMap(fftAverages[i],0, max, 0, 1.0);
            }
        }
 
        for(int i = 0; i < averageSize; i++) {
            dampened[i] = (fftAverages[i] * dampening) + dampened[i]*(1-dampening);
        }
        
        //normalizer hack
        lastFFTPosition = fftPosition;
	}
    
	return dampened;
}

int ofxTLAudioFMOD3DTrack::getBufferSize()
{
    return player.getBuffer().size() / player.getNumChannels();
}

vector<float>& ofxTLAudioFMOD3DTrack::getCurrentBuffer(int _size)
{
    buffered = player.getCurrentBuffer(_size);
    return buffered;
}

vector<float>& ofxTLAudioFMOD3DTrack::getBufferForFrame(int _frame, int _size)
{
    if(_frame != lastBufferPosition)
    {
        lastBufferPosition = _frame;
        buffered = player.getBufferForFrame(_frame, timeline->getTimecode().getFPS(), _size);
        return buffered;
    }
    return buffered;
}

void ofxTLAudioFMOD3DTrack::generateEnvelope(int size){
    envelope.clear();
    
    for(int i = 0; i < size; i++) {
        envelope.push_back(ofBezierPoint(ofPoint(0.05,0),
                                         ofPoint(0.1, 0),
                                         ofPoint(0.2, 0),
                                         ofPoint(1.0, 0),
                                         ofMap(i, 0,size-1, 0,1) ).x);
    }
}

string ofxTLAudioFMOD3DTrack::getTrackType(){
    return "Audio";    
}

void ofxTLAudioFMOD3DTrack::setPanType(FMOD_PAN_TYPE _panType){
    player.setPanType(_panType);
}

FMOD_PAN_TYPE ofxTLAudioFMOD3DTrack::getPanType(){
    return player.getPanType();
}

void ofxTLAudioFMOD3DTrack::setPosition2D(glm::vec3 position2D){
    player.setPosition2D(position2D);
}

void ofxTLAudioFMOD3DTrack::setPosition3D(glm::vec3 position3D){
    player.setPosition3D(position3D);
}

glm::vec3 ofxTLAudioFMOD3DTrack::getPosition2D(){
    return player.getPosition2D();
}
glm::vec3 ofxTLAudioFMOD3DTrack::getPosition3D(){
    return player.getPosition3D();
}

#endif  // TIMELINE_AUDIO_INCLUDED
