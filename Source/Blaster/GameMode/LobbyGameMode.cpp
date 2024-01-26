// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

//플레이어가 처음 join시 safe access가 가능하도록 한다.
void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//게임 스테이트는 플레이어 스테이트를 보관하는 PlayerArray가 있고, 이것의 사이즈를 반환하여 현재 플레이어 수를 구한다.
	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if(NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if(World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
		}
	}
}
