#include "NetworkPlayer.h"
#include "NetworkedGame.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

NetworkPlayer::NetworkPlayer(NetworkedGame* game, int num)	{
	this->game = game;
	playerNum  = num;

	sprintTimer = SprintCDT;
	fireTimer = FireCDT;
	score = 0;
}

NetworkPlayer::~NetworkPlayer()	{

}

void NetworkPlayer::OnCollisionBegin(GameObject* otherObject) {
	if (game) {
		if (dynamic_cast<NetworkPlayer*>(otherObject))
		{
			game->OnPlayerCollision(this, (NetworkPlayer*)otherObject);
		}
	}
}

void NetworkPlayer::GameTick(float dt)
{
	UpdateTimer(dt);
}

Quaternion GenerateOrientation(const Vector3& axis, float angle)
{
	float halfAngle = angle / 2;
	float s = std::sin(halfAngle);
	float w = std::cos(halfAngle);
	float x = axis.x * s;
	float y = axis.y * s;
	float z = axis.z * s;
	return Quaternion(x, y, z, w);
}

void NetworkPlayer::SetPlayerYaw(const Vector3& pointPos)
{
	Quaternion orientation;
	Vector3 pos = transform.GetPosition();
	Vector3 targetForwardVec = (pointPos - pos);
	targetForwardVec.y = 0;
	targetForwardVec = targetForwardVec.Normalised();

	Vector3 forward = Vector3(0, 0, -1);

	float cosTheta = Vector3::Dot(forward, targetForwardVec);
	Vector3 rotationAxis;
	float angle;
	if (cosTheta < -1 + 0.001f) 
	{
		rotationAxis = Vector3::Cross(Vector3(0, 0, 1), forward);
		if (rotationAxis.Length() < 0.01)
		{
			rotationAxis = Vector3::Cross(Vector3(1, 0, 0), forward);
		}
		rotationAxis = rotationAxis.Normalised();
		angle = 3.1415926f;
	}
	else
	{
		rotationAxis = Vector3::Cross(forward, targetForwardVec);
		rotationAxis = rotationAxis.Normalised();
		angle = std::acos(cosTheta);
	}
	orientation = GenerateOrientation(rotationAxis, angle);

	transform.SetOrientation(orientation);

	//Debug::DrawLine(pos, pos + forward * 30, Debug::MAGENTA);
}

void NetworkPlayer::MovePlayer(bool Up, bool Down, bool Right, bool Left)
{
	Vector3 MoveDir = Vector3(0, 0, 0);
	if (Up)    { MoveDir += Vector3( 0, 0, -1); }
	if (Down)  { MoveDir += Vector3( 0, 0,  1); }
	if (Right) { MoveDir += Vector3( 1, 0,  0); }
	if (Left)  { MoveDir += Vector3(-1, 0,  0); }
	MoveDir = MoveDir.Normalised();

	float maxSpeed = 10.0f;
	float f = 600.0f;
	if (physicsObject->GetLinearVelocity().Length() > maxSpeed)
	{
		f = 0.0f;
	}
	Vector3 force = MoveDir * f;
	
	this->physicsObject->AddForce(force);
	Debug::DrawLine(this->transform.GetPosition(), this->transform.GetPosition() + force, Debug::RED, 0.0f);
}

void NetworkPlayer::PlayerSprint()
{
	if (sprintTimer <= 0.0f)
	{
		Vector3 sprintDir = getPlayerForwardVector();
		float f = 150000.0f;
		Vector3 force = sprintDir * f;
		this->physicsObject->AddForce(force);
		Debug::DrawLine(transform.GetPosition(), transform.GetPosition() + force, Debug::GREEN, 3.0f);
		sprintTimer = SprintCDT;
	}
}

void NetworkPlayer::PlayerFire()
{
	if (fireTimer <= 0)
	{
		Vector3 fireDir = getPlayerForwardVector();
		Debug::DrawLine(transform.GetPosition(), transform.GetPosition() + fireDir * 20.0f, Debug::CYAN, 3.0f);
		fireTimer = FireCDT;
	}
}

Vector3 NetworkPlayer::getPlayerForwardVector()
{
	Vector3 vec = Vector3(0, 0, -1);
	vec = transform.GetOrientation() * vec;
	vec = vec.Normalised();
	return vec;
}

void NetworkPlayer::UpdateTimer(float dt)
{
	sprintTimer -= dt;
	fireTimer -= dt;

	if (sprintTimer < 0) { sprintTimer = 0; }
	if (fireTimer < 0) { fireTimer = 0; }
}
