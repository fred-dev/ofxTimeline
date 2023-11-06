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

#include "ofxTL2DVecTrack.h"
#include "ofxTimeline.h"
#include <cfloat>
#include "ofxHotKeys.h"

ofxTL2DVecTrack::ofxTL2DVecTrack()
 :	drawingVec2fWindow(false),
	clickedInVec2fRect(false),
	defaultVec2f(ofVec2f(0,0)),
	previousSample(NULL),
	nextSample(NULL),
	setNextAndPreviousOnUpdate(false)

{
	//
}

void ofxTL2DVecTrack::draw(){

	if(bounds.height == 0){
		return;
	}

	if(viewIsDirty || shouldRecomputePreviews){
		updatePreviewPalette();
	}

	if(keyframes.size() == 0){
		ofPushStyle();
		ofSetColor(defaultVec2f.x, defaultVec2f.y);
		ofFill();
		ofDrawRectangle(bounds);
		ofPopStyle();
	}
	else if(keyframes.size() == 1){
		ofPushStyle();
		ofxTLVec2fSample* s = (ofxTLVec2fSample*)keyframes[0];
		ofSetColor(s->vec2f.x, s->vec2f.y);
		ofFill();
		ofDrawRectangle(bounds);
		ofPopStyle();
	}
	else{
		previewPalette.draw(bounds);
	}

	for(int i = 0; i < keyframes.size(); i++){

		if(!isKeyframeIsInBounds(keyframes[i])){
			continue;
		}

		float screenX = millisToScreenX(keyframes[i]->time);

		ofVec2f a = ofVec2f(screenX-10,bounds.y);
		ofVec2f b = ofVec2f(screenX+10,bounds.y);
		ofVec2f c = ofVec2f(screenX,bounds.y+10);

		ofPushStyle();
		ofFill();
		ofxTLVec2fSample* s = (ofxTLVec2fSample*)keyframes[i];
        ofSetColor(s->vec2f.x, s->vec2f.y);
		ofDrawTriangle(a,b,c);
		ofNoFill();
		ofSetColor(ofColor(s->vec2f.x, s->vec2f.y ).getInverted());
		ofSetLineWidth(1);
		ofDrawTriangle(a,b,c);

		if(keyframes[i] == hoverKeyframe){
			ofSetColor(timeline->getColors().highlightColor);
			ofSetLineWidth(3);
		}
		else if(isKeyframeSelected(keyframes[i])){
			ofSetColor(timeline->getColors().textColor);
			ofSetLineWidth(2);
		}
		else{
            ofSetColor(ofColor(s->vec2f.x, s->vec2f.y ).getInverted());
		}
		ofDrawLine(c, ofVec2f(screenX, bounds.getMaxY()));
		ofPopStyle();
	}
}

void ofxTL2DVecTrack::drawModalContent(){
	if(drawingVec2fWindow){

		//this happens when a new keyframe is added
		//we need to wait until the draw cycle for the new
		//key to be in the array so we can determine it's
		//surrounding samples
		if(setNextAndPreviousOnUpdate){
			setNextAndPreviousSamples();
			setNextAndPreviousOnUpdate = false;
		}

		if(selectedKeyframe == NULL){
			ofLogError("ofxTL2DVecTrack::drawModalContent") << "The selected keyframe is null" << endl;
			drawingVec2fWindow = false;
			timeline->dismissedModalContent();
			return;
		}

		if(!vec2fModalBackground.isAllocated()){
			ofLogError("ofxTL2DVecTrack::drawModalContent") << "The color palette is not allocated" << endl;
			timeline->dismissedModalContent();
			drawingVec2fWindow = false;
		}
		ofPushStyle();
		ofFill();
		ofSetColor(255);

		ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
		vec2fWindow = ofRectangle( millisToScreenX(selectedKeyframe->time), bounds.y+bounds.height, 200, 200);
		if(vec2fWindow.getMaxY()+25 > timeline->getBottomLeft().y){
			vec2fWindow.y = bounds.y - 25 - vec2fWindow.height;
		}
		if(vec2fWindow.getMaxX() > ofGetWidth()){
			vec2fWindow.x -= vec2fWindow.width;
		}
		vec2fModalBackground.draw(vec2fWindow);

        glm::vec2 selectionPoint = vec2fWindow.getMin() + selectedSample->samplePoint * glm::vec2(vec2fWindow.width,vec2fWindow.height);
		ofSetColor(selectedSample->vec2f.x,selectedSample->vec2f.y);
		ofDrawLine(selectionPoint - ofVec2f(8,0), selectionPoint + ofVec2f(8,0));
		ofDrawLine(selectionPoint - ofVec2f(0,8), selectionPoint + ofVec2f(0,8));

		ofPushStyle();
		ofNoFill();
		if(previousSample != NULL){
			ofVec2f previousSamplePoint = vec2fWindow.getMin() + previousSample->samplePoint * ofVec2f(vec2fWindow.width,vec2fWindow.height);
			ofSetColor(ofColor(previousSample->vec2f.x,previousSample->vec2f.y).getInverted(), 150);
			ofDrawCircle(previousSamplePoint, 3);
			ofDrawLine(previousSamplePoint,selectionPoint);
		}
		if(nextSample != NULL){
			ofVec2f nextSamplePoint = vec2fWindow.getMin() + nextSample->samplePoint * ofVec2f(vec2fWindow.width,vec2fWindow.height);
			ofSetColor(nextSample->vec2f.x,nextSample->vec2f.y);

			//draw a little triangle pointer
			ofVec2f direction = (nextSamplePoint - selectionPoint).getNormalized();
			ofVec2f backStep = nextSamplePoint-direction*5;
			ofDrawTriangle(nextSamplePoint,
						   backStep + direction.getRotated(90)*3,
						   backStep - direction.getRotated(90)*3);
			ofDrawLine(nextSamplePoint,selectionPoint);
		}
		ofPopStyle();

		previousVec2fRect = ofRectangle(vec2fWindow.x, vec2fWindow.getMaxY(), vec2fWindow.width/2, 25);
		newVec2fRect = ofRectangle(vec2fWindow.x+vec2fWindow.width/2, vec2fWindow.getMaxY(), vec2fWindow.width/2, 25);

        ofSetColor(vec2fAtClickTime.x, vec2fAtClickTime.y);
		ofDrawRectangle(previousVec2fRect);
        ofSetColor(selectedSample->vec2f.x, selectedSample->vec2f.y);
		ofDrawRectangle(newVec2fRect);
		ofSetColor(timeline->getColors().keyColor);
		ofNoFill();
		ofSetLineWidth(2);
		ofDrawRectangle(vec2fWindow);
		ofPopStyle();
	}
}

void ofxTL2DVecTrack::loadColorPalette(ofBaseHasPixels& image){
	vec2fModalBackground.setFromPixels(image.getPixels());
	refreshAllSamples();
}

bool ofxTL2DVecTrack::loadColorPalette(string imagePath){
	if(vec2fModalBackground.load(imagePath)){
		palettePath = imagePath;
		refreshAllSamples();
		return true;
	}
	return false;
}

ofVec2f ofxTL2DVecTrack::getVec2f(){
	//play solo change
//	return getVec2fAtMillis(timeline->getCurrentTimeMillis());
	return getVec2fAtMillis(currentTrackTime());
}

ofVec2f ofxTL2DVecTrack::getVec2fAtSecond(float second){
	return getVec2fAtMillis(second*1000);
}

ofVec2f ofxTL2DVecTrack::getVec2fAtPosition(float pos){
	return getVec2fAtMillis(pos * timeline->getDurationInMilliseconds());
}

ofVec2f ofxTL2DVecTrack::getVec2fAtMillis(unsigned long long millis){
	if(keyframes.size() == 0){
		return defaultVec2f;
	}

	if(millis <= keyframes[0]->time){
		//cout << "getting color before first key " << ((ofxTLVec2fSample*)keyframes[0])->color << endl;
		return ((ofxTLVec2fSample*)keyframes[0])->vec2f;
	}

	if(millis >= keyframes[keyframes.size()-1]->time){
		return ((ofxTLVec2fSample*)keyframes[keyframes.size()-1])->vec2f;
	}

	for(int i = 1; i < keyframes.size(); i++){
		if(keyframes[i]->time >= millis){
			ofxTLVec2fSample* startSample = (ofxTLVec2fSample*)keyframes[i-1];
			ofxTLVec2fSample* endSample = (ofxTLVec2fSample*)keyframes[i];
			float interpolationPosition = ofMap(millis, startSample->time, endSample->time, 0.0, 1.0);
            return samplePaletteAtPosition(glm::mix(startSample->samplePoint,endSample->samplePoint, interpolationPosition));
		}
	}
	ofLogError("ofxTL2DVecTrack::getVec2fAtMillis") << "Could not find color for millis " << millis << endl;
	return defaultVec2f;
}

void ofxTL2DVecTrack::setDefaultVec2f(ofVec2f vec2f){
	defaultVec2f = vec2f;
}

ofVec2f ofxTL2DVecTrack::getDefaultVec2f(){
	return defaultVec2f;
}

string ofxTL2DVecTrack::getPalettePath(){
	if(palettePath == ""){
		ofLogWarning("ofxTL2DVecTrack::getPalettePath -- Palette is not loaded from path");
	}
	return palettePath;
}

//For selecting keyframe type only,
//the superclass controls keyframe placement
bool ofxTL2DVecTrack::mousePressed(ofMouseEventArgs& args, long millis){
	if(drawingVec2fWindow){
		clickedInVec2fRect = args.button == 0 && vec2fWindow.inside(args.x, args.y);
		if(clickedInVec2fRect){
			ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
			selectedSample->samplePoint = ofVec2f(ofMap(args.x, vec2fWindow.getX(), vec2fWindow.getMaxX(), 0, 1.0, true),
												  ofMap(args.y, vec2fWindow.getY(), vec2fWindow.getMaxY(), 0, 1.0, true));
			refreshSample(selectedSample);
			shouldRecomputePreviews = true;
		}
		else if(args.button == 0 && previousVec2fRect.inside(args.x, args.y)){
			ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
			selectedSample->samplePoint = samplePositionAtClickTime;
			refreshSample(selectedSample);
			clickedInVec2fRect = true; //keep the window open
			shouldRecomputePreviews = true;
		}

		return true;
	}
	else{
		return ofxTLKeyframes::mousePressed(args, millis);
	}
}

void ofxTL2DVecTrack::mouseDragged(ofMouseEventArgs& args, long millis){
	if(drawingVec2fWindow){
		if(clickedInVec2fRect){
			ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
			selectedSample->samplePoint = ofVec2f(ofMap(args.x, vec2fWindow.getX(), vec2fWindow.getMaxX(), 0, 1.0,true),
												  ofMap(args.y, vec2fWindow.getY(), vec2fWindow.getMaxY(), 0, 1.0,true));
			refreshSample(selectedSample);
			shouldRecomputePreviews = true;
		}
	}
	else{
		ofxTLKeyframes::mouseDragged(args, millis);
		if(keysDidDrag){
			shouldRecomputePreviews = true;
		}
	}
}

void ofxTL2DVecTrack::mouseReleased(ofMouseEventArgs& args, long millis){
	if(drawingVec2fWindow){
		//if(args.button == 0 && !vec2fWindow.inside(args.x, args.y) ){
		if(args.button == 0 && !clickedInVec2fRect && !ofGetModifierControlPressed()){
			ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
			if(selectedSample->vec2f != vec2fAtClickTime){
				timeline->flagTrackModified(this);
			}
			timeline->dismissedModalContent();
			drawingVec2fWindow = false;
			shouldRecomputePreviews = true;
		}
	}
	else{
		ofxTLKeyframes::mouseReleased(args, millis);
	}
}

void ofxTL2DVecTrack::keyPressed(ofKeyEventArgs& args){
	if(drawingVec2fWindow){
		if(args.key == OF_KEY_RETURN){
			ofxTLVec2fSample* selectedSample = (ofxTLVec2fSample*)selectedKeyframe;
			if(selectedSample->vec2f != vec2fAtClickTime){
				timeline->flagTrackModified(this);
			}
			timeline->dismissedModalContent();
			drawingVec2fWindow = false;
			shouldRecomputePreviews = true;
		}
	}
	else{
		ofxTLKeyframes::keyPressed(args);
	}
}

void ofxTL2DVecTrack::updatePreviewPalette(){

	if(previewPalette.getWidth() != bounds.width){
		previewPalette.allocate(bounds.width, 1, OF_IMAGE_COLOR); //someday support alpha would be rad
	}

	if(keyframes.size() == 0 || keyframes.size() == 1){
		return; //we just draw solid colors in this case
	}

	previewPalette.setUseTexture(false);
	for(int i = 0; i < bounds.width; i++){
		previewPalette.setColor(i, getVec2fAtMillis(screenXToMillis(bounds.x+i)).x, getVec2fAtMillis(screenXToMillis(bounds.x+i)).y );
	}
	previewPalette.setUseTexture(true);
	previewPalette.update();

	shouldRecomputePreviews = false;
}

ofxTLKeyframe* ofxTL2DVecTrack::newKeyframe(){
	ofxTLVec2fSample* sample = new ofxTLVec2fSample();
	sample->samplePoint = ofVec2f(.5,.5);
	sample->vec2f = defaultVec2f;
	//when creating a new keyframe select it and draw a color window
	vec2fAtClickTime = defaultVec2f;
	samplePositionAtClickTime = sample->samplePoint;
	drawingVec2fWindow = true;
	clickedInVec2fRect = true;
	//find surrounding points next drawModal cycle
	setNextAndPreviousOnUpdate = true;
	timeline->presentedModalContent(this);
	return sample;
}

void ofxTL2DVecTrack::restoreKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore){
	ofxTLVec2fSample* sample = (ofxTLVec2fSample*)key;

	sample->samplePoint = ofVec2f(xmlStore.getValue("sampleX", 0.0),
								  xmlStore.getValue("sampleY", 0.0));

	//for pasted keyframes cancel the color window
	drawingVec2fWindow = false;
	timeline->dismissedModalContent();
	refreshSample(sample);
}

void ofxTL2DVecTrack::storeKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore){
	ofxTLVec2fSample* sample = (ofxTLVec2fSample*)key;
	xmlStore.setValue("sampleX", sample->samplePoint.x);
	xmlStore.setValue("sampleY", sample->samplePoint.y);
}

void ofxTL2DVecTrack::regionSelected(ofLongRange timeRange, ofRange valueRange){
    for(int i = 0; i < keyframes.size(); i++){
    	if(timeRange.contains( keyframes[i]->time )){
            selectKeyframe(keyframes[i]);
        }
	}
}

void ofxTL2DVecTrack::selectedKeySecondaryClick(ofMouseEventArgs& args){

	drawingVec2fWindow = true;
	previousSample = NULL;
	nextSample = NULL;

	vec2fAtClickTime = ((ofxTLVec2fSample*)selectedKeyframe)->vec2f;
	samplePositionAtClickTime = ((ofxTLVec2fSample*)selectedKeyframe)->samplePoint;

	timeline->presentedModalContent(this);
	setNextAndPreviousSamples();
}

void ofxTL2DVecTrack::setNextAndPreviousSamples(){
	previousSample = nextSample = NULL;
	for(int i = 0; i < keyframes.size(); i++){
		if(keyframes[i] == selectedKeyframe){
			if(i > 0){
				previousSample = (ofxTLVec2fSample*)keyframes[i-1];
			}
			if(i < keyframes.size()-1){
				nextSample = (ofxTLVec2fSample*)keyframes[i+1];
			}
			break;
		}
	}
}


ofxTLKeyframe* ofxTL2DVecTrack::keyframeAtScreenpoint(ofVec2f p){
	if(isHovering()){
		for(int i = 0; i < keyframes.size(); i++){
			float offset = p.x - timeline->millisToScreenX(keyframes[i]->time);
			if (abs(offset) < 5) {
				return keyframes[i];
			}
		}
	}
	return NULL;
}

void ofxTL2DVecTrack::refreshAllSamples(){
	for(int i = 0; i < keyframes.size(); i++){
		refreshSample((ofxTLVec2fSample*)keyframes[i]);
	}
	shouldRecomputePreviews = true;
}

void ofxTL2DVecTrack::refreshSample(ofxTLVec2fSample* sample){
	sample->vec2f = samplePaletteAtPosition(sample->samplePoint);
}

//assumes normalized position
ofVec2f ofxTL2DVecTrack::samplePaletteAtPosition(glm::vec2 position){
	if(vec2fModalBackground.isAllocated()){
        ofVec2f positionPixelSpace = position * glm::vec2(vec2fModalBackground.getWidth(),vec2fModalBackground.getHeight());
		//bilinear interpolation from http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/bilinear-interpolation-of-texture-maps-r810
//		int x0 = int(positionPixelSpace.x);
//		int y0 = int(positionPixelSpace.y);
//		float dx = positionPixelSpace.x-x0, dy = positionPixelSpace.y-y0, omdx = 1-dx, omdy = 1-dy;
//		return vec2fModalBackground.getPixels().getColor(x0,y0)*omdx*omdy +
//		       vec2fModalBackground.getPixels().getColor(x0,MIN(y0+1, vec2fModalBackground.getHeight()-1))*omdx*dy +
//		       vec2fModalBackground.getPixels().getColor(MIN(x0+1,vec2fModalBackground.getWidth()-1),y0)*dx*omdy +
//		       vec2fModalBackground.getPixels().getColor(MIN(x0+1,vec2fModalBackground.getWidth()-1),MIN(y0+1, vec2fModalBackground.getHeight()-1))*dx*dy;
        return positionPixelSpace;
	}
	else{
		ofLogError("ofxTL2DVecTrack::refreshSample -- sampling palette is null");
		return defaultVec2f;
	}
}

string ofxTL2DVecTrack::getTrackType(){
	return "2DVec";
}
