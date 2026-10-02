#pragma once

#include "ofMain.h"
#include "ofxTimeline.h"
#include "ofxTLExternalControl.h"

// A bank of on-screen "motorised faders" standing in for a real controller.
// Swap them for MIDI (or OSC, serial...) in moveFader() and the mouse handlers.
class ofApp : public ofBaseApp {
  public:
	void setup();
	void draw();
	void keyPressed(int key);
	void mousePressed(int x, int y, int button);
	void mouseDragged(int x, int y, int button);
	void mouseReleased(int x, int y, int button);

	// The timeline asks for a fader to move here
	void moveFader(ofxTLControlMessage& message);

	ofxTimeline timeline;
	ofxTLExternalControl control;

	static const int NUM_FADERS = 4;
	float faderPosition[NUM_FADERS];
	int grabbedFader = -1;
	ofRectangle faderArea(int i);
};
