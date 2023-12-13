#pragma once
#include "GameObject.h"
#include "GameClient.h"

namespace NCL {
	namespace CSC8503 {
		class NetworkedGame;
		class StateMachine;

		enum ScoreType {
			bulletHitAI  = 3,
			bulletHitPlayer = 5
		};

		class NetworkPlayer : public GameObject {
		public:
			static constexpr float SprintCDT = 4.0f;
			static constexpr float FireCDT = 2.0f;

			static NetworkPlayer* createAngryGoose(NetworkedGame* game, int num);

			NetworkPlayer(NetworkedGame* game, int num);
			~NetworkPlayer();

			void OnCollisionBegin(GameObject* otherObject) override;

			void GameTick(float dt);

			int GetPlayerNum() const {
				return playerNum;
			}

			void SetBtnState(int btn, char val) { btnState[btn] = val; }
			char GetBtnState(int btn)const { return btnState[btn]; }

			void SetPlayerYaw(const Vector3& pointPos);
			void MovePlayer(bool Up, bool Down, bool Right, bool Left);
			void PlayerSprint();
			void PlayerFire();

			Vector3 getPlayerForwardVector();

			float getSprintCD() const { return sprintTimer < 0 ? 0 : sprintTimer; }
			void setSprintCD(float st) { sprintTimer = st; }
			float getFireCD() const { return fireTimer < 0 ? 0 : fireTimer; }
			void setFireCD(float ft) { fireTimer = ft; }

			int getPlayerScore() const { return score; }
			void setPlayerSocer(int score) { this->score = score; }
			void addPlayerScore(int increaseNum) { score += increaseNum; }

			NetworkedGame* getGame() { return game; }

		protected:
			void UpdateTimer(float dt);

			NetworkedGame* game;
			int playerNum;
			char btnState[4] = { 0,0,0,0 };

			int score;

			float sprintTimer;
			float fireTimer;

			StateMachine* stateMachine = nullptr;
		};
	}
}

