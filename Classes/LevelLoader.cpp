#include "LevelLoader.h"

///
///	LevelLoader Class
///



void LevelLoader::createLevelLayers()
{
	//	do not change the order of how we create layers, it will affect zOrder
	this->backgroundLayer = CCLayer::create();
	this->worldNode->addChild(this->backgroundLayer);

	this->mainLayer = CCLayer::create();
	this->worldNode->addChild(this->mainLayer);
}

bool LevelLoader::parse()
{
	bool success = false;

	sharedDoc = xmlReadFile(this->levelPath, "utf-8", XML_PARSE_RECOVER);
	CCAssert(sharedDoc, "Cannot load level file");	
	
	xmlNodePtr currNode = sharedDoc->children->children;
	
	while (currNode)
	{		
		if (xmlStrcasecmp(currNode->name, (const xmlChar*) "Layers") == 0)
		{						
			xmlNodePtr layers = currNode->children;
			while (layers)
			{
				const xmlChar* name = xmlGetProp(layers, (const xmlChar*) "Name");
				bool visible = parseNodeVisible(layers);	//
				
				if (visible && xmlStrcasecmp(name, (const xmlChar*) MAIN_LAYER_NAME) == 0)
				{
					xmlNodePtr mainLayers = layers->children->next;
					short count = xmlChildElementCount(mainLayers);
					CCLog("Found main layer with %d children", count);

					unsigned int zOrder = 0;
					xmlNodePtr mainLayerChild = mainLayers->children;
					while (mainLayerChild)
					{
						parseCurrentNode(mainLayerChild, 0, zOrder++);
						mainLayerChild = mainLayerChild->next;
					}
				}
				else if (visible && xmlStrcasecmp(name, (const xmlChar*) BACKGROUND_LAYER_NAME) == 0)
				{
					xmlNode *backLayers = layers->children->next;
					short count = xmlChildElementCount(backLayers);
					CCLog("Found background layer with %d children", count);

					unsigned int zOrder = 0;
					xmlNodePtr backLayerChild = backLayers->children;
					while (backLayerChild)
					{						
						parseCurrentNode(backLayerChild, 1, zOrder++);
						backLayerChild = backLayerChild->next;
					}
				}
				layers = layers->next;
			}

			break;	//	we are not interested in custom stuff for now, only layers part
		}
		currNode = currNode->next;
	}

	success = this->playerNode != NULL && this->playerBody != NULL;

	return success;
}

void LevelLoader::parseCurrentNode(xmlNodePtr node, unsigned int type, unsigned int zOrder)
{
	xmlChar* nodeName = xmlGetProp(node, (const xmlChar*) "Name");
	xmlChar* nodeType = xmlGetProp(node, (const xmlChar*) "type");

	if (nodeType == NULL)
		return;	//	skipp unknown or closing elements
	
	//CCLog("Processing node: %s with name: %s", nodeType, nodeName);

	//	Read everything we need from XML
	NODEINFO info;	
	info.xmlNode = node;
	info.position = parseNodePosition(node);
	info.size = parseNodeSize(node);
	info.rotation = parseNodeRotation(node);
	info.radius = parseNodeRadius(node);
	info.scale = parseNodeScale(node);
	info.color = parseNodeColor(node);
	info.tint = parseNodeColor(node, true);
	info.texture = parseNodeTexture(node);
	info.rawTexture = parseNodeTexture(node, true);
	info.assetTexture = parseNodeAssetName(node);
	info.flipHorizontally = parseNodeFlip(node);
	info.flipVertically = parseNodeFlip(node, true);
	info.visible = parseNodeVisible(node);	

	if (!info.visible)
	{
		//CCLog("Node hidden. Skipping.");
		return;
	}	

	//	check type
	if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_RECTANGLE) == 0)
	{		
		info.nodeType = NODEINFO_BLOCK; 
	}
	else if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_CIRCLE) == 0)
	{
		info.nodeType = NODEINFO_CIRCLE;
	}
	else if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_TEXTURE) == 0)
	{		
		info.nodeType = NODEINFO_TEXTURE;
	}

		//	select layer to insert it into
	CCNode* layer = type == 0 ? this->mainLayer : this->backgroundLayer;
	CCNode* toInsert = NULL;
	GameEntitySprite *sprite;

	if (info.nodeType == NODEINFO_BLOCK)
		sprite = GameEntityRectangle::create(info);
	else if (info.nodeType == NODEINFO_CIRCLE)
		sprite = GameEntityCircle::create(info);
	else if (info.nodeType == NODEINFO_TEXTURE)
		sprite = GameEntitySprite::create(info);

	if (sprite && sprite->getProperties().isPlayerObject())
	{
		sprite->removeBody();
		sprite->release();
		sprite = NULL;
		
		GameEntityPlayer* player = GameEntityPlayer::create(info);		
		if (player)
		{
			player->createBody(this->boxWorld);
			layer->addChild(player->getSkin(), 1000000);

			this->playerBody = player->getBody();
			this->playerNode = player->getSkin();
		}
	}
	else if (sprite)
	{
		layer->addChild(sprite->getSprite(), zOrder);
		
		if (type == 0)
			sprite->createBody(this->boxWorld);
	}	
		

	//	FIRST PROCESS PROPERTIES
	/*
	CustomProperties props;
	bool propsObtained = parseNodeProperties(info, &props);

	//	THEN PARSE NODE AND INSERT IT INTO COCOS WORLD
	bool inserted = parseNodeToCocosNode(info, props, type, zOrder);

	//	THEN PROCESS PHYSICS (main layer only)
	if (type == 0 && inserted)
		parseNodePhysics(info, props);
	*/
	//	since parseNodeTexture allocates we release it here	
	//if (info.texture)
	//	delete [] info.texture;	
}

bool LevelLoader::parseNodeToCocosNode(NODEINFO &info, CustomProperties props , unsigned int type, unsigned int zOrder)
{
	return true;
	/*
	//	select layer to insert it into
	CCNode* layer = type == 0 ? this->mainLayer : this->backgroundLayer;
	CCNode* toInsert = NULL;
	GameEntitySprite *sprite;

	if (info.nodeType == NODEINFO_BLOCK)
		sprite = GameEntityRectangle::create(info);
	else if (info.nodeType == NODEINFO_CIRCLE)
		sprite = GameEntityCircle::create(info);
	else if (info.nodeType == NODEINFO_TEXTURE)
		sprite = GameEntitySprite::create(info);

	if (sprite)
		layer->addChild(sprite->getSprite(), zOrder);

	return true;

	//	NOW DO ACTUAL WORLD CREATIONG -- LIKE A BOSS	
	if (info.type == 0)
	{
		//	anchor point is always in (0,0) for layer, but our position is mapped to (0.5, 0.5), or element center
		//	compensate (sounds awfully like something said in star trek :)
		info.position.x += info.size.width / 2.0f;
		info.position.y -= info.size.height / 2.0f;

		//	primitives cannot be batched, we will use block.png stretched as a block primitive
		CCSprite *a = CCSprite::create(RESOURCE_BLOCK);
		a->setColor(ccc3(info.color.r, info.color.g, info.color.b));

		float sx = info.size.width / a->getContentSize().width;
		float sy = info.size.height / a->getContentSize().height;
		
		a->setScaleX(sx);
		a->setScaleY(sy);
		info.scale = 0;

		toInsert = a;
	}
	else if (info.type == 1)
	{
		CCSprite *a = NULL;
		if (props.isPlayerObject())
		{
			a = CCSprite::create(RESOURCE_PLAYER);
			a->setTag(PLAYER_TAG);
		}
		else
		{
			a = CCSprite::create(RESOURCE_CIRCLE);
			a->setColor(ccc3(info.color.r, info.color.g, info.color.b));
		}
		
		float width = a->getContentSize().width;
		info.scale =  info.radius * 2 / width; //	diametar is 2xradius
		toInsert = a;
	}
	else if (info.type == 2)
	{
		CCSprite *a = CCSprite::create(info.texture);

		info.size.width = a->getContentSize().width * info.scale;
		info.size.height = a->getContentSize().height * info.scale;
		
		a->setColor(ccc3(info.tint.r, info.tint.g, info.tint.b));

		toInsert = a;		
	}

	if (toInsert)
	{
		toInsert->setPosition(info.position);
		toInsert->setRotation(CC_RADIANS_TO_DEGREES(info.rotation));
		if (info.scale)
			toInsert->setScale(info.scale);
		
		if (info.flipHorizontally)
			toInsert->runAction(CCFlipX::create(true));
		if (info.flipVertically)
			toInsert->runAction(CCFlipY::create(true));
		
		layer->addChild(toInsert, zOrder);

		//	check for known types
		if (props.isPlayerObject())
			this->playerNode = toInsert;
		else if (props.isFinishObject())
			this->finishNode = toInsert;
	}

	info.cocosNode = toInsert;
	return toInsert != NULL;
	*/
}

bool LevelLoader::parseNodeProperties(NODEINFO &info, __out CustomProperties *props)
{
	bool res = props->parseFromNode(info.xmlNode);
	return res;
}

bool LevelLoader::parseNodePhysics(NODEINFO &info, __in CustomProperties props)
{
	return false;
	/*
	bool skipPhysics = false;
	if (!skipPhysics)
	{
		if (this->boxWorld == NULL)
		{
			CCLog("Box world is not set up. Cannot create physics");
			return false;
		}

		//	now set body fixtures
		b2BodyDef def;
		def.userData = info.cocosNode;
		def.type = props.isDynamicObject() || props.isPlayerObject() ? b2_dynamicBody : b2_staticBody;

		def.position.Set(SCREEN_TO_WORLD(info.position.x), SCREEN_TO_WORLD(info.position.y));
		def.angle = -1 * info.rotation;		

		b2Body* body = this->boxWorld->CreateBody(&def);

		//	now create fixtures for known shapes, or images without shape data
		b2FixtureDef fd;
		fd.density = 1.0f;
		
		if (props.isPlayerObject())
		{
			this->playerBody = body;
			fd.density = 3.0f;			
			fd.friction = 0.8f;
			fd.restitution = 0;
			
			b2FixtureDef groundBody;
			b2PolygonShape groundBodyShape;

			CCSize playerSize = this->playerNode->getContentSize();
			playerSize.height += 60;	//	more than player size - so it goes into ground
			groundBodyShape.SetAsBox(SCREEN_TO_WORLD(playerSize.width / 2), SCREEN_TO_WORLD(playerSize.height / 2));

			groundBody.isSensor = true;
			groundBody.shape = &groundBodyShape;

			this->playerBody->CreateFixture(&groundBody);
		}
		
		//	check for custom shape
		bool customFixture = shapeHelper->createShapeForItem(info.assetTexture, body, info.size, fd.density, fd.friction, props.isBouncable());

		if (!customFixture)
		{		
			b2CircleShape cs;
			b2PolygonShape ps;

			if (info.type == 1)
			{
						b2CircleShape cs;
				cs.m_radius = SCREEN_TO_WORLD(info.radius);
	
				fd.shape = &cs;				
			}
			else
			{
				ps.SetAsBox(SCREEN_TO_WORLD(info.size.width / 2), SCREEN_TO_WORLD(info.size.height / 2));
				fd.shape = &ps;
			}

			if (props.isBouncable())
				fd.restitution = 0.5;

			body->CreateFixture(&fd);
		}					
	}
	*/
	return true;
}

CCPoint LevelLoader::parseNodePosition(xmlNodePtr node)
{
	CCPoint r(0, 0);	
	xmlNodePtr pos = XMLHelper::findChildNodeWithName(node, "Position");
	if (pos)
	{
		r.x = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(pos, "X"));
		r.y =  (1 /* WE HAVE TO INVERT Y */) - XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(pos, "Y"));
	}
	//CCLog("POSITION x: %f y: %f", r.x, r.y);
	return r;
}

CCSize LevelLoader::parseNodeSize(xmlNodePtr node)
{
	CCSize r(0, 0);
	r.width = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(node, "Width"));
	r.height = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(node, "Height"));
	//CCLog("SIZE width: %f height: %f", r.width, r.height);
	return r;
}

float LevelLoader::parseNodeRotation(xmlNodePtr node)
{
	float ret = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(node, "Rotation"));
	//CCLog("Rotation: %f", ret);
	return ret;
}

float LevelLoader::parseNodeRadius(xmlNodePtr node)
{
	float ret = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(node, "Radius"));
	//CCLog("Radius: %f", ret);
	return ret;
}

float LevelLoader::parseNodeScale(xmlNodePtr node)
{
	float ret = XMLHelper::readNodeContentF(XMLHelper::findChildNodeWithName(XMLHelper::findChildNodeWithName(node, "Scale"), "X"));
	if (ret == NULL)
		ret = 1; //	default scale
	//CCLog("Scale: %f", ret);
	return ret;
}

bool LevelLoader::parseNodeFlip(xmlNodePtr node, bool vertical)
{
	char* name = vertical ? "FlipVertically" : "FlipHorizontally";
	bool res = XMLHelper::readNodeContentB(XMLHelper::findChildNodeWithName(node, name));
	return res;
}

bool LevelLoader::parseNodeVisible(xmlNodePtr node)
{	
	const xmlChar* res = xmlGetProp(node, (const xmlChar*) "Visible");
	return xmlStrcasecmp(res, (const xmlChar*) "true") == 0;
}

ccColor4B LevelLoader::parseNodeColor(xmlNodePtr node, bool tint)
{
	ccColor4B ret;
	char* name = tint == true ? "TintColor" : "FillColor";
	xmlNodePtr color = XMLHelper::findChildNodeWithName(node, name);	
	ret.r = XMLHelper::readNodeContentU(XMLHelper::findChildNodeWithName(color, "R"));
	ret.g = XMLHelper::readNodeContentU(XMLHelper::findChildNodeWithName(color, "G"));
	ret.b = XMLHelper::readNodeContentU(XMLHelper::findChildNodeWithName(color, "B"));
	ret.a = XMLHelper::readNodeContentU(XMLHelper::findChildNodeWithName(color, "A"));	
	//CCLog("Color (%s) r: %d g: %d b: %d a: %d", name, ret.r, ret.g, ret.b, ret.a);
	return ret;
}

char* LevelLoader::parseNodeTexture(xmlNodePtr node, bool raw)
{
	char* ret = NULL;
	char* read = XMLHelper::readNodeContent(XMLHelper::findChildNodeWithName(node, "texture_filename"));

	if (read && !raw)
	{
		ret = (char*) malloc(255);
		sprintf(ret, "%s%s", RESOURCE_DIR, read);
	}
	else if (read)
		ret = read;

	//CCLog("Texture: %s", ret);
	return ret;
}

char* LevelLoader::parseNodeAssetName(xmlNodePtr node)
{
	char* ret = NULL;
	char* read = XMLHelper::readNodeContent(XMLHelper::findChildNodeWithName(node, "asset_name"));
	return read;
}
