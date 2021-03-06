#include "MainScene.h"

CCScene* MainScene::scene()
{	
	CCScene *ms = (CCScene*) MainScene::create();
	return ms;
}

MainScene::MainScene()
{
	this->debugLayer = NULL;
	this->boxWorld = NULL;	
	this->weather = NULL;
	this->worldLayer = NULL;
	this->gamePlayer = NULL;
	this->prevPlayerPos = ccp(0, 0);
	
	this->worldListener = NULL;
	this->disableKeyboard = false;
	this->touchesInProgress = false;
	this->cameraMoveInProgress = false;		
	this->zoomInProgress = false;
	this->statsLapse = 0;
	this->stats = NULL;
	this->shinyConsole = NULL;
	this->audio = NULL;
	
	this->batchManager = NULL;
}

MainScene::~MainScene()
{
	CCLog("MainScene destructor called");
	this->unscheduleAllSelectors();
	this->removeAllChildrenWithCleanup(true);

	//	release all physics references created in world
	b2Body* list = boxWorld->GetBodyList();
	while (list)
	{
		GameEntity* e = (GameEntity*) list->GetUserData();		
		CC_SAFE_RELEASE(e);
		
		list = list->GetNext();
	}
	
	CC_SAFE_DELETE(worldListener);
	CC_SAFE_DELETE(boxWorld);
	CC_SAFE_DELETE(weather);
	CC_SAFE_DELETE(batchManager);	
	
	LevelProperties::purge();
	KeyboardHelper::purge();
	ShapeHelper::purge();		
}

bool MainScene::init()
{
	PROFILE_FUNC();

	CCLayer::init();
	winSize = CCDirector::sharedDirector()->getWinSizeInPixels();
	
	//	Create world layer
	this->worldLayer = CCLayer::create();
	this->worldLayer->setTag(WORLD_NODE_ID);

	sceneScale = DEFAULT_SCALE;
	this->worldLayer->setScale(sceneScale);
	this->addChild(worldLayer, 1000);

	CCDirector::sharedDirector()->setDisplayStats(false);
	this->audio = NewAudioSystem::shared();

	//	add loading layer
	this->loadLayer = CCSprite::create(RESOURCE_LOADING);
	loadLayer->setPosition(ccp(winSize.width / 2, winSize.height / 2));
	this->addChild(loadLayer, 10000);
	
	//	load map after 1 second
	//this->scheduleOnce(schedule_selector(MainScene::loadMap), 0.0f);
	loadMap(0);

	return true;
}

void MainScene::loadMap(float none)
{
	PROFILE_FUNC();

	//	setup physics object 
	this->setupPhysics();		
	bool loaded = this->loadLevel();	//	<-- load level!!!
	if (!loaded)
	{
		CCLog("Could not load level %s", __FUNCTION__);
		return;
	}
	
	//level settings
	LevelProperties* lp = LevelProperties::sharedProperties();
	if (!lp)
	{
		CCLog("Could not load LevelProperties %s", __FUNCTION__);
		return;
	}

	if (lp->CameraZoom )
		this->worldLayer->setScale(lp->CameraZoom);	

	//	weather data
	if (lp && lp->WeatherActive)
		this->weather = WeatherHelper::create(this, this->worldLayer, WEATHER_CONTROLLER_DATA);	

	//	add shift sprite
	this->shiftSprite = CCSprite::create(RESOURCE_SHIFT);
	this->shiftSprite->setAnchorPoint(ccp(0, 0));
	this->shiftSprite->setPosition(ccp(10, 30));
	this->shiftSprite->setVisible(false);
	this->addChild(this->shiftSprite, 100000);		

	//	schedule camera update
	if (this->gamePlayer)
	{		
		CCPoint playerStart = this->worldLayer->convertToWorldSpace(this->gamePlayer->getSkin()->getPosition());

		//	always start in (SCREEN_MARGIN/SCREEN_MARGIN) coordinates
		float twentyPercent = SCREEN_MARGIN_BOTTOM;
		float xs = ( playerStart.x - (winSize.width * twentyPercent) ) * -1;
		float ys = ( playerStart.y - (winSize.height * twentyPercent) ) * -1;	
		this->worldLayer->setPosition(this->getPositionX() + xs, this->getPositionY() + ys);
	}


#if defined(_DEBUG) && !defined(DISABLE_SHINY)
	PROFILER_UPDATE();
	PROFILER_OUTPUT("profiler_load.txt");
#endif
	
	//	schedule controls
	drawDebugControls();

	//	remove loading layer
	this->loadLayer->runAction(CCFadeTo::create(1.0f, 0));

	//	now do UPDATE schedule	
	//((CCLayer*) this)->setTouchEnabled(true);
	this->setTouchEnabled(true);
	//this->schedule(schedule_selector(MainScene::updateKeyboard));
	this->scheduleUpdate();


	//	now run level script
}




void MainScene::drawDebugControls()
{
#ifdef DISABLE_CONTROLS_TEXT
	return;
#endif
	
	char* message = "[F1] Toggle debug modes\r\n[F4] Restart game\r\n[F7] Zoom in\r\n[F8] Zoom out\r\n[F9] Reset camera\r\n[F12] Exit";

	int zOrder = 100000;
	CCLabelTTF* lab = CCLabelTTF::create(message, "Consolas", 12.0f);
	lab->setAnchorPoint(ccp(0, 1));
	lab->setHorizontalAlignment(CCTextAlignment::kCCTextAlignmentLeft);
	lab->setPosition(ccp(10, CCDirector::sharedDirector()->getWinSizeInPixels().height - 15));
	this->addChild(lab, zOrder);

	//	make background
	CCLayerColor *labOut = CCLayerColor::create(ccc4(0, 0, 0,  100));
	labOut->ignoreAnchorPointForPosition(false);
	labOut->setAnchorPoint(ccp(0, 1));

	//	calculate size
	float extend = 8;
	CCSize labOutS = lab->getContentSize();
	labOutS.width += 6;
	labOutS.height += extend;
	labOut->setContentSize(labOutS);
	CCPoint labOutPos = lab->getPosition();
	labOutPos.x -= 3;
	labOutPos.y += 3;
	labOut->setPosition(labOutPos);
	this->addChild(labOut, zOrder - 1);

	/*
	labOutS.height -= extend;
	stats = CCLabelTTF::create("FPS:", "Consolas", 12.0f);

	labOutPos.x = lab->getPositionX();
	labOutPos.y -= labOutS.height + 10;
	stats->setPosition(labOutPos);
	stats->setAnchorPoint(ccp(0, 1));
	this->addChild(stats, zOrder);
	*/

#ifndef DISABLE_SHINY

	PROFILER_UPDATE();
	std::string shinyOut = PROFILER_OUTPUT_TREE_STRING();

	shinyConsole = CCLabelTTF::create(shinyOut.c_str(), "Consolas", 12);
	
	CCPoint pos = lab->getPosition();
	pos.x = winSize.width - 10;

	shinyConsole->setPosition(pos);
	shinyConsole->setAnchorPoint(ccp(1, 1));

	this->addChild(shinyConsole, zOrder);

	CCSize s = shinyConsole->getContentSize();
	
	shinyConsoleBackground = CCLayerColor::create(ccc4(0, 0, 0, 100));
	shinyConsoleBackground->ignoreAnchorPointForPosition(false);
	shinyConsoleBackground->setContentSize(s);
	shinyConsoleBackground->setPosition(pos);
	shinyConsoleBackground->setAnchorPoint(shinyConsole->getAnchorPoint());
	this->addChild(shinyConsoleBackground, zOrder - 1);

	shinyConsole->setVisible(false);
	shinyConsoleBackground->setVisible(false);
	
	Shiny::ProfileManager::instance.clear();	//	clear prev data	

#endif
}

void MainScene::setupPhysics()
{
	PROFILE_FUNC();

	//	setup Physics
	const b2Vec2 gravity(0.0f, BOX_WORLD_GRAVITY);
	this->boxWorldSleep = true;

	this->boxWorld = new b2World(gravity);
	this->boxWorld->SetAllowSleeping(this->boxWorldSleep);

	this->worldListener = new ContactListener(this);
	this->boxWorld->SetContactListener(this->worldListener);
	
	//	setup debug drawing	
	if (true)
	{
		this->debugLayer = B2DebugDrawLayer::create(this->boxWorld, PTM_RATIO);
		this->debugLayer->setVisible(false);
		this->worldLayer->addChild(this->debugLayer, 9999);	
	}
}

bool MainScene::loadLevel()
{	
	PROFILE_FUNC();
	
	//	load level
	char *level = GAME_START_LEVEL;
	extern char* levelOverride;
	
	if (levelOverride && strlen(levelOverride))
		level = levelOverride;
	
	batchManager = new BatchManager();
	LevelLoader l(this->worldLayer, level, this->boxWorld, this->batchManager);	
	bool loaded = l.parse();

	if (loaded)
	{
		gamePlayer = l.player;

		//	set rotation and friction damping and gravity scale
		this->gamePlayer->getBody()->SetFixedRotation(true);
		this->gamePlayer->getBody()->SetLinearDamping(0.5f);
		this->gamePlayer->getBody()->SetGravityScale(2);

		this->runLevelScript(level);

		/*CCRect r = LevelProperties::sharedProperties()->SceneSize;
		CCLayerColor* l = CCLayerColor::create(ccc4(255, 255, 255, 255));
		l->ignoreAnchorPointForPosition(false);
		l->setAnchorPoint(ccp(0, 0));
		l->setPosition(r.origin);
		l->setContentSize(r.size);
		this->worldLayer->addChild(l);
		*/
	}
	else
	{
		#ifdef CC_PLATFORM_WIN32
			MessageBox(NULL, "Player object is not found in this level.", "How are you gona play?", MB_ICONWARNING | MB_OK);
		#endif
	}
	
	return loaded;
}

void MainScene::runLevelScript(std::string level)
{
	PROFILE_FUNC();

#ifdef ENABLE_SCRIPTING
	ScriptingCore* monkey = ScriptingCore::getInstance();	
	if (monkey)
	{
		std::string tmp = level.substr(0, level.find_last_of("."));
		tmp += ".js";

		CCLog("Looking for script: %s", tmp.c_str());

		if (doesFileExits((char*) tmp.c_str()))
		{
			CCLog("Found it! Running script!\r\n");
			monkey->runScript(tmp.c_str());
		}
	}
#endif
}

void MainScene::toggleCameraProgress()
{
	this->cameraMoveInProgress = !this->cameraMoveInProgress;
}

void MainScene::updateCamera(float delta)
{		
	PROFILE_FUNC();

	//CCLog("DELTA CAMERA: %f", delta);
	if (this->gamePlayer == NULL || this->touchesInProgress  || !this->worldLayer)
		return;
	
	CCPoint realPos = this->worldLayer->convertToWorldSpace(this->gamePlayer->getSkin()->getPosition());	
	CCPoint prevPos = this->worldLayer->getPosition();

	CCSize winSize = CCDirector::sharedDirector()->getWinSizeInPixels();	

	float rightMargin = winSize.width - winSize.width * SCREEN_MARGIN_FRONT;
	float leftMargin = winSize.width * SCREEN_MARGIN_LEFT;
	float modifier = ((delta - 1/60.0f));
	float timeDelay = 1;

	float topMargin = winSize.height - winSize.height * SCREEN_MARGIN_TOP;
	float bottomMargin = winSize.height * SCREEN_MARGIN_BOTTOM;

	float xm = prevPos.x;
	float ym = prevPos.y;

	//	check for X axis
	if (realPos.x >= rightMargin)
		xm -= realPos.x - rightMargin;
	else if (realPos.x <= leftMargin)
		xm += leftMargin - realPos.x;

	//	Set X axis independently	
	this->worldLayer->setPosition(xm, ym);

	//	check for Y axis (only if player is not in air)
	if ( !this->gamePlayer->isPlayerMidAir() && !cameraMoveInProgress )
	{
		float lastYM = ym;

		if (realPos.y <= bottomMargin)
			ym += bottomMargin - realPos.y;
		else if (realPos.y >= topMargin)
			ym -= realPos.y - topMargin;

		//	If needed ajdust position on Y axis animated 
		if (ym != lastYM)
		{
			this->cameraMoveInProgress = true;

			CCMoveTo* m1 = CCMoveTo::create(ZOOM_TIME, ccp(xm, ym));
			CCCallFunc* f1 = CCCallFunc::create(this, callfunc_selector(MainScene::toggleCameraProgress));

			CCSequence* seq = CCSequence::createWithTwoActions(m1, f1);
			this->worldLayer->runAction(seq);
		}
	}

	
}

void MainScene::updateKeyboard(float delta)
{
	PROFILE_FUNC();

	if (disableKeyboard)
		return;

	KeyboardHelper *key = KeyboardHelper::sharedHelper();
	if (key->getF1() == KeyStateDown)
	{
#ifndef DISABLE_SHINY		
		//	hide if both shown
		if (debugLayer->isVisible() && shinyConsole->isVisible())
		{
			debugLayer->setVisible(false);
			shinyConsole->setVisible(false);
			shinyConsoleBackground->setVisible(false);
			CCDirector::sharedDirector()->setDisplayStats(false);
		}
		//	if shiny visible show debug next
		else if (!debugLayer->isVisible() && shinyConsole->isVisible())		
			debugLayer->setVisible(true);
		//	if nothing shown show shiny
		else if (!shinyConsole->isVisible())
		{
			shinyConsoleBackground->setVisible(true);
			shinyConsole->setVisible(true);
			CCDirector::sharedDirector()->setDisplayStats(true);
		}
		
#else
		debugLayer->setVisible(!debugLayer->isVisible());
#endif

	}
	else if (key->getF4() == KeyStateDown)	
		playerDied();
	else if (key->getF7() == KeyStateDown)
		incSceneZoom();
	else if (key->getF8() == KeyStateDown)
		descSceneZoom();
	else if (key->getF9() == KeyStateDown)
	{
		if (touchesInProgress)
		{
			touchesInProgress = false;
		}
		else
			resetSceneZoom();
	}
	
	if (key->getF12() == KeyStateDown)
	{
		exit(0);
	}

	//	shift key sprite
	if (key->getShift() == KeyStateDown)
	{
		if (!this->shiftSprite->isVisible())
			this->shiftSprite->setVisible(true);
	}
	else if (this->shiftSprite->isVisible())
		this->shiftSprite->setVisible(false);
}

void MainScene::updateFPS(float delta)
{
#ifndef DISABLE_SHINY

	PROFILE_FUNC(); //- unimportant


	if (statsLapse == 0)
	{		
		statsLapse += delta;

		//	update Shiny data if debug layer ON
		if (this->shinyConsole && this->shinyConsole->isVisible())
		{
			PROFILER_UPDATE();
			std::string out = PROFILER_OUTPUT_TREE_STRING();
			shinyConsole->setString(out.c_str());
			Shiny::ProfileManager::instance.clear();

			//	resize console if needed
			CCSize sc = shinyConsole->getContentSize();
			CCSize scb = shinyConsoleBackground->getContentSize();
			if (sc.width != scb.width || sc.height != scb.height)
				shinyConsoleBackground->setContentSize(sc);
		}

	}//	if time is right
	else if (statsLapse >= 1.0f) //update every half second	
		statsLapse = 0;	
	else
		statsLapse += delta;

#endif
}

void MainScene::update(float delta)
{	
	PROFILE_FUNC();

	//	stats update
	updateFPS(delta);
	
	if (audio)
		audio->update();

	//	Keyboard update
	updateKeyboard(delta);	

	if (this->gamePlayer)
	{
		if (this->gamePlayer->checkForDeath())
		{
			playerDied();
			return;	//	stop update
		}		
		gamePlayer->updatePlayerMovement(delta);
	}

	//	PHYSICS UPDATE
	updatePhysics(delta);

#ifndef DISABLE_CAMERA				
	//	Update camera here
	updateCamera(delta);
#endif	

	//	And weather ofcourse
	if (weather)
		weather->update(delta);


#ifdef CC_PLATFORM_WIN32
	//	hide cursor for app
	extern bool fullScreen;
	if (fullScreen)
		SetCursor(NULL);
#endif
}

void MainScene::updatePhysics(float delta)
{
	PROFILE_FUNC();

	this->boxWorld->Step(BOX_WOLRD_STEP, BOX_WORLD_VELOCITY_PASSES, BOX_WORLD_POSITION_PASSES);

	b2Body* b = this->boxWorld->GetBodyList();
	while (b)
	{
		GameEntity *userData = (GameEntity*) b->GetUserData();
		if (userData)
		{
			if (!userData->isForRemoval())
			{
				userData->updatePosition(b->GetPosition());
				userData->updateRotation(b->GetAngle());
				b = b->GetNext();
			}
			else
			{
				//	remove from world
				b2Body* b2 = b->GetNext();
				this->boxWorld->DestroyBody(b);
				userData->bodyRemovedFromWorld();	//	notify object that its body was removed
				b = b2;
			}
		}
	}
}

void MainScene::ccTouchesBegan(CCSet* touches, CCEvent* event)
{
	PROFILE_FUNC();
	CCLog("Touches began");	
	//this->touchesInProgress = true;
}

void MainScene::ccTouchesEnded(CCSet* touches, CCEvent* event)
{
	PROFILE_FUNC();
	CCLog("Touches ended");
	//this->touchesInProgress = false;	
}

void MainScene::ccTouchesMoved(CCSet* touches, CCEvent* event)
{
#ifndef NO_MOUSE_MOVE

	PROFILE_FUNC();

	CCLog("Touch moved");
	this->touchesInProgress = true;	

	CCTouch *t = (CCTouch*) touches->anyObject();
	if (!t)
		return;
	
	CCPoint loc = t->getLocationInView();
	CCPoint locPrev = t->getPreviousLocationInView();

	CCPoint diff = ccpSub(loc, locPrev);
	diff.y *= -1;
	this->worldLayer->setPosition(ccpAdd(this->worldLayer->getPosition(), diff));

#endif
}

void MainScene::setSceneZoom(float newScale)
{		
	PROFILE_FUNC();

	if (!gamePlayer || !worldLayer)
		return;

	if (zoomInProgress)
		return;
	
	zoomInProgress = true;
	
	float currScale = worldLayer->getScale();

	CCScaleTo* scale = CCScaleTo::create(ZOOM_TIME, newScale);
	CCCallFunc* reset = CCCallFunc::create(this, callfunc_selector(MainScene::resetZoomInProgress));

	float diffScale = newScale - currScale;

	//	Move to correct position
	CCPoint pos = worldLayer->convertToWorldSpace(gamePlayer->getSkin()->getPosition());	
	worldLayer->setScale(newScale);
	
	CCPoint posAfterScale = worldLayer->convertToWorldSpace(gamePlayer->getSkin()->getPosition());	
	worldLayer->setScale(currScale);
	
	CCPoint dif = ccpSub(posAfterScale, pos);
	CCPoint difWorld = ccpSub(worldLayer->getPosition(), dif);
	worldLayer->runAction(CCMoveTo::create(ZOOM_TIME, difWorld));

	prevPlayerPos = pos;
	CCLog("POS: x: %f y: %f", pos.x, pos.y);

	CCSequence *s = CCSequence::createWithTwoActions(scale, reset);
	worldLayer->runAction(s);

	this->sceneScale = newScale;
}

void MainScene::resetZoomInProgress()
{
	zoomInProgress = false;
}

void MainScene::incSceneZoom()
{
	float newScale = min(2.0f, this->sceneScale + ZOOM_STEP);
	setSceneZoom(newScale);
}

void MainScene::descSceneZoom()
{
	float newScale = max(0.005f, this->sceneScale - ZOOM_STEP);
	setSceneZoom(newScale);
}

void MainScene::resetSceneZoom()
{	
	setSceneZoom(DEFAULT_SCALE);
}

void MainScene::playerDied()
{
	PROFILE_FUNC();
	CCDirector::sharedDirector()->replaceScene(MainScene::scene());			
}

