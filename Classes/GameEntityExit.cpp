#include "GameEntityExit.h"
#include "MainScene.h"
#include "Settings.h"

bool GameEntityExit::postInit()
{
	//	take scale in consideration
	m_nodeInfo.size.width *= m_nodeInfo.scale;
	m_nodeInfo.size.height *= m_nodeInfo.scale;

	//	fix anchor point
	m_nodeInfo.position.x += m_nodeInfo.size.width / 2;
	m_nodeInfo.position.y -= m_nodeInfo.size.height / 2;
	return true;
}

bool GameEntityExit::createFixture()
{
	m_b2Body = m_b2World->CreateBody(&m_b2BodyDef);
	if (m_b2Body == NULL)
		return false;

	this->retain();

	b2FixtureDef fd;
	fd.isSensor = true;

	b2PolygonShape ps;
	ps.SetAsBox(SCREEN_TO_WORLD(m_nodeInfo.size.width / 2.0f), SCREEN_TO_WORLD(m_nodeInfo.size.height / 2.0f));
	
	fd.shape = &ps;

	m_b2Body->CreateFixture(&fd);

	return true;
}

bool GameEntityExit::loadNextLevel()
{
	//	if next level path is set load that, otherwise load same level again
	char* name = m_nodeInfo.nextLevel;
	if (name != NULL)
	{
		char tmp[MAX_PATH];
		sprintf_s(tmp, "%s%s", RESOURCE_DIR, name);			

		if (doesFileExits(tmp))
		{
			extern char* levelOverride;
			levelOverride = _strdup((char*) tmp);
		}
		else
			MessageBox(NULL, "Exit level path not set or file does not exit, reloading this one!", "NextLevel condition not set", MB_OK | MB_ICONWARNING);
	}

	CCDirector::sharedDirector()->replaceScene(MainScene::scene());

	return true;
}