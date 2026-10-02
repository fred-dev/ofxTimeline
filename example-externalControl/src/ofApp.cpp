#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);
	ofBackground(25);

	timeline.setup();
	timeline.setDurationInSeconds(20);
	timeline.setLoopType(OF_LOOP_NORMAL);
	timeline.setWidth(ofGetWidth() - 260);

	// Four tracks, each bound to a fader. Values are normalised to 0-1
	// across each track's range before they reach the controller.
	control.setup(&timeline);
	for(int i = 0; i < NUM_FADERS; i++){
		string name = "Fader " + ofToString(i + 1);
		timeline.addCurves(name, ofRange(0, 127));
		control.bind(name, i + 1);
		faderPosition[i] = 0;
	}
	control.setMode(ofxTLExternalControl::MODE_TOUCH);
	ofAddListener(control.sendToController, this, &ofApp::moveFader);
}

//--------------------------------------------------------------
void ofApp::moveFader(ofxTLControlMessage& message){
	// With a real motorised controller you would send MIDI here, e.g. with ofxMidi:
	//   midiOut.sendPitchBend(message.channel, message.value * 16383);   // Mackie-style faders
	//   midiOut.sendControlChange(1, message.channel, message.value * 127);
	faderPosition[message.channel - 1] = message.value;
}

//--------------------------------------------------------------
ofRectangle ofApp::faderArea(int i){
	float x = ofGetWidth() - 240 + i * 60;
	return ofRectangle(x, 60, 40, ofGetHeight() - 160);
}

//--------------------------------------------------------------
void ofApp::draw(){
	timeline.draw();

	for(int i = 0; i < NUM_FADERS; i++){
		ofRectangle r = faderArea(i);
		int channel = i + 1;
		ofSetColor(60);
		ofDrawRectangle(r.getCenter().x - 2, r.y, 4, r.height);
		float y = r.getBottom() - faderPosition[i] * r.height;
		ofSetColor(control.isWriting(channel) ? ofColor(230, 60, 60) : ofColor(200));
		ofDrawRectangle(r.x, y - 8, r.width, 16);
		ofSetColor(200);
		ofDrawBitmapString(ofToString(channel), r.x + 15, r.getBottom() + 20);
	}

	ofSetColor(220);
	ofDrawBitmapString("space: play/stop   mode: 0 off  1 read  2 touch  3 latch  4 write\n"
					   "current mode: " + ofxTLExternalControl::getModeName(control.getMode(1)) +
					   "   drag the faders on the right (red = recording)", 20, ofGetHeight() - 40);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if(key == ' '){
		timeline.togglePlay();
	}
	if(key >= '0' && key <= '4'){
		control.setMode((ofxTLExternalControl::Mode)(key - '0'));
	}
}

//--------------------------------------------------------------
// These mouse handlers stand in for MIDI input from the controller.
void ofApp::mousePressed(int x, int y, int button){
	for(int i = 0; i < NUM_FADERS; i++){
		if(faderArea(i).inside(x, y)){
			grabbedFader = i;
			control.controllerTouched(i + 1, true);
			mouseDragged(x, y, button);
		}
	}
}

void ofApp::mouseDragged(int x, int y, int button){
	if(grabbedFader < 0) return;
	ofRectangle r = faderArea(grabbedFader);
	faderPosition[grabbedFader] = ofClamp((r.getBottom() - y) / r.height, 0, 1);
	control.controllerMoved(grabbedFader + 1, faderPosition[grabbedFader]);
}

void ofApp::mouseReleased(int x, int y, int button){
	if(grabbedFader >= 0){
		control.controllerTouched(grabbedFader + 1, false);
	}
	grabbedFader = -1;
}
