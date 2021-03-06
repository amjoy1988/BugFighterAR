// Fill out your copyright notice in the Description page of Project Settings.

#include "SingleGameLogicActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine.h"
#include "Characters/HeroCharacter.h"
#include "Runtime/Engine/Classes/Components/CapsuleComponent.h"
#include "StateMachine/SingleGame/SingleGameWaitingState.h"
#include "Data/FHeroTableRow.h"
#include "Data/FCharAttackAnimTableRow.h"
#include "Data/FCharAttackComboRowBase.h"
#include "GameLogic/Action/ACTPlayerActionActor.h"
#include "GameTypes.h"

ASingleGameLogicActor::ASingleGameLogicActor() {
	ConstructorHelpers::FObjectFinder<UDataTable> HeroDataTable_BP(TEXT("DataTable'/Game/Project/Blueprints/Data/HeroData.HeroData'"));
	HeroDataTable = HeroDataTable_BP.Object;
	ConstructorHelpers::FObjectFinder<UDataTable> CharAttackAnimDataTable_BP(TEXT("DataTable'/Game/Project/Blueprints/Data/CharAttackAnimDataTable.CharAttackAnimDataTable'"));
	CharAnimDataTable = CharAttackAnimDataTable_BP.Object;
	ConstructorHelpers::FObjectFinder<UDataTable> CharAttackComboDataTable_BP(TEXT("DataTable'/Game/Project/Blueprints/Data/CharAttackComboDataTable.CharAttackComboDataTable'"));
	CharComboDataTable = CharAttackComboDataTable_BP.Object;
}

void ASingleGameLogicActor::BeginPlay()
{
	PlayerPawn = Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	PlayerController = Cast<AARPlayerController>(
		UGameplayStatics::GetPlayerController(GetWorld(), 0));

	Super::BeginPlay();
}

void ASingleGameLogicActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsGameInit) {
		StateMachine->TickMachine();
		if (PlayerAction) {
			PlayerAction->JoystickMove(PlayerController->MovementInput);
		}
	}
}

void ASingleGameLogicActor::InitLogic()
{
	Super::InitLogic();
	ARCoreManager = LoadClass<UObject>(NULL, TEXT("Class'/Game/Project/Blueprints/ARManager.ARManager_C'"));
	ARData = Cast<AARManager>(GetWorld()->SpawnActor(ARCoreManager));

	UClass* InitGameUIBP = LoadClass<UObject>(NULL, TEXT("/Game/Project/Blueprints/UI/InitGameUIBP.InitGameUIBP_C"));
	InitGameUI = CreateWidget<UBaseUserWidget>(GetWorld()->GetFirstPlayerController(), InitGameUIBP);
	InitGameUI->GameLogic = this;
	InitGameUI->AddToViewport();

	SingleGameWaitingState* WaitingState = new SingleGameWaitingState();
	WaitingState->GameLogic = this;
	StateMachine->RegisterState(EGameplayState::Waiting, WaitingState);

	StateMachine->InitMachine(EGameplayState::Waiting);

	IsGameInit = true;
	SetHeroID("Hero_ACT_1");

	//StartGame();
}

AHeroCharacter * ASingleGameLogicActor::CreatPlayerHero(FString HeroResPath, FVector Location, FRotator Rotator)
{
	Super::CreatPlayerHero(HeroResPath, Location, Rotator);
	FVector L(0.f);
	PlayerHero = (UClass*)AssetManager->LoadBPForCAssetMap(HeroResPath);
	AHeroCharacter* HeroChar = GetWorld()->SpawnActor<AHeroCharacter>(PlayerHero, Location, Rotator);

	if (HeroChar) {
		HeroChar->SetActorLocation(FVector::UpVector*HeroChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + HeroChar->GetActorLocation());
		return HeroChar;
	}
	return nullptr;
}

void ASingleGameLogicActor::InitNavMesh(FTransform Center)
{
	if (ARData->GetClass()->ImplementsInterface(UARDataInterface::StaticClass()))
	{
		TArray<FTransform> PlaneTrans = IARDataInterface::Execute_GetMainARWorldCenterTransform(ARData);
		
		CubeActor = LoadClass<UObject>(NULL, TEXT("Class'/Game/Project/Blueprints/PlaneCubeBP.PlaneCubeBP_C'"));

		AStaticMeshActor* CubeTemp = GetWorld()->SpawnActor<AStaticMeshActor>(CubeActor, Center.GetLocation() - FVector::UpVector, Center.GetRotation().Rotator());
		//CubeTemp->SetActorLocationAndRotation(Center.GetLocation() - FVector::UpVector, Center.GetRotation());
		CubeTemp->SetActorScale3D(FVector(10, 10, 0.05f));

		TArray<AActor*> NavMeshs;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), NavMeshs);

		/*CurNavMeshBoundsVolume = GetWorld()->SpawnActor<ANavMeshBoundsVolume>();*/
		CurNavMeshBoundsVolume = (ANavMeshBoundsVolume*)NavMeshs[0];
		CurNavMeshBoundsVolume->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		CurNavMeshBoundsVolume->SetActorLocationAndRotation(Center.GetLocation(), Center.GetRotation());
		CurNavMeshBoundsVolume->SetActorScale3D(FVector(5, 5, 20.f));
		CurNavMeshBoundsVolume->GetRootComponent()->SetMobility(EComponentMobility::Static);

		GetWorld()->GetNavigationSystem()->OnNavigationBoundsUpdated(CurNavMeshBoundsVolume);
	}
}

void ASingleGameLogicActor::StartGame()
{
	InitGameUI->RemoveFromViewport();
	InitGameUI->SetVisibility(ESlateVisibility::Collapsed);
	InitGameUI->SetIsEnabled(false);

//#if !defined(PLATFORM_ANDROID) || !defined(PLATFORM_APPLE)
	if (ARData->GetClass()->ImplementsInterface(UARDataInterface::StaticClass()))
	{
		TArray<FTransform> ARPlaneCenterTrans = IARDataInterface::Execute_GetMainARWorldCenterTransform(ARData);
		ARPlaneCenterTrans.Sort([](const FTransform& A, const FTransform& B) {
			return A.GetScale3D().Size() > B.GetScale3D().Size();
		});

		FTransform Center = ARPlaneCenterTrans[0];
		//Center.SetLocation = (Center.GetLocation - FVector::UpVector * 40.0f);
		InitNavMesh(Center);
		InitHero(Center);
		InitPlayerUI();
		InitPlayerAction();
	}
//#else
//		FTransform Center = FTransform();
//		InitNavMesh(Center);
//		InitHero(Center);
//		InitPlayerUI();
//		InitPlayerAction();
//#endif
}

void ASingleGameLogicActor::InitHero(FTransform location)
{
	Super::InitHero(location);
	static const FString ContextString(TEXT("GENERAL"));
	FHeroTableRow* CurHeroRow = HeroDataTable->FindRow<FHeroTableRow>(FName(*CurHeroID), ContextString);
	//CurPlayerHero = CreatPlayerHero(TEXT("Class'/Game/Project/Blueprints/Hero/HeroCharacter_G4_Skin_1.HeroCharacter_G4_Skin_1_C'"),
	//	location.GetLocation(), FRotator::ZeroRotator);
	if (CurHeroRow) {
		FString Path = CurHeroRow->HeroActorPath.ToString();
		Path.Replace(TEXT("Blueprint"), TEXT("Class"));
		static int idx = 0;
		Path.FindLastChar('\'', idx);
		Path.InsertAt(idx, TEXT("_C"));

		CurPlayerHero = CreatPlayerHero(Path,
			location.GetLocation(), FRotator::ZeroRotator);

		CurHeroType = CurHeroRow->HeroType;
		FCharAttackAnimTableRow* CurAttackAnim = CharAnimDataTable->FindRow<FCharAttackAnimTableRow>(CurHeroRow->AttackAnimRowName, ContextString);
		if (CurAttackAnim) {
			for (TMap<FName, FName>::TIterator It(CurAttackAnim->CharacterAttackAnimMontagePath); It; ++It) {
				UAnimMontage* MontageTemp = Cast<UAnimMontage>(AssetManager->LoadBPAssetMap(It.Value().ToString()));
				CurPlayerHero->CharacterAttackAnimMontageMap.Add(It.Key(), MontageTemp);
			}
		}

		FCharAttackComboRowBase* CurCombAnim = CharComboDataTable->FindRow<FCharAttackComboRowBase>(CurHeroRow->AttackComboRowName, ContextString);

		CurPlayerHero->CharacterAttackComboList = CurCombAnim->CharacterAttackComboList;
			
		GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Red, TEXT("Creat Hero"));
	}
	
}

void ASingleGameLogicActor::InitPlayerUI()
{
	Super::InitPlayerUI();
	TSubclassOf<UGameInputWidget> ActionUI;

	switch (CurHeroType)
	{
	case EHeroType::ACT:
		ActionUI = (UClass*)AssetManager->LoadBPForCAssetMap("Class'/Game/Project/Blueprints/UI/MyACTGameInputWidget.MyACTGameInputWidget_C'");
		if (ActionUI) {
			PlayerUI = CreateWidget<UGameInputWidget>(GetWorld(), ActionUI);
			PlayerUI->GameLogic = this;
			PlayerUI->AddToViewport();
		}
		PlayerController->InitJoyStick();
		PlayerController->SetJoyStickActive(true);
		break;
	case EHeroType::MOBA:

		break;
	default:
		break;
	}
}

void ASingleGameLogicActor::InitPlayerAction(){
	Super::InitPlayerAction();
	//if (PlayerAction) {
	//	Destroy(PlayerAction);
	//}
	switch (CurHeroType)
	{
	case EHeroType::ACT:
		PlayerAction = GetWorld()->SpawnActor<AACTPlayerActionActor>();
		PlayerAction->GameLogic = this;
		break;
	case EHeroType::MOBA:

		break;
	default:
		break;
	}
}

void ASingleGameLogicActor::TapPressed(FHitResult Result)
{
	Super::TapPressed(Result);
	if (PlayerAction) {
		PlayerAction->TapPressed(Result);
	}
}

