#ifndef _GAME_ENTITY_PLAYER_H_
#define _GAME_ENTITY_PLAYER_H_

#include "GameEntitySprite.h"
#include "KeyboardHelper.h"
#include "Performance.h"

//	Good movement mechanics example
//http://www.miniclip.com/games/dale-and-peakot/en/#tag-cat-anim-2289

///
///	Movement modifiers
///
#define PLAYER_SPEED			14.0f
#define PLAYER_JUMP_VALUE		400.0f
#define PLAYER_STEP_VALUE		25.0f

#define PLAYER_MID_AIR_FACTOR	0.3f
#define PLAYER_SHIFT_FACTOR		1.45f

#define PLAYER_JUMP_HALFSTEP	0.1f
#define PLAYER_HALFSTEP_VALUE	170.0f
#define PLAYER_HALFSTEP_TIMES	3.0f

//	Make arch when jumping
#define PLAYER_JUMP_PUSH		-11.0f

//	How long player has to be in air before claimed dead (in seconds)
#define IN_AIR_BEFORE_DEATH		4.0f


///
///	Assets used for player
///
#define RESOURCE_PLAYER			RESOURCE_PL_DIR	"dog.png"
#define RESOURCE_PLAYER2		RESOURCE_PL_DIR	"dog2.png"
#define RESOURCE_PLAYER3		RESOURCE_PL_DIR	"dog3.png"
#define RESOURCE_PLAYER4		RESOURCE_PL_DIR	"dog4.png"

///	Player direction
enum PlayerDirection
{
	PlayerDirectionLeft,
	PlayerDirectionRight
};

///
///	Rectangle Primitive Entity
///
class GameEntityPlayer : public GameEntity
{
protected:

	CCNode* m_skin;
	CCAnimation* m_animationStill;	

	b2Fixture* m_sensorFixture;
	
	///	Keeps permanent record of player death state in case of tunneling
	bool m_bPlayerDied;

	float m_bForwardTrustWasOn;
	float m_bVerticalThrustWasOn;
	float m_bVerticalThrustCounter;

	float stepValue;
	float jumpValue;
	float maxSpeed;
	float shiftFactor;
	float midAirFactor;

	float m_secondsInAir;

	GameEntityPlayer(NODEINFO info) : GameEntity(info) 
	{
		m_secondsInAir = 0;
		m_bPlayerDied = false;
		direction = PlayerDirectionRight;

		stepValue = PLAYER_STEP_VALUE;
		jumpValue = PLAYER_JUMP_VALUE;		
		maxSpeed = PLAYER_SPEED;
		shiftFactor = PLAYER_SHIFT_FACTOR;
		midAirFactor = PLAYER_MID_AIR_FACTOR;
		m_bForwardTrustWasOn = 0;
		m_bVerticalThrustWasOn = 0;
		m_bVerticalThrustCounter = 0;
	}
	
	virtual bool init();
	
	//	Helper methods
	virtual void checkPlayerDirection(bool left, bool right);


public:
	
	static GameEntityPlayer* create(NODEINFO info)
	{
		GameEntityPlayer* p = new GameEntityPlayer(info);
		if (p && p->init())
		{
			p->autorelease();
			return p;
		}
		CC_SAFE_DELETE(p);
		return p;
	}

	PlayerDirection direction;

	virtual bool createBody(b2World* world);

	virtual void updatePosition(b2Vec2 pos);
	virtual void updateRotation(float32 angle);

	virtual void updatePlayerMovement(float delta);

	virtual CCNode* getSkin() { return m_skin; } 

	///	Checks if player is touching the ground
	
	virtual bool isPlayerMidAir();	
	virtual bool checkForDeath();

};

#endif