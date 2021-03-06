#include "LevelLoader.h"

///
///	LevelLoader Class
///

void LevelLoader::createLevelLayers()
{
	paralaxNode = CCParallaxNode::create();
	paralaxNode->setPosition(0, 0);
	this->worldNode->addChild(paralaxNode);
}

bool LevelLoader::parse()
{
	PROFILE_FUNC();

	bool success = false;

	sharedDoc = XMLHelper::loadFile(this->levelPath);
	if (!sharedDoc)
	{
		CCLog("Cannot init XML from file %s", this->levelPath);
		return false;
	}

	//	now parse level properties
	parseLevelProperties();
	
	//xmlNodePtr currNode = sharedDoc->children->children;
	xmlNodePtr layers = XMLHelper::findChildNodeWithName((xmlNodePtr) sharedDoc, "Level");
	if (layers){
		layers = XMLHelper::findChildNodeWithName(layers, "Layers");
		if (layers)
			layers = layers->FirstChild();
	}

	while (layers)
	{
		bool visible = parseNodeVisible(layers);
		const xmlChar* name = XMLHelper::readNodeAttribute(layers, "Name");
		bool main = xmlStrcasecmp(name, (const xmlChar*) MAIN_LAYER_NAME) == 0;
		
		//	get paralax info
		CCPoint parallax = ccp(1, 1);	//default
		CCLayer *parent = NULL;
				
		parent = CCLayer::create();

		if (main)
			mainLayer = parent;
		
		xmlNodePtr speed = XMLHelper::findChildNodeWithName(layers, "ScrollSpeed");
		if (speed)
		{
			xmlNodePtr xn = XMLHelper::findChildNodeWithName(speed, "X");
			if (xn)
				parallax.x = XMLHelper::readNodeContentF(xn);
			xmlNodePtr yn = XMLHelper::findChildNodeWithName(speed, "Y");
			if (yn)
				parallax.y = XMLHelper::readNodeContentF(yn);
		}

		paralaxNode->addChild(parent, 0, parallax, ccp(0, 0));

		if (layers->FirstChild())
		{
			xmlNodePtr subLayers = layers->FirstChild();
			if (subLayers)
			{
				xmlNodePtr ptr = subLayers->FirstChild();
				while (ptr)
				{
					parseCurrentNode(ptr, parallax, parent, main);
					ptr = ptr->NextSibling();
				}
			}
		}
		layers = layers->NextSibling();
	}


	success = this->playerNode != NULL && this->playerBody != NULL;

	return success;
}

void LevelLoader::parseCurrentNode(xmlNodePtr node, CCPoint parallax, CCLayer* parent, bool isMainLayer)
{
	PROFILE_FUNC();

	xmlChar* nodeName = (xmlChar*) XMLHelper::readNodeAttribute(node, "Name");
	xmlChar* nodeType = (xmlChar*) XMLHelper::readNodeAttribute(node, "xsi:type");

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
	info.rawTexture = parseNodeTexture(node, true);
	info.assetTexture = parseNodeAssetName(node);
	info.flipHorizontally = parseNodeFlip(node);
	info.flipVertically = parseNodeFlip(node, true);
	info.visible = parseNodeVisible(node);	
	info.worldLayerNode = this->worldNode;
	info.boxWorldNode = this->boxWorld;

	//	copy allocated string and free it
	char* tmp = parseNodeTexture(node);
	if (tmp)
	{
		strcpy_s(info.texture, tmp);
		delete [] tmp;
	}

	///	skip invisible nodes
	if (!info.visible)
		return;	

	if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_PLAYER) == 0 && !loadedPlayer)
	{
		loadedPlayer = true;
		info.nodeType = NodeTypePlayer;
		GameEntityPlayer* player = GameEntityPlayer::create(info);		
		if (player)
		{
			player->createBody(this->boxWorld);
			this->mainLayer->addChild(player->getSkin(), 1000000);

			this->playerBody = player->getBody();
			this->playerNode = player->getSkin();

			this->player = player;			
		}
	}
	else if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_EXIT) == 0)
	{
		info.nodeType = NodeTypeExit;
		xmlNodePtr np = XMLHelper::findChildNodeWithName(info.xmlNode, "NextLevel");
		if (np)		
			info.nextLevel = XMLHelper::readNodeContent(np);		

		GameEntityExit* ge = GameEntityExit::create(info);
		if (ge)
		{
			ge->createBody(this->boxWorld);
			ge->createFixture();
		}
	}
	else if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_COLL_PATH) == 0)
	{
		info.nodeType = NodeType::NodeTypeCollisionPath;
		GameEntityCollisionPath* path = GameEntityCollisionPath::create(info);
		if (path)
		{
			path->createBody(this->boxWorld);
			path->createFixture();
		}
	}
	else if (xmlStrcasecmp(nodeType, (const xmlChar*) ENEMY_TYPE_CHIPMUNK) == 0)
	{
		CCLog("Fouund chipmunk");

		info.nodeType = NodeTypeEnemy;
		info.enemyType = EnemyTypeSquirel;
		EnemyChipmunk * chipmunk = EnemyChipmunk::create(info);

		CCSprite* s = chipmunk->getSprite();
		parent->addChild(s);

		chipmunk->createBody(this->boxWorld);
	}
	else
	{
		//	select layer to insert it into
		CCNode* layer = parent;
		CCNode* toInsert = NULL;
		GameEntitySprite *sprite;
		
		//	check type
		if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_TEXTURE) == 0)
		{		
			info.nodeType = NodeTypeTexture;
			sprite = GameEntitySprite::create(info);
		}
		if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_RECTANGLE) == 0)
		{		
			info.nodeType = NodeTypeBlock; 
			sprite = GameEntityRectangle::create(info);
		}
		else if (xmlStrcasecmp(nodeType, (const xmlChar*) ITEM_TYPE_CIRCLE) == 0)
		{
			info.nodeType = NodeTypeCircle;
			sprite = GameEntityCircle::create(info);
		}		
	
		//	then instert to view
		if (sprite && info.nodeType != NodeType::NodeTypeUndefined)
		{
			//layer->addChild(sprite->getSprite());
			//BatchManager* manager = BatchManager::sharedManager();
			if (manager)
			{
				manager->addItem(sprite, layer, parallax);

				//	calculate max positions of the scene
				LevelProperties *lp = LevelProperties::sharedProperties();
				CCPoint pos = sprite->getNodeInfo().position;
				CCSize size = sprite->getNodeInfo().size;

				//	further to the left
				if (pos.x < lp->SceneSize.origin.x)
					lp->SceneSize.origin.x = pos.x;

				float toRight = pos.x + size.width;
				if (toRight > lp->SceneSize.size.width)
					lp->SceneSize.size.width = toRight;

				if (pos.y < lp->SceneSize.origin.y)
					lp->SceneSize.origin.y = pos.y;

				float toTop = pos.y + size.height;
				if (toTop > lp->SceneSize.size.height)
					lp->SceneSize.size.height = toTop;
			}
		
			if (isMainLayer || sprite->getProperties().isSolid() || sprite->getProperties().isCollectable())
				sprite->createBody(this->boxWorld);
		}
	}
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
	const xmlChar* res = XMLHelper::readNodeAttribute(node, "Visible");
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

void LevelLoader::parseLevelProperties()
{	
	LevelProperties* l = LevelProperties::sharedProperties(sharedDoc);
	if (l)
		l->WorldLayer = this->worldNode;
}