#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "PushdownMachine.h"

#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"


#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 2;
	}
};
	
NetworkedGame::NetworkedGame()	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;

	MenuSystem = new PushdownMachine(new MainMenu());
	MenuSystem->SetGame(this);
	isGameover = false;
	isRoundstart = false;

	PlayersList.clear();
	for (int i = 0; i < 4; ++i)
	{
		PlayersList.push_back(-1);
	}
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
	delete MenuSystem;
}

bool NetworkedGame::StartAsServer() {
	if (thisServer != nullptr)
	{
		return true;
	}
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 3);

	thisServer->RegisterPacketHandler(Received_State, this);
	return true;
}

bool NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	if (thisClient != nullptr)
	{
		return true;
	}
	thisClient = new GameClient();
	if (!thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort()))
	{
		return false;
	}

	//thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);
	thisClient->RegisterPacketHandler(Message, this);
	thisClient->RegisterPacketHandler(Round_State, this);
	return true;
}

void NetworkedGame::UpdateGame(float dt) {
	if (!MenuSystem->Update(dt))
	{
		isGameover = true;
	}

	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		// only the round start, the Server and Client can send the message about the round game;
		if (isRoundstart)
		{
			if (thisServer) {
				UpdateAsServer(dt);
			}
			else if (thisClient) {
				UpdateAsClient(dt);
			}
		}
		if (thisServer) { ServerUpdatePlayerList(); }

		timeToNextPacket += 1.0f / 60.0f; //20hz server/client update
	}

	// Server and Client Receive and process there packet
	if (thisServer) { thisServer->UpdateServer(); }
	if (thisClient) { thisClient->UpdateClient(); }

	if (isRoundstart)
	{
		UpdateGamePlayerInput(dt);

		//Test View
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::Q))
		{
			if (lockedObject == nullptr) {
				lockedObject = localPlayer; 
				Window::GetWindow()->ShowOSPointer(true);
			}
			else if (lockedObject == lockedObject) { 
				lockedObject = nullptr;
				Window::GetWindow()->ShowOSPointer(false);
			}
		}

		if (!inSelectionMode) {
			world->GetMainCamera().UpdateCamera(dt);
		}
		if (lockedObject != nullptr) {
			Vector3 objPos = lockedObject->GetTransform().GetPosition();
			Vector3 camPos = objPos + lockedOffset;

			Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

			Matrix4 modelMat = temp.Inverse();

			Quaternion q(modelMat);
			Vector3 angles = q.ToEuler(); //nearly there now!

			world->GetMainCamera().SetPosition(camPos);
			world->GetMainCamera().SetPitch(angles.x);
			world->GetMainCamera().SetYaw(angles.y);
		}

		//UpdateKeys();
		world->UpdateWorld(dt);
		renderer->Update(dt);
		// Only Server need to do physics calculation
		if (thisServer) { physics->Update(dt); }
	}

	renderer->Render();
	Debug::UpdateRenderables(dt);
}

int NetworkedGame::GetClientPlayerNum()
{
	if (thisClient)
	{
		for (int i = 1; i < 4; ++i)
		{
			if (PlayersList[i] == thisClient->GetPeerID())
			{
				return i;
			}
		}
	}
	return -1;
}

void NetworkedGame::InitWorld()
{
	world->ClearAndErase();
	physics->Clear();

	InitDefaultFloor();

	BridgeConstraintTest();
	//testStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));

	SpawnPlayer();
}

void NetworkedGame::UpdateAsServer(float dt) {
	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
}

void NetworkedGame::UpdateAsClient(float dt) {
	ClientPacket newPacket;

	Vector3 PointerPos;
	findOSpointerWorldPosition(PointerPos);
	newPacket.PointerPos = PointerPos;
	newPacket.btnStates[0] = Window::GetKeyboard()->KeyHeld(KeyCodes::W) ? 1 : 0;
	newPacket.btnStates[1] = Window::GetKeyboard()->KeyHeld(KeyCodes::S) ? 1 : 0;
	newPacket.btnStates[2] = Window::GetKeyboard()->KeyHeld(KeyCodes::D) ? 1 : 0;
	newPacket.btnStates[3] = Window::GetKeyboard()->KeyHeld(KeyCodes::A) ? 1 : 0;
	newPacket.btnStates[4] = Window::GetKeyboard()->KeyPressed(KeyCodes::SHIFT) ? 1 : 0;
	newPacket.btnStates[5] = Window::GetMouse()->ButtonPressed(MouseButtons::Type::Left) ? 1 : 0;
	newPacket.lastID = 0;

	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
	//	//fire button pressed!
	//	//newPacket.buttonstates[0] = 1;
	//	newPacket.lastID = 0; //You'll need to work this out somehow...
	//}
	thisClient->SendPacket(newPacket);
}

void NetworkedGame::ServerUpdatePlayerList()
{
	if (thisServer == nullptr)
	{
		return;
	}
	PlayersList[0] = 0;
	int peerID;
	for (int i = 0; i < 3; ++i)
	{
		if (thisServer->GetNetPeer(i, peerID))
		{
			PlayersList[i + 1] = peerID;
		}
		else
		{
			PlayersList[i + 1] = -1;
		}
	}
	PLayerListPacket plist(PlayersList);
	thisServer->SendGlobalPacket(plist);
}

void NetworkedGame::UpdateGamePlayerInput(float dt)
{
	if (thisServer)
	{
		if (localPlayer != nullptr)
		{
			NetworkPlayer* realPlayer = (NetworkPlayer*)localPlayer;

			Vector3 PointerPos;
			findOSpointerWorldPosition(PointerPos);
			realPlayer->SetPlayerYaw(PointerPos);
			bool btnVal[4] = { false, false, false ,false };
			if (Window::GetKeyboard()->KeyHeld(KeyCodes::W)) { btnVal[0] = true; }
			if (Window::GetKeyboard()->KeyHeld(KeyCodes::S)) { btnVal[1] = true; }
			if (Window::GetKeyboard()->KeyHeld(KeyCodes::D)) { btnVal[2] = true; }
			if (Window::GetKeyboard()->KeyHeld(KeyCodes::A)) { btnVal[3] = true; }
			realPlayer->MovePlayer(btnVal[0], btnVal[1], btnVal[2], btnVal[3]);
			if (Window::GetKeyboard()->KeyPressed(KeyCodes::SHIFT)) { realPlayer->PlayerSprint(); }
			if (Window::GetMouse()->ButtonPressed(MouseButtons::Type::Left)) { realPlayer->PlayerFire(); }
			realPlayer->GameTick(dt);
		}
	}
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		//TODO - you'll need some way of determining
		//when a player has sent the server an acknowledgement
		//and store the lastID somewhere. A map between player
		//and an int could work, or it could be part of a 
		//NetworkPlayer struct. 
		int playerState = o->GetLatestNetworkState().stateID;
		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket, deltaFrame, playerState)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
	}
}

void NetworkedGame::ServerSendRoundState()
{
	RoundStatePacket state(isRoundstart);
	thisServer->SendGlobalPacket(state);
}

void NetworkedGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

bool NetworkedGame::serverProcessCP(ClientPacket* cp, int source)
{

	return true;
}

bool NetworkedGame::clientProcessFp(FullPacket* fp)
{
	auto itr = networkObjects.find(fp->objectID);
	if (itr == networkObjects.end()) {
		std::cout << "Client Num" << GetClientPlayerNum() << "can't find netObject" << std::endl;
		return false;
	}
	itr->second->ReadPacket(*fp);
	return true;
}

bool NetworkedGame::clinetProcessDp(DeltaPacket* dp)
{
	auto itr = networkObjects.find(dp->objectID);
	if (itr == networkObjects.end()) {
		std::cout << "Client Num" << GetClientPlayerNum() << "can't find netObject" << std::endl;
		return false;
	}
	itr->second->ReadPacket(*dp);
	return true;
}

void NetworkedGame::findOSpointerWorldPosition(Vector3& position)
{
	Ray ray = CollisionDetection::BuildRayFromMouse(world->GetMainCamera());
	RayCollision closestCollision;
	if (world->Raycast(ray, closestCollision, true))
	{
		position = closestCollision.collidedAt;
		//Debug::DrawLine(ray.GetPosition(), position, Debug::GREEN, 3.0f);
	}
}

GameObject* NetworkedGame::AddNetPlayerToWorld(const Vector3& position, int playerNum)
{
	float meshSize = 1.0f;
	Vector3 volumeSize = Vector3(0.5, 0.5, 0.5);
	float inverseMass = 1.0f / 60.0f;

	NetworkPlayer* character = new NetworkPlayer(this, playerNum);
	AABBVolume* volume = new AABBVolume(volumeSize);

	character->SetBoundingVolume((CollisionVolume*)volume);
	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));
	character->SetNetworkObject(new NetworkObject(*character, playerNum));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(character);
	networkObjects.insert(std::pair<int, NetworkObject*>(playerNum, character->GetNetworkObject()));

	Vector4 colour;
	switch (playerNum)
	{
	case 0:
		colour = Debug::RED;
		break;
	case 1:
		colour = Debug::BLUE;
		break;
	case 2:
		colour = Debug::YELLOW;
		break;
	case 3:
		colour = Debug::CYAN;
		break;
	}
	character->GetRenderObject()->SetColour(colour);
	
	return character;
}

void NetworkedGame::SpawnPlayer() {
	serverPlayers.clear();
	for (int i = 0; i < 4; ++i)
	{
		if (GetPlayerPeerID(i) != -1)
		{
			serverPlayers.push_back(AddNetPlayerToWorld(Vector3(), i)); // got problem here
		}
		else
		{
			serverPlayers.push_back(nullptr);
		}
	}
	if (isServer())
	{
		localPlayer = serverPlayers[0];
	}
	else if (isClient())
	{
		localPlayer = serverPlayers[GetClientPlayerNum()];
	}
	LockCameraToObject(localPlayer);
}

void NetworkedGame::StartLevel() {
	InitWorld();

	physics->UseGravity(true);
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	switch (type)
	{
	case BasicNetworkMessages::Message:{
		PLayerListPacket* realPacket = (PLayerListPacket*)payload;
		realPacket->GetPlayerList(PlayersList);
		break;
	}
	case BasicNetworkMessages::Round_State: {
		RoundStatePacket* realPacket = (RoundStatePacket*)payload;
		isRoundstart = realPacket->isRoundStart;
		break;
	}
	case BasicNetworkMessages::Full_State: {
		FullPacket* realPacket = (FullPacket*)payload;
		clientProcessFp(realPacket);
		break;
	}
	case BasicNetworkMessages::Delta_State: {
		DeltaPacket* realPacket = (DeltaPacket*)payload;
		clinetProcessDp(realPacket);
		break;
	}
	case BasicNetworkMessages::Received_State: {
		ClientPacket* realPacket = (ClientPacket*)payload;
		serverProcessCP(realPacket);
		break;
	}
	}
}

void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		/*MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID  = a->GetPlayerNum();

		thisClient->SendPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisClient->SendPacket(newPacket);*/
	}
}

void NetworkedGame::ChangeRoundState(bool val)
{
	if (thisServer)
	{
		isRoundstart = val;
		ServerSendRoundState();
	}
}
