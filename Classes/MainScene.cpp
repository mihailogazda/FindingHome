#include "MainScene.h"

short backgroundAlphaStart = 80;
short backgroundAlphaEnd = 255;

short backgroundMatrix[][3] = 
{ 	
	{134, 193, 253},	//day
	{255, 201, 14},		//sunset
	{80, 80, 180}		//night
};

CCScene* MainScene::scene()
{
	CCScene* scene = CCScene::create();	

	MainScene *ms = MainScene::create();
	if (ms)
		scene->addChild(ms);

	return scene;
}

bool MainScene::init()
{	
	lastUsedBackgroundIndex = 0;
	firstBackgroundChange = true;
	backgroundTimer = NULL;
	gback = CCLayerGradient::create(
		ccc4(backgroundMatrix[0][0], backgroundMatrix[0][1], backgroundMatrix[0][2], backgroundAlphaStart), 
		ccc4(backgroundMatrix[0][0], backgroundMatrix[0][1], backgroundMatrix[0][2], backgroundAlphaEnd)
		);
	this->addChild(gback);
	
	whiteBox = CCLayerColor::create(ccc4(255, 255, 255, 255));
	whiteBox->setContentSize(CCSizeMake(100, 100));
	whiteBox->setPosition(ccp(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 - 50));

	this->addChild(whiteBox);

	blackBox = CCLayerColor::create(ccc4(0, 0, 0, 255));
	blackBox->setContentSize(CCSizeMake(WINDOW_WIDTH, 50));
	blackBox->setPosition(ccp(0, 0));
	blackBox->setAnchorPoint(ccp(0, 0));
	this->addChild(blackBox);


	//	setup Physics	
	const b2Vec2 gravity(0.0f, -10.0f);
	this->boxWorldSleep = true;

	this->boxWorld = new b2World(gravity);
	this->boxWorld->SetAllowSleeping(this->boxWorldSleep);			
	
	//	setup debug drawing
	this->debugDraw =  new GLESDebugDraw(PTM_RATIO);
	this->boxWorld->SetDebugDraw(this->debugDraw);		

	this->debugDraw->AppendFlags(GLESDebugDraw::e_shapeBit);
	//this->debugDraw->AppendFlags(GLESDebugDraw::e_centerOfMassBit);
	//this->debugDraw->AppendFlags(GLESDebugDraw::e_aabbBit);
	//	<- ? does it work? seems not.	

	//	setup ground shape
	b2BodyDef bb;
	bb.userData = this->blackBox;
	bb.position.Set(this->blackBox->getPositionX(), this->blackBox->getPositionY());

	b2Body* bbb = this->boxWorld->CreateBody(&bb);

	b2PolygonShape bpb;
	bpb.SetAsBox((WINDOW_WIDTH / 2) / PTM_RATIO, (50 / 2) / PTM_RATIO);
	bbb->CreateFixture(&bpb, 0.0f);	

	//	setup dynamic box
	b2BodyDef bw;
	bw.userData = this->whiteBox;
	bw.type = b2_dynamicBody;
	bw.position.Set(this->whiteBox->getPositionX(), this->whiteBox->getPositionY());			

	boxWhite = this->boxWorld->CreateBody(&bw);
	
	b2PolygonShape bpw;
	bpw.SetAsBox(50 / PTM_RATIO, 50 / PTM_RATIO);		

	b2FixtureDef bfw;
	//bfw.density = 1.0f;
	//bfw.friction = 0.3f;
	bfw.shape = &bpw;
	boxWhite->CreateFixture(&bfw);	


	//	process keydown	
	this->schedule(schedule_selector(MainScene::tickKeyboard));
	this->setKeypadEnabled(true);

	this->scheduleUpdate();
	this->schedule(schedule_selector(MainScene::tickBackground));

	return true;
}

void MainScene::tickKeyboard(float delta)
{
	//CCLog("Tick keyboard");
	short l = GetKeyState(VK_LEFT);
	short r = GetKeyState(VK_RIGHT);
	short d = GetKeyState(VK_DOWN);
	short u = GetKeyState(VK_UP);

	long down = 0x8000; // hi bit
	short step = 50.0f;

	float x = whiteBox->getPositionX();
	float y = whiteBox->getPositionY();
	
	if (l & down)	
		x -= step;	
	if (r & down)	
		x += step;	
	if (d & down && y > blackBox->getContentSize().height)	
		y -= step;	
	if (u & down)	
		y += step;	

	//whiteBox->setPosition(x, y);
	boxWhite->ApplyForce(b2Vec2(x - whiteBox->getPositionX(), y - whiteBox->getPositionY()), b2Vec2(0, 0));
}

short tweenColor(short start, short end)
{
	if (start == end)
		return end;
	else if (start > end)
		return max(start - 1, 0);
	else
		return min(start + 1, 255);
}


void MainScene::update(float delta)
{
	//	PHYSICS UPDATE FIRST	
	this->boxWorld->Step(BOX_WOLRD_STEP, BOX_WORLD_VELOCITY_PASSES, BOX_WORLD_POSITION_PASSES);	

	for (b2Body* b = this->boxWorld->GetBodyList(); b; b = b->GetNext())
	{
		CCSprite* s = (CCSprite*) b->GetUserData();
		
		if (s != NULL)
		{
			b2Vec2 pos = b->GetPosition();
			s->setPosition(ccp(pos.x, pos.y));
			s->setRotation(CC_RADIANS_TO_DEGREES(b->GetAngle()));
			CCLog("New pos: %f %f", pos.x, pos.y);
		}
	}	
}

void MainScene::draw()
{
	//	check http://asmak9.blogspot.com/2012/08/cocos2d-x-enable-box2d-debugger.html

	CCLayer::draw();	  
	this->boxWorld->DrawDebugData();

}

void MainScene::tickBackground(float delta)
{
	//	update background color
	short timeToChange = 15;
	backgroundTimer += delta;
	//CCLog("Background timer %f", backgroundTimer);

	if (backgroundTimer >= timeToChange)
	{
		if (!firstBackgroundChange)
			lastUsedBackgroundIndex++;
		if (lastUsedBackgroundIndex > 2)
			lastUsedBackgroundIndex = 0;
		backgroundTimer = 0;
		firstBackgroundChange = false;
	}

	if (!firstBackgroundChange)
	{
		ccColor3B currentColor = gback->getStartColor();
		ccColor3B nextColor = lastUsedBackgroundIndex < 2 ? 
			ccc3(backgroundMatrix[lastUsedBackgroundIndex + 1][0], backgroundMatrix[lastUsedBackgroundIndex + 1][1], backgroundMatrix[lastUsedBackgroundIndex + 1][2])
			:
			ccc3(backgroundMatrix[0][0], backgroundMatrix[0][1], backgroundMatrix[0][2]);

		short r = tweenColor(currentColor.r, nextColor.r);
		short g = tweenColor(currentColor.g, nextColor.g);
		short b = tweenColor(currentColor.b, nextColor.b);
	
		ccColor3B newColor = ccc3(r, g, b);
		gback->setStartColor(newColor);
		gback->setEndColor(newColor);
	}
}