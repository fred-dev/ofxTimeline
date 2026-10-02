/**
 * ofxTLExternalControl
 * See the header for how to connect a controller.
 */

#include "ofxTLExternalControl.h"
#include "ofxTimeline.h"
#include "ofxTLKeyframes.h"

ofxTLExternalControl::ofxTLExternalControl(){
	timeline = NULL;
	autoUpdate = false;
	wasPlaying = false;
	writeInterval = 40;
	touchTimeout = 300;
	sendThreshold = 1.0 / 1024.0;
	returnTime = 300;
}

ofxTLExternalControl::~ofxTLExternalControl(){
	if(autoUpdate){
		ofRemoveListener(ofEvents().update, this, &ofxTLExternalControl::onUpdate, OF_EVENT_ORDER_AFTER_APP);
	}
}

void ofxTLExternalControl::setup(ofxTimeline* _timeline, bool _autoUpdate){
	timeline = _timeline;
	if(_autoUpdate && !autoUpdate){
		ofAddListener(ofEvents().update, this, &ofxTLExternalControl::onUpdate, OF_EVENT_ORDER_AFTER_APP);
	}
	else if(!_autoUpdate && autoUpdate){
		ofRemoveListener(ofEvents().update, this, &ofxTLExternalControl::onUpdate, OF_EVENT_ORDER_AFTER_APP);
	}
	autoUpdate = _autoUpdate;
}

void ofxTLExternalControl::onUpdate(ofEventArgs& args){
	update();
}

bool ofxTLExternalControl::bind(string trackName, int channel){
	if(timeline == NULL){
		ofLogError("ofxTLExternalControl::bind") << "call setup() with a timeline first";
		return false;
	}
	return bind(dynamic_cast<ofxTLKeyframes*>(timeline->getTrack(trackName)), channel);
}

bool ofxTLExternalControl::bind(ofxTLKeyframes* track, int channel){
	if(track == NULL){
		ofLogError("ofxTLExternalControl::bind") << "no keyframe track to bind to channel " << channel;
		return false;
	}
	Binding b;
	b.track = track;
	b.channel = channel;
	bindings[channel] = b;
	return true;
}

void ofxTLExternalControl::unbind(int channel){
	bindings.erase(channel);
}

void ofxTLExternalControl::unbindAll(){
	bindings.clear();
}

bool ofxTLExternalControl::isBound(int channel){
	return bindings.find(channel) != bindings.end();
}

vector<int> ofxTLExternalControl::getBoundChannels(){
	vector<int> channels;
	for(auto& it : bindings){
		channels.push_back(it.first);
	}
	return channels;
}

void ofxTLExternalControl::setMode(Mode mode){
	for(auto& it : bindings){
		setMode(it.first, mode);
	}
}

void ofxTLExternalControl::setMode(int channel, Mode mode){
	auto it = bindings.find(channel);
	if(it == bindings.end()) return;
	it->second.mode = mode;
	it->second.latched = false;
	it->second.writing = false;
	it->second.lastSent = -1; //resend so the fader shows the track again
}

ofxTLExternalControl::Mode ofxTLExternalControl::getMode(int channel){
	auto it = bindings.find(channel);
	return it == bindings.end() ? MODE_OFF : it->second.mode;
}

string ofxTLExternalControl::getModeName(Mode mode){
	switch(mode){
		case MODE_OFF: return "off";
		case MODE_READ: return "read";
		case MODE_TOUCH: return "touch";
		case MODE_LATCH: return "latch";
		case MODE_WRITE: return "write";
	}
	return "";
}

void ofxTLExternalControl::controllerMoved(int channel, float value){
	auto it = bindings.find(channel);
	if(it == bindings.end()) return;
	Binding& b = it->second;
	b.input = ofClamp(value, 0, 1);
	b.hasInput = true;
	b.inputChanged = true;
	b.lastMoveTime = ofGetElapsedTimeMillis();
}

void ofxTLExternalControl::controllerTouched(int channel, bool touched){
	auto it = bindings.find(channel);
	if(it == bindings.end()) return;
	it->second.touchSensing = true;
	it->second.touched = touched;
	if(touched){
		it->second.lastMoveTime = ofGetElapsedTimeMillis();
	}
}

bool ofxTLExternalControl::isTouched(Binding& b){
	if(b.touchSensing){
		return b.touched;
	}
	return b.hasInput && ofGetElapsedTimeMillis() - b.lastMoveTime < (unsigned long long)touchTimeout;
}

float ofxTLExternalControl::normalisedValueAt(Binding& b, unsigned long long millis){
	ofRange range = b.track->getValueRange();
	float v = b.track->getValueAtTimeInMillis(millis);
	return range.span() == 0 ? 0 : ofClamp((v - range.min) / range.span(), 0, 1);
}

float ofxTLExternalControl::getValue(int channel){
	auto it = bindings.find(channel);
	if(it == bindings.end() || timeline == NULL) return 0;
	return normalisedValueAt(it->second, timeline->getCurrentTimeMillis());
}

bool ofxTLExternalControl::isWriting(int channel){
	auto it = bindings.find(channel);
	return it != bindings.end() && it->second.writing;
}

void ofxTLExternalControl::setWriteInterval(int millis){
	writeInterval = MAX(1, millis);
}

void ofxTLExternalControl::setTouchTimeout(int millis){
	touchTimeout = MAX(0, millis);
}

void ofxTLExternalControl::setSendThreshold(float threshold){
	sendThreshold = MAX(0, threshold);
}

void ofxTLExternalControl::setReturnTime(int millis){
	returnTime = MAX(0, millis);
}

void ofxTLExternalControl::send(Binding& b, float value, bool force){
	if(!force && b.lastSent >= 0 && fabs(value - b.lastSent) < sendThreshold){
		return;
	}
	b.lastSent = value;
	ofxTLControlMessage message;
	message.channel = b.channel;
	message.value = value;
	message.trackName = b.track->getName();
	ofNotifyEvent(sendToController, message, this);
}

void ofxTLExternalControl::sendAll(){
	if(timeline == NULL) return;
	for(auto& it : bindings){
		if(it.second.mode != MODE_OFF){
			send(it.second, normalisedValueAt(it.second, timeline->getCurrentTimeMillis()), true);
		}
	}
}

//records the fader into the track, replacing what was there since the last write
void ofxTLExternalControl::writeValue(Binding& b, unsigned long long millis){
	ofRange range = b.track->getValueRange();
	float value = range.min + b.input * range.span();
	if(!b.wroteSinceStart || millis < b.lastWriteMillis){
		//first write of a pass, or playback jumped back (loop or seek)
		b.track->addKeyframeAtMillis(value, millis);
		b.lastWriteMillis = millis;
		b.wroteSinceStart = true;
		return;
	}
	if(millis > b.lastWriteMillis){
		b.track->deleteKeyframesInRange(b.lastWriteMillis + 1, millis);
	}
	if(millis - b.lastWriteMillis >= (unsigned long long)writeInterval){
		b.track->addKeyframeAtMillis(value, millis);
		b.lastWriteMillis = millis;
	}
}

void ofxTLExternalControl::update(){
	if(timeline == NULL) return;
	bool playing = timeline->getIsPlaying();
	unsigned long long now = timeline->getCurrentTimeMillis();

	for(auto& it : bindings){
		Binding& b = it.second;
		if(b.mode == MODE_OFF){
			b.writing = false;
			continue;
		}
		bool touched = isTouched(b);
		if(!playing && wasPlaying){
			b.latched = false; //latch ends when playback stops
		}

		bool shouldWrite = false;
		switch(b.mode){
			case MODE_TOUCH:
				shouldWrite = touched && b.hasInput;
				break;
			case MODE_LATCH:
				if(touched && b.hasInput) b.latched = true;
				shouldWrite = b.latched;
				break;
			case MODE_WRITE:
				shouldWrite = b.hasInput && (playing || touched);
				break;
			default:
				break;
		}

		if(shouldWrite){
			if(!b.writing){
				b.writing = true;
				b.wroteSinceStart = false;
			}
			if(playing){
				writeValue(b, now);
			}
			else if(b.inputChanged){
				//stopped: the fader edits the value at the playhead
				ofRange range = b.track->getValueRange();
				b.track->addKeyframeAtMillis(range.min + b.input * range.span(), now);
			}
			b.lastSent = b.input;
		}
		else{
			if(b.writing && playing){
				//punch out: keep the last written value at the release point
				writeValue(b, now);
			}
			b.writing = false;
			//never fight a hand on the fader; when it lets go, glide it back to
			//the track (eased), following the automation if the timeline is playing
			if(!touched){
				float target = normalisedValueAt(b, now);
				if(b.wasTouched && b.hasInput && returnTime > 0){
					b.gliding = true;
					b.glideFrom = b.input;
					b.glideStart = ofGetElapsedTimeMillis();
				}
				if(b.gliding){
					float t = float(ofGetElapsedTimeMillis() - b.glideStart) / returnTime;
					if(t >= 1){
						b.gliding = false;
						send(b, target, true);
					}
					else{
						float eased = t * t * (3 - 2 * t); //smoothstep
						send(b, ofLerp(b.glideFrom, target, eased), true);
					}
				}
				else{
					send(b, target, b.wasTouched);
				}
			}
			else{
				b.gliding = false;
			}
		}
		b.inputChanged = false;
		b.wasTouched = touched;
	}
	wasPlaying = playing;
}
