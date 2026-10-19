#include "DungeonValve.h"
#include "DungeonInteractZone.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ADungeonValve::ADungeonValve()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultTurnSound(TEXT("/Game/LevelDesignKit/Audio/SFX_LeverPulled.SFX_LeverPulled"));

	// Placeholder shapes: a stem sticking out of the wall (-X) and a flat wheel facing +X.
	StemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StemMesh"));
	StemMesh->SetupAttachment(SceneRoot);
	StemMesh->SetMobility(EComponentMobility::Movable);
	StemMesh->SetRelativeLocation(FVector(-20.f, 0.f, 0.f));
	StemMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	StemMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.4f));

	WheelPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WheelPivot"));
	WheelPivot->SetupAttachment(SceneRoot);

	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh"));
	WheelMesh->SetupAttachment(WheelPivot);
	WheelMesh->SetMobility(EComponentMobility::Movable);
	WheelMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	WheelMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.08f));

	if (CylinderMesh.Succeeded())
	{
		StemMesh->SetStaticMesh(CylinderMesh.Object);
		WheelMesh->SetStaticMesh(CylinderMesh.Object);
	}

	InteractZone = CreateDefaultSubobject<UDungeonInteractZone>(TEXT("InteractZone"));
	InteractZone->SetupAttachment(SceneRoot);
	InteractZone->SetRelativeLocation(FVector(90.f, 0.f, 0.f));
	InteractZone->SetBoxExtent(FVector(100.f, 100.f, 120.f));

	PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptText->SetupAttachment(SceneRoot);
	PromptText->SetMobility(EComponentMobility::Movable);
	PromptText->SetRelativeLocation(FVector(10.f, 0.f, 55.f));
	PromptText->SetHorizontalAlignment(EHTA_Center);
	PromptText->SetVerticalAlignment(EVRTA_TextCenter);
	PromptText->SetWorldSize(14.f);
	PromptText->SetHiddenInGame(true);

	if (DefaultTurnSound.Succeeded())
	{
		TurnSound = DefaultTurnSound.Object;
	}

	PromptTurnOn = NSLOCTEXT("DungeonPuzzle", "ValveTurnOn", "Press E to turn the valve");
	PromptTurnOff = NSLOCTEXT("DungeonPuzzle", "ValveTurnOff", "Press E to turn the valve back");
}

void ADungeonValve::BeginPlay()
{
	Super::BeginPlay();

	InteractZone->SetPrompt(PromptText);
	InteractZone->OnInteract.AddDynamic(this, &ADungeonValve::HandleInteract);
	UpdatePrompt();
}

void ADungeonValve::HandleInteract(APawn* Interactor)
{
	Use(Interactor);
}

void ADungeonValve::Use(AActor* User)
{
	if (bTurning || !CanSwitchTo(!bIsOn))
	{
		return;
	}

	bTurning = true;
	bTurningOn = !bIsOn;
	TurnElapsed = 0.f;
	StartAngle = CurrentAngle;
	PendingUser = User;

	if (TurnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TurnSound, WheelPivot->GetComponentLocation());
	}

	InteractZone->SetInteractionEnabled(false);
	SetActorTickEnabled(true);

	if (!bSignalWhenTurnFinishes)
	{
		SetOn(bTurningOn, User);
	}
}

void ADungeonValve::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bTurning)
	{
		return;
	}

	TurnElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(TurnElapsed / TurnDuration, 0.f, 1.f);
	const float EndAngle = bTurningOn ? TurnAngle : 0.f;
	CurrentAngle = FMath::Lerp(StartAngle, EndAngle, FMath::SmoothStep(0.f, 1.f, Alpha));
	WheelPivot->SetRelativeRotation(FRotator(0.f, 0.f, CurrentAngle));

	if (Alpha >= 1.f)
	{
		bTurning = false;
		SetActorTickEnabled(false);

		if (bSignalWhenTurnFinishes)
		{
			SetOn(bTurningOn, PendingUser.Get());
		}
		UpdatePrompt();
	}
}

void ADungeonValve::UpdatePrompt()
{
	const bool bCanUse = !bTurning && CanSwitchTo(!bIsOn);
	InteractZone->SetPromptText(bIsOn ? PromptTurnOff : PromptTurnOn);
	InteractZone->SetInteractionEnabled(bCanUse);
}
