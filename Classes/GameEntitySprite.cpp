#include "GameEntitySprite.h"

///
///	GameEntitySprite
///	
bool GameEntitySprite::init(char* overidePath)
{
	bool pre = GameEntity::init();
	char* path = overidePath != NULL ? overidePath : m_nodeInfo.texture;
	if (path == NULL)
		return false;
	
	CCTextureCache* cache = CCTextureCache::sharedTextureCache();
	if (cache == NULL)
		return false;

	CCTexture2D* tex = cache->textureForKey(path);
	if (tex == NULL)
		tex = cache->addImage(path);
	
	m_sprite = CCSprite::createWithTexture(tex);
	return m_sprite != NULL;
}

///	
///	Apply style to sprite using NODEINFO structure.
///
bool GameEntitySprite::postInit()
{
	//position rotation and scale
	m_sprite->setPosition(m_nodeInfo.position);
	m_sprite->setRotation(CC_RADIANS_TO_DEGREES(m_nodeInfo.rotation));
	if (m_nodeInfo.scale)
		m_sprite->setScale(m_nodeInfo.scale);
	
	//	transformation
	if (m_nodeInfo.flipHorizontally)
		m_sprite->runAction(CCFlipX::create(true));
	if (m_nodeInfo.flipVertically)
		m_sprite->runAction(CCFlipY::create(true));

	return GameEntity::postInit();
}

bool GameEntitySprite::createBody(b2World* world)
{
	if ( world == NULL )
		return false;
	
	//	default 
	GameEntity::createBody(world);

	m_nodeInfo.size.width = m_sprite->getContentSize().width * m_nodeInfo.scale;
	m_nodeInfo.size.height = m_sprite->getContentSize().height * m_nodeInfo.scale;

	//	define and retain
	b2BodyDef def;
	def.userData = this;
	this->retain();

	def.type = m_customProps.isDynamicObject() ? b2_dynamicBody : b2_staticBody;
	def.position.Set(SCREEN_TO_WORLD(m_nodeInfo.position.x), SCREEN_TO_WORLD(m_nodeInfo.position.y));
	def.angle = -1 * m_nodeInfo.rotation;
	
	//	create
	m_b2Body = m_b2World->CreateBody(&def);
	if (m_b2Body == NULL)
		return false;	

	return createFixture();	
}

void GameEntitySprite::updatePosition(b2Vec2 pos)
{
	if (m_sprite)
	{	
		CCPoint posRecalc = ccp(WORLD_TO_SCREEN(pos.x), WORLD_TO_SCREEN(pos.y));
		m_sprite->setPosition(posRecalc);
	}
}

void GameEntitySprite::updateRotation(float32 angle)
{
	if (!m_sprite)
		m_sprite->setRotation(-1 * CC_RADIANS_TO_DEGREES(angle));
}

bool GameEntitySprite::createFixture()
{	
	b2PolygonShape ps;	
	ps.SetAsBox(SCREEN_TO_WORLD(m_nodeInfo.size.width / 2), SCREEN_TO_WORLD(m_nodeInfo.size.height / 2));

	m_b2FixtureDef.shape = &ps;

	b2Fixture* b = m_b2Body->CreateFixture(&m_b2FixtureDef);
	return b != NULL;
}