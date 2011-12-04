/*
 *  THISTimeline.cpp
 *  THIS_Editor
 *
 *  Created by Jim on 9/23/10.
 *  Copyright 2010 FlightPhase. All rights reserved.
 *
 */

#include "ofxTimeline.h"

bool headersort(ofxTLElementHeader* a, ofxTLElementHeader* b){
	return a->getDrawRect().y < b->getDrawRect().y;
}

#define TICKER_HEIGHT 30
#define ZOOMER_HEIGHT 20


ofxTimeline::ofxTimeline()
:	mouseoverPlayheadPosition(0),
	playbackPlayheadPosition(0),
	width(1024),
	offset(ofVec2f(0,0)),
	modalIsShown(false),
	autosave(true),
	isFrameBased(false),
	durationInFrames(100),
	durationInSeconds(100.0f/30.0f),
	isShowing(true),
	filenamePrefix("defaultTimeline_"),
	isSetup(false),
	usingEvents(false),
	isPlaying(false)
{
}

ofxTimeline::~ofxTimeline(){
	if(isSetup){
		if(isPlaying){
			stop();
		}
		
		for(int i = 0; i < pages.size(); i++){ 
			delete pages[i];
		}
		pages.clear();
		
		delete ticker;
		delete tabs;
		delete zoomer;
		
		ofRemoveListener(ofxTLEvents.viewNeedsResize, this, &ofxTimeline::viewNeedsResize);
		ofRemoveListener(ofxTLEvents.pageChanged, this, &ofxTimeline::pageChanged);
		
	}
}

void ofxTimeline::setup(){

	isSetup = true;
	
	width = ofGetWidth();

	tabs = new ofxTLPageTabs();
	tabs->setTimeline(this);
	tabs->setup();
	tabs->setDrawRect(ofRectangle(0, 0, width, TICKER_HEIGHT));

	ticker = new ofxTLTicker();
	ticker->setTimeline(this);
	ticker->setup();
	ticker->setDrawRect(ofRectangle(0, TICKER_HEIGHT, width, TICKER_HEIGHT));
	
	zoomer = new ofxTLZoomer();
	zoomer->setTimeline(this);
	zoomer->setXMLFileName(filenamePrefix + "_zoomer.xml");
	zoomer->setup();
	zoomer->setDrawRect(ofRectangle(0, TICKER_HEIGHT*2, width, ZOOMER_HEIGHT));
	
	colors.loadColors();
	
	enable();
	
	ofAddListener(ofxTLEvents.viewNeedsResize, this, &ofxTimeline::viewNeedsResize);
	ofAddListener(ofxTLEvents.pageChanged, this, &ofxTimeline::pageChanged);

	addPage("_defaultPage", true);
	
}

#pragma mark CONFIGURATION
void ofxTimeline::show(){
	isShowing = true;
}
	
void ofxTimeline::hide(){
	isShowing = false;
}

bool ofxTimeline::toggleShow(){
	isShowing = !isShowing;
	return isShowing;
}

ofxTLColors& ofxTimeline::getColors(){
	return colors;
}

void ofxTimeline::play(){
	if(!isPlaying){
		ofAddListener(ofEvents.update, this, &ofxTimeline::update);
		isPlaying = true;
		if (isFrameBased) {
			playbackStartFrame = ofGetFrameNum();
		}
		else{
			playbackStartTime = ofGetElapsedTimef();
		}
	}
}

void ofxTimeline::stop(){
	if(isPlaying){
		isPlaying = false;
		ofRemoveListener(ofEvents.update, this, &ofxTimeline::update);
	}
}

bool ofxTimeline::togglePlay(){
	if(isPlaying){
		stop();
	}
	else{
		play();
	}
	return isPlaying;
}

bool ofxTimeline::getIsPlaying(){
	return isPlaying;
}

void ofxTimeline::setCurrentFrame(int newFrame){
	if(!isFrameBased){
		ofLogWarning("ofxTimeline -- setting current frame on a timebased timline has no effect.");
	}
	currentFrame = newFrame;
}

void ofxTimeline::setCurrentTime(float time){
	if(isFrameBased){
		ofLogWarning("ofxTimeline -- setting current time on a framebased timeline has no effect.");
	}
	currentTime = time;
}

bool ofxTimeline::getIsFrameBased(){
	return isFrameBased;
}

int ofxTimeline::getCurrentFrame(){
	if(!isFrameBased){
		ofLogWarning("ofxTimeline -- getting current frame on a timebased timline will return inaccurate results.");
	}
	return currentFrame;
}

float ofxTimeline::getCurrentTime(){
	if(isFrameBased){
		ofLogWarning("ofxTimeline -- getting current time on a framebased will return inaccurate results.");
	}
	return currentTime;
}

bool ofxTimeline::toggleEnabled(){
	isEnabled = !isEnabled;
	return isEnabled;
}

void ofxTimeline::enable(){
	isEnabled = true;
	enableEvents();
}

void ofxTimeline::disable(){
	isEnabled = false;
	disableEvents();
}

void ofxTimeline::setDurationInFrames(int frames){
	durationInFrames = frames;
	isFrameBased = true;
}

void ofxTimeline::setDurationInSeconds(float seconds){
	durationInSeconds = seconds;
	isFrameBased = false;
}

int ofxTimeline::getDurationInFrames(){
	if(!isFrameBased){
		ofLogWarning("ofxTimeline -- getting duration in frames on a timebased timline will return inaccurate results.");
	}
	return durationInFrames;
}

float ofxTimeline::getDurationInSeconds(){
	if(isFrameBased){
		ofLogWarning("ofxTimeline -- getting duration in time on a framebased will return inaccurate results.");
	}
	return durationInSeconds;
}

void ofxTimeline::setAutosave(bool doAutosave){
	autosave = doAutosave;
}

void ofxTimeline::setOffset(ofVec2f newOffset){
	offset = newOffset;
	recalculateBoundingRects();
	updatePagePositions();
}

void ofxTimeline::setWidth(float newWidth){
	width = newWidth;
	updatePagePositions();
}

void ofxTimeline::updatePagePositions(){
	if(isSetup){
		ofVec2f pageOffset = ofVec2f(0, ticker->getDrawRect().y+ticker->getDrawRect().height);
		for(int i = 0; i < pages.size(); i++){
			pages[i]->setContainer(pageOffset, width);
		}
		currentPage->recalculateHeight();
	}
}

#pragma mark EVENTS
void ofxTimeline::enableEvents() {
	if (!usingEvents) {
		ofAddListener(ofEvents.mouseMoved, this, &ofxTimeline::mouseMoved);
		ofAddListener(ofEvents.mousePressed, this, &ofxTimeline::mousePressed);
		ofAddListener(ofEvents.mouseReleased, this, &ofxTimeline::mouseReleased);
		ofAddListener(ofEvents.mouseDragged, this, &ofxTimeline::mouseDragged);
		
		ofAddListener(ofEvents.keyPressed, this, &ofxTimeline::keyPressed);
		ofAddListener(ofEvents.windowResized, this, &ofxTimeline::windowResized);
		
		usingEvents = true;
	}
}

void ofxTimeline::disableEvents() {
	if (usingEvents) {
		ofRemoveListener(ofEvents.mouseMoved, this, &ofxTimeline::mouseMoved);
		ofRemoveListener(ofEvents.mousePressed, this, &ofxTimeline::mousePressed);
		ofRemoveListener(ofEvents.mouseReleased, this, &ofxTimeline::mouseReleased);
		ofRemoveListener(ofEvents.mouseDragged, this, &ofxTimeline::mouseDragged);
		
		ofRemoveListener(ofEvents.keyPressed, this, &ofxTimeline::keyPressed);
		ofRemoveListener(ofEvents.windowResized, this, &ofxTimeline::windowResized);
		
		usingEvents = false;
	}
}

void ofxTimeline::mousePressed(ofMouseEventArgs& args){
	currentPage->mousePressed(args);
	zoomer->mousePressed(args);
}

void ofxTimeline::mouseMoved(ofMouseEventArgs& args){
	ticker->mouseMoved(args);
	currentPage->mouseMoved(args);
	zoomer->mouseMoved(args);
}

void ofxTimeline::mouseDragged(ofMouseEventArgs& args){
	ticker->mouseDragged(args);
	currentPage->mouseDragged(args);
	zoomer->mouseDragged(args);
}

void ofxTimeline::mouseReleased(ofMouseEventArgs& args){
	tabs->mouseReleased(args);
	currentPage->mouseReleased(args);
	zoomer->mouseReleased(args);
}

void ofxTimeline::keyPressed(ofKeyEventArgs& args){
	currentPage->keyPressed(args);
	zoomer->keyPressed(args);
}

void ofxTimeline::windowResized(ofResizeEventArgs& args){
	recalculateBoundingRects();
}

#pragma mark DRAWING
void ofxTimeline::viewNeedsResize(ofEventArgs& args){
	recalculateBoundingRects();
}

void ofxTimeline::recalculateBoundingRects(){
	
	tabs->setDrawRect( ofRectangle(offset.x, offset.y, width, TICKER_HEIGHT) );
	ticker->setDrawRect( ofRectangle(offset.x, offset.y+TICKER_HEIGHT, width, TICKER_HEIGHT) );
	updatePagePositions();

	zoomer->setDrawRect(ofRectangle(offset.x, offset.y+currentPage->getComputedHeight()+TICKER_HEIGHT*2, width, ZOOMER_HEIGHT));
	ofRectangle totalDrawRect = ofRectangle(offset.x, offset.y+TICKER_HEIGHT*2,
											width,currentPage->getComputedHeight()+ZOOMER_HEIGHT);
	ticker->setTotalDrawRect(totalDrawRect);	
}


void ofxTimeline::pageChanged(ofxTLPageEventArgs& args){
	for(int i = 0; i < pages.size(); i++){
		if(pages[i]->getName() == args.currentPageName){
			currentPage = pages[i];
			recalculateBoundingRects();
			return;
		}
	}
	
	ofLogError("ofxTimeline -- Tabbed to nonexistence page " + args.currentPageName);
}

bool ofxTimeline::setLoopType(ofLoopType newType){
	loopType = newType;
}

void ofxTimeline::update(ofEventArgs& updateArgs){
	if(isFrameBased){
		currentFrame = ofGetFrameNum() - playbackStartFrame;
		if(currentFrame >= durationInFrames){
			if(loopType == OF_LOOP_NONE){
				currentFrame = durationInFrames;
				stop();
			}
			else if(loopType == OF_LOOP_NORMAL) {
				currentFrame %= durationInFrames;
			}
		}
	}
	else{
		currentTime = ofGetElapsedTimef() - playbackStartTime;
		if(currentTime >= durationInSeconds){
			if(loopType == OF_LOOP_NONE){
				currentTime = durationInSeconds;
				stop();
			}
			else if(loopType == OF_LOOP_NORMAL) {
				currentTime = fmod(currentTime, durationInSeconds);
			}
		}
	}
}

void ofxTimeline::draw(){	
	if(isShowing){
		tabs->draw();
		currentPage->draw();
		zoomer->draw();
		ticker->draw();
	}
}

#pragma mark ELEMENT CREATORS/GETTERS/SETTERS
void ofxTimeline::addPage(string name, bool makeCurrent){
	if(name == ""){
		ofLogError("ofxTimeline -- Cannot add page with an empty name.");
		return;
	}
	
	for(int i = 0; i < pages.size(); i++){
		if(name == pages[i]->getName()){
			ofLogError("ofxTimeline -- Page " + name + " already exists");
			return;
		}
	}
	
	ofxTLPage* newPage = new ofxTLPage();
	newPage->setName(name);
	newPage->setup();
	newPage->setAutosave(autosave);
	tabs->addPage(name);
	
	pages.push_back(newPage);
	if(makeCurrent){
		tabs->selectPage(name);
	}
}

void ofxTimeline::setCurrentPage(string name){
	for(int i = 0; i < pages.size(); i++){
		if(name == pages[i]->getName()){
			currentPage = pages[i];
			return;
		}
	}
	ofLogError("ofxTimeline -- Page " + name + " not found");
}

void ofxTimeline::setCurrentPage(int index){
	if(index >= pages.size()){
		ofLogError("ofxTimeline -- Page at index " + ofToString(index) + " does not exist");
		return;
	}
	currentPage = pages[index];
	
}

//can be used to add custom elements
void ofxTimeline::addElement(string name, ofxTLElement* element){
	element->setTimeline( this );
	currentPage->addElement(name, element);		
	elementNameToPage[name] = currentPage;
	recalculateBoundingRects();
}

ofxTLKeyframer* ofxTimeline::addKeyframes(string name, string xmlFileName, ofRange valueRange){

	ofxTLKeyframer*	newKeyframer = new ofxTLKeyframer();
	newKeyframer->setCreatedByTimeline(true);
	newKeyframer->setValueRange(valueRange);
	newKeyframer->setXMLFileName(xmlFileName);
	addElement(name, newKeyframer);
	
	return newKeyframer;
}

float ofxTimeline::getKeyframeValue(string name){
	if(isFrameBased){
		return getKeyframeValue(name, currentFrame);
	}
	else {
		return getKeyframeValue(name, currentTime);
	}
}

float ofxTimeline::getKeyframeValue(string name, float atTime){
	ofxTLKeyframer* keyframer = (ofxTLKeyframer*)elementNameToPage[name]->getElement(name);
	if(keyframer == NULL){
		ofLogError("ofxTimeline -- Couldn't find element " + name);
		return 0.0;
	}
	return keyframer->getValueAtPercent(atTime/durationInSeconds);
}

float ofxTimeline::getKeyframeValue(string name, int atFrame){
	ofxTLKeyframer* keyframer = (ofxTLKeyframer*)elementNameToPage[name]->getElement(name);
	if(keyframer == NULL){
		ofLogError("ofxTimeline -- Couldn't find element " + name);
		return 0.0;
	}
	return keyframer->getValueAtPercent(1.0*currentFrame/durationInFrames);
}


ofxTLSwitcher* ofxTimeline::addSwitcher(string name, string xmlFileName){
}

bool ofxTimeline::getSwitcherOn(string name){
}

bool ofxTimeline::getSwitcherOn(string name, float atTime){
}

bool ofxTimeline::getSwitcherOn(string name, int atFrame){
}


ofxTLTrigger* ofxTimeline::addTriggers(string name, string xmlFileName){
	ofxTLTrigger* newTrigger = new ofxTLTrigger();
	newTrigger->setCreatedByTimeline(true);
	newTrigger->setXMLFileName(xmlFileName);
	addElement(name, newTrigger);

	return newTrigger;
}

string ofxTimeline::getNextTrigger(string name){
}

string ofxTimeline::getNextTrigger(string name, float atTime){
}

string ofxTimeline::getNextTrigger(string name, int atFrame){
}


string ofxTimeline::getLastTrigger(string name){
}

string ofxTimeline::getLastTrigger(string name, float atTime){
}

string ofxTimeline::getLastTrigger(string name, int atFrame){
}

ofxTLImageSequence* ofxTimeline::addImageSequence(string name){
	ofFileDialogResult result = ofSystemLoadDialog("Load Sequence", true);
	if(result.bSuccess && ofDirectory::doesDirectoryExist(result.filePath, false)){
		addImageSequence(name, result.getPath());
	}
}


ofxTLImageSequence* ofxTimeline::addImageSequence(string name, string directory){
	ofxTLImageSequence*	newImageSequence = new ofxTLImageSequence();
	newImageSequence->setCreatedByTimeline(true);
	newImageSequence->loadSequence(directory);
	addElement(name, newImageSequence);
	return newImageSequence;	
}

ofImage* ofxTimeline::getImage(string name){
}

ofImage* ofxTimeline::getImage(string name, float atTime){
}

ofImage* ofxTimeline::getImage(string name, int atFrame){
}

