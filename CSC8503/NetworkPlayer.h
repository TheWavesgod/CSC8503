#pragma once
#include "GameObject.h"
#include "GameClient.h"

namespace NCL {
	namespace CSC8503 {
		class NetworkedGame;

		class NetworkPlayer : public GameObject {
		public:
			static constexpr float SprintCDT = 3.0f;
			static constexpr float FireCDT = 2.0f;

			NetworkPlayer(NetworkedGame* game, int num);
			~NetworkPlayer();

			void OnCollisionBegin(GameObject* otherObject) override;

			void GameTick(float dt);

			int GetPlayerNum() const {
				return playerNum;
			}

			void SetPlayerYaw(const Vector3& pointPos);
			void MovePlayer(bool Up, bool Down, bool Right, bool Left);
			void PlayerSprint();
			void PlayerFire();

			Vector3 getPlayerForwardVector();

			float getSprintCD() const { return sprintTimer < 0 ? 0 : sprintTimer; }
			float getFireCD() const { return fireTimer < 0 ? 0 : fireTimer; }

		protected:
			void UpdateTimer(float dt);

			NetworkedGame* game;
			int playerNum;

			float sprintTimer;
			float fireTimer;
		};
	}
}

