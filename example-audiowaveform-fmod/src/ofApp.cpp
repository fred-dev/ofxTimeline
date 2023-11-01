/**
 * ofxTimeline -- AudioWaveform Example
 * openFrameworks graphical timeline addon
 *
 * Copyright (c) 2011-2012 James George http://www.jamesgeorge.org
 * Development Supported by YCAM InterLab http://interlab.ycam.jp/en/ +
 *
 * http://github.com/YCAMinterLab
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
 */

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	
    ofBackground(0);
    ofEnableSmoothing();
    
    timeline.setup();
    timeline.setLoopType(OF_LOOP_NORMAL);
    timeline.setBPM(120.f);
    timeline.enableSnapToBPM(true);
    timeline.setShowBPMGrid(true);
    timeline.addAudioFMOD3DTrack("Audio","mono_panner.wav", FMOD_PAN_TYPE::FMOD_PAN_2D);
    //timeline.getAudioTrack("Audio")->setPanType(OPENAL_PAN_2D);
    
    //this means that calls to play/stop etc will be  routed to the waveform and that timing will be 100% accurate
    timeline.setTimecontrolTrack("Audio");

    //fun to watch on FFT
    //timeline.addAudioTrackWithPath("audiocheck.net_sweep20-20klog.wav");
    //timeline.addAudioTrackWithPath("audiocheck.net_sweep20-20klin.wav");

    timeline.setDurationInSeconds(timeline.getAudioTrack("Audio")->getDuration());
    listener.setScale(1);
    listener.setPosition(0, 0, 0);
    listener.setOrientation(glm::vec3(0,0,0));
    soundSource.setScale(1);
    soundSource.setPosition(0, 0, 0);
    soundSource.setOrientation(glm::vec3(0,0,0));

}

//--------------------------------------------------------------
void ofApp::update(){
    soundSource.setPosition(glm::vec3(ofMap(ofGetMouseX(), 0, ofGetWidth(), -2, 2, true), ofMap(ofGetMouseY(), 0, ofGetHeight(), -2, 2, true),0.1));
    soundSource.lookAt(listener);
    
    timeline.getAudioTrack("Audio")->setPosition3D(soundSource.getPosition());
    
    // Get the position of the soundSource
    glm::vec3 soundSourcePosition = soundSource.getPosition();

    // Get the position of the listener
    glm::vec3 listenerPosition = listener.getPosition();

    // Calculate the direction vector from soundSource to listener
    glm::vec3 directionVector = glm::normalize(listenerPosition - soundSourcePosition);
    timeline.getAudioTrack("Audio")->setDirection3D(directionVector);
    

}

//--------------------------------------------------------------
void ofApp::draw(){
    
    //change the background color based on the current bin and amount

    ofxTLAudioTrack* track = timeline.getAudioTrack("Audio");
    int bin = ofMap(mouseX, 0, ofGetWidth(), 0, track->getFFTSize()-1, true);
    //ofBackground( track->getFFT()[bin] * 2000 );
    
    timeline.draw();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case 'f':
            ofToggleFullscreen();
            break;
        case 'P':
            cout << "pan type: " << timeline.getAudioTrack("Audio")->getPanType() << endl;
            cout << "pan 2d: " << timeline.getAudioTrack("Audio")->getPosition2D() << endl;
            cout << "pan 3d: " << timeline.getAudioTrack("Audio")->getPosition3D() << endl;
            cout << "velocity 3d: " << timeline.getAudioTrack("Audio")->getVelocity3D() << endl;
            cout << "Direction 3d: " << timeline.getAudioTrack("Audio")->getDirection3D() << endl;

            
        default:
            break;
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
