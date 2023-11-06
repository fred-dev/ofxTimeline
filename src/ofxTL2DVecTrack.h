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

#pragma once

#include "ofMain.h"
#include "ofxTLKeyframes.h"

class ofxTLVec2fSample : public ofxTLKeyframe {
  public:
    glm::vec2 samplePoint; //normalized to image space
	ofVec2f vec2f; //cached sample
};

class ofxTL2DVecTrack : public ofxTLKeyframes {
  public:
    ofxTL2DVecTrack();
	
	virtual void draw();
    virtual void drawModalContent();
    
	//For selecting keyframe type only,
    //the superclass controls keyframe placement
	virtual bool mousePressed(ofMouseEventArgs& args, long millis);
    virtual void mouseDragged(ofMouseEventArgs& args, long millis);
	virtual void mouseReleased(ofMouseEventArgs& args, long millis);
	virtual void keyPressed(ofKeyEventArgs& args);

    virtual string getTrackType();
	
	virtual void loadColorPalette(ofBaseHasPixels& image);
	virtual bool loadColorPalette(string imagePath);
	virtual string getPalettePath(); //only valid when it's been loaded from an image path
	
    ofVec2f getVec2f();
	ofVec2f getVec2fAtSecond(float second);
	ofVec2f getVec2fAtMillis(unsigned long long millis);
	ofVec2f getVec2fAtPosition(float pos);

	virtual void setDefaultVec2f(ofVec2f vec2f);
	virtual ofVec2f getDefaultVec2f();
	virtual void regionSelected(ofLongRange timeRange, ofRange valueRange);
	
  protected:
	ofImage vec2fModalBackground;
	ofImage previewPalette;
	string palettePath;
	
	virtual void updatePreviewPalette();
	virtual ofxTLKeyframe* newKeyframe();
    virtual ofxTLKeyframe* keyframeAtScreenpoint(ofVec2f p);
	
	ofVec2f vec2fAtClickTime;
	ofVec2f samplePositionAtClickTime;
	bool clickedInVec2fRect;
	bool drawingVec2fWindow;
	bool setNextAndPreviousOnUpdate;
	ofRectangle vec2fWindow;
	ofRectangle previousVec2fRect;
	ofRectangle newVec2fRect;
	
    virtual void restoreKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore);
	virtual void storeKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore);
    virtual void selectedKeySecondaryClick(ofMouseEventArgs& args);
	
	void refreshAllSamples();
	//set when selecting
	void setNextAndPreviousSamples();
	ofxTLVec2fSample* previousSample;
	ofxTLVec2fSample* nextSample;
	void refreshSample(ofxTLVec2fSample* sample);
   ofVec2f samplePaletteAtPosition(glm::vec2 position);
	
	ofVec2f defaultVec2f;
	
};


