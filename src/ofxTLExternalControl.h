/**
 * ofxTLExternalControl
 *
 * Links keyframe tracks (curves, LFOs...) to an external controller such as a
 * bank of motorised faders, without depending on any protocol. The timeline
 * tells you where each fader should be through sendToController; you turn that
 * into MIDI, OSC, serial or anything else. Fader moves and touches come back in
 * through controllerMoved() and controllerTouched(), and can be recorded into
 * the tracks while the timeline plays, like automation on a mixing desk.
 *
 * Values are always normalised 0-1 across the track's value range.
 *
 *	timeline.addCurves("Volume", ofRange(0, 1));
 *	control.setup(&timeline);
 *	control.bind("Volume", 1);                  // fader 1
 *	control.setMode(ofxTLExternalControl::MODE_TOUCH);
 *	ofAddListener(control.sendToController, this, &ofApp::moveFader);
 *	...
 *	void ofApp::moveFader(ofxTLControlMessage & m){
 *		midiOut.sendPitchBend(m.channel, m.value * 16383);   // e.g. with ofxMidi
 *	}
 *	void ofApp::newMidiMessage(ofxMidiMessage & msg){
 *		control.controllerMoved(msg.channel, msg.value / 16383.0);
 *	}
 */

#pragma once

#include "ofMain.h"

class ofxTimeline;
class ofxTLKeyframes;

class ofxTLControlMessage : public ofEventArgs {
  public:
	int channel;        // the channel the track was bound to
	float value;        // 0-1 across the track's value range
	string trackName;
};

class ofxTLExternalControl {
  public:
	enum Mode {
		MODE_OFF,       // nothing is sent or recorded
		MODE_READ,      // the timeline drives the faders; fader moves are ignored
		MODE_TOUCH,     // records while a fader is touched, goes back to reading when released
		MODE_LATCH,     // starts recording when a fader is touched, keeps going until playback stops
		MODE_WRITE      // records every bound fader whenever the timeline plays
	};

	ofxTLExternalControl();
	virtual ~ofxTLExternalControl();

	//update() is called automatically after the app's update unless you pass false
	void setup(ofxTimeline* timeline, bool autoUpdate = true);
	void update();

	//bind a keyframe track to a controller channel (fader number, CC number...)
	//returns false if the track isn't found or isn't a keyframe track
	bool bind(string trackName, int channel);
	bool bind(ofxTLKeyframes* track, int channel);
	void unbind(int channel);
	void unbindAll();
	bool isBound(int channel);
	vector<int> getBoundChannels();

	void setMode(Mode mode);              //all channels
	void setMode(int channel, Mode mode);
	Mode getMode(int channel);
	static string getModeName(Mode mode);

	//from the controller
	void controllerMoved(int channel, float value);   //value 0-1
	void controllerTouched(int channel, bool touched); //for faders with touch sensing

	//to the controller: the position each fader should move to
	ofEvent<ofxTLControlMessage> sendToController;

	//sends every bound fader its current position, e.g. after the controller connects
	void sendAll();

	//current normalised value of a bound track at the playhead
	float getValue(int channel);
	bool isWriting(int channel);

	//minimum time between recorded keyframes (default 40ms)
	void setWriteInterval(int millis);
	//for controllers without touch sensing, a move counts as a touch for this long (default 300ms)
	void setTouchTimeout(int millis);
	//only send when a value has changed by at least this much (default 1/1024)
	void setSendThreshold(float threshold);
	//when a fader is let go, it glides back to the track value over this time
	//instead of jumping (default 300ms, 0 to jump)
	void setReturnTime(int millis);

  protected:
	struct Binding {
		ofxTLKeyframes* track = NULL;
		int channel = 0;
		Mode mode = MODE_READ;
		bool touchSensing = false;     //becomes true once controllerTouched() is called
		bool touched = false;
		bool latched = false;
		bool writing = false;
		bool hasInput = false;
		float input = 0;
		float lastSent = -1;
		unsigned long long lastMoveTime = 0;
		unsigned long long lastWriteMillis = 0;
		bool wroteSinceStart = false;
		bool inputChanged = false;
		bool wasTouched = false;
		bool gliding = false;
		float glideFrom = 0;
		unsigned long long glideStart = 0;
	};

	void onUpdate(ofEventArgs& args);
	bool isTouched(Binding& b);
	void writeValue(Binding& b, unsigned long long millis);
	void send(Binding& b, float value, bool force = false);
	float normalisedValueAt(Binding& b, unsigned long long millis);

	ofxTimeline* timeline;
	map<int, Binding> bindings;
	bool autoUpdate;
	bool wasPlaying;
	int writeInterval;
	int touchTimeout;
	float sendThreshold;
	int returnTime;
};
