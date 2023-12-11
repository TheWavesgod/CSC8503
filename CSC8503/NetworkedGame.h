#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "PushdownState.h"

namespace NCL {
	namespace CSC8503 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;
		class PushdownMachine;
		class DeltaPacket;
		class FullPacket;

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame();
			~NetworkedGame();

			bool StartAsServer();
			bool StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			void SpawnPlayer();

			void StartLevel();

			void ReceivePacket(int type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			bool isRoundStart() { return isRoundstart; }
			void ChangeRoundState(bool val);

			bool isGameOver() { return isGameover; }

			bool isServer() { return thisServer != nullptr; }
			bool isClient() { return thisClient != nullptr; }

			int GetPlayerPeerID(int num) { return PlayersList[num]; }
			int GetClientPlayerNum();

		protected:
			void InitWorld() override;

			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);
			void ServerUpdatePlayerList();

			void UpdateGamePlayerInput(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void ServerSendRoundState();
			void UpdateMinimumState();

			bool serverProcessCP(ClientPacket* cp, int source);
			bool clientProcessFp(FullPacket* fp);
			bool clinetProcessDp(DeltaPacket* dp);

			void findOSpointerWorldPosition(Vector3& position);

			GameObject* AddNetPlayerToWorld(const Vector3& position, int playerNum);

			std::map<int, int> stateIDs;  

			GameServer* thisServer;
			GameClient* thisClient;
			float timeToNextPacket;
			int packetsToSnapshot;

			std::map<int, NetworkObject*> networkObjects;

			std::vector<int> PlayersList;
			std::vector<GameObject*> serverPlayers;
			GameObject* localPlayer;

			PushdownMachine* MenuSystem;
			bool isRoundstart;
			bool isGameover;
		};

		class InGame : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Round Start", Vector2(33, 10), Debug::YELLOW);
				return PushdownResult::NoChange;
			}
		};

		class MultiplayerLobby : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("MultiPlayer Lobby", Vector2(33, 10), Debug::YELLOW);
				Debug::Print("================================================", Vector2(5, 20), Debug::YELLOW);
				Debug::Print("================================================", Vector2(5, 70), Debug::YELLOW);
				Debug::Print("Press Esc : MultiPlayer Menu", Vector2(5, 90), Debug::YELLOW);
				if (game)
				{
					NetworkedGame* thisGame = (NetworkedGame*)game;
					if (thisGame->isRoundStart())
					{
						thisGame->StartLevel();
						*newState = new InGame();
						return PushdownResult::Push;
					}
					if (thisGame->isServer())
					{
						Debug::Print("is You", Vector2(40, 30), Debug::RED);
						Debug::Print("Press S : Game Start!", Vector2(5, 80), Debug::YELLOW);
						if (Window::GetKeyboard()->KeyPressed(KeyCodes::S))
						{
							thisGame->ChangeRoundState(true);
						}
					}
					if (thisGame->isClient())
					{
						int num = thisGame->GetClientPlayerNum();
						Debug::Print("is You", Vector2(40, 30 + num * 10), Debug::RED);
						Debug::Print("Wait for Server to start game....", Vector2(5, 80), Debug::YELLOW);
					}
					if (thisGame->GetPlayerPeerID(0) != -1)
					{
						Debug::Print("Player 1", Vector2(5, 30), Debug::RED);
					}
					if (thisGame->GetPlayerPeerID(1) != -1)
					{
						Debug::Print("Player 2", Vector2(5, 40), Debug::BLUE);
					}
					if (thisGame->GetPlayerPeerID(2) != -1)
					{
						Debug::Print("Player 3", Vector2(5, 50), Debug::YELLOW);
					}
					if (thisGame->GetPlayerPeerID(3) != -1)
					{
						Debug::Print("Player 4", Vector2(5, 60), Debug::CYAN);
					}
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE))
				{
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
		};

		class MultiplayerMenu : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Press 1 : Start As Server", Vector2(5, 10), Debug::YELLOW);
				Debug::Print("Press 2 : Start As Client", Vector2(5, 20), Debug::YELLOW);
				Debug::Print("Press Esc : Main Menu", Vector2(5, 70), Debug::YELLOW);

				if (game)
				{
					NetworkedGame* thisGame = (NetworkedGame*)game;
					if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
					{
						thisGame->StartAsServer();
						*newState = new MultiplayerLobby();
						return PushdownResult::Push;
					}
					if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
					{
						if (thisGame->StartAsClient(127, 0, 0, 1))
						{
							*newState = new MultiplayerLobby();
							return PushdownResult::Push;
						}
						Debug::Print("Failed to connect the server!!!", Vector2(50, 30), Debug::RED);
					}
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE))
				{
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
		};

		class MainMenu : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Press 1 : Solo Game", Vector2(5, 10), Debug::YELLOW);
				Debug::Print("Press 2 : MultiPlayer Game", Vector2(5, 20), Debug::YELLOW);
				Debug::Print("Press 3 : Option", Vector2(5, 30), Debug::YELLOW);
				Debug::Print("Press Esc : Game Over", Vector2(5, 70), Debug::YELLOW);

				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				{

				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				{
					*newState = new MultiplayerMenu();
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM3))
				{

				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE))
				{
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
		};
	}
}



