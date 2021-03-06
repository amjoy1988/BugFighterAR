// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine.h"
#include "Characters/HeroCharacter.h"
#include "AR/ARDataInterface.h"
#include "AR/ARManager.h"
#include "Characters/HeroCharacter.h"
#include "Runtime/Engine/Classes/AI/Navigation/NavMeshBoundsVolume.h"
#include "GameLogic/GameAssetManagerActor.h"
#include "StateMachine/FSMMachine.h"
#include "StateMachine/FSMState.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "GameTypes.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "GameLogic/Action/BasePlayerActionActor.h"
#include "PlayerPawn.h"
#include "BaseGameLogicActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArScanedEvent, bool, IsScaned);

UCLASS()
class ROBOTBURST_API ABaseGameLogicActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseGameLogicActor();

	UPROPERTY()
		class AGameAssetManagerActor* AssetManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


public:

	UPROPERTY()
	bool IsGameInit = false;

	//AR数据接口
	AARManager* ARData;

	UPROPERTY()
	//AR管理类
	TSubclassOf<AARManager> ARCoreManager;

	UPROPERTY()
	//玩家英雄类型模板
	TSubclassOf<AHeroCharacter> PlayerHero;
		
	UPROPERTY()
	TEnumAsByte<EHeroType::Type> CurHeroType;

	UPROPERTY()
	class APlayerPawn* PlayerPawn;

	UPROPERTY()
	//当前玩家英雄
	AHeroCharacter* CurPlayerHero;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	class ABasePlayerActionActor* PlayerAction;

	UPROPERTY()
	FString CurHeroID;

	UPROPERTY()
	//寻路地图集合
	ANavMeshBoundsVolume* CurNavMeshBoundsVolume;

	//状态机
	FSMMachine* StateMachine;

	UPROPERTY()
	UDataTable* HeroDataTable;

	UPROPERTY()
		UDataTable* CharAnimDataTable;

	UPROPERTY()
		UDataTable* CharComboDataTable;

	UFUNCTION()
	virtual AHeroCharacter* CreatPlayerHero(FString HeroResPath, FVector Location, FRotator Rotator);
	
	UFUNCTION()
	virtual void InitLogic();

	UFUNCTION()
	virtual void ReleaseLogic();

	UFUNCTION(BlueprintCallable)
	virtual void StartGame();

	UFUNCTION(BlueprintCallable)
	virtual void SetHeroID(FString ID);

	UFUNCTION()
	virtual void InitHero(FTransform location);

	UFUNCTION()
	virtual void InitPlayerUI();

	UFUNCTION()
	virtual void InitPlayerAction();

	UPROPERTY(BlueprintAssignable)
	FArScanedEvent ArScanedEvent;

	UFUNCTION()
	virtual void TapPressed(FHitResult Result);

};
