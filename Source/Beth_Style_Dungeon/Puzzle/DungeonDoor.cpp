#include "DungeonDoor.h"
#include "DungeonInteractZone.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADungeonDoor::ADungeonDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDoorMesh(TEXT("/Game/LevelDesignKit/Meshes/Architecture/SM_DoorFantasy02.SM_DoorFantasy02"));
	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultOpenSound(TEXT("/Game/Blueprints/universfield-creaking-door-03-487855.universfield-creaking-door-03-487855"));
	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultUnlockSound(TEXT("/Game/LevelDesignKit/Audio/SFX_Lock_Unlocked.SFX_Lock_Unlocked"));
	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultLockedSound(TEXT("/Game/LevelDesignKit/Audio/SFX_Door_Locked.SFX_Door_Locked"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(SceneRoot);
	FrameMesh->SetMobility(EComponentMobility::Movable);

	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(SceneRoot);
	DoorPivot->SetMobility(EComponentMobility::Movable);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);
	DoorMesh->SetMobility(EComponentMobility::Movable);
	if (DefaultDoorMesh.Succeeded())
	{
		DoorMesh->SetStaticMesh(DefaultDoorMesh.Object);
	}

	InteractZone = CreateDefaultSubobject<UDungeonInteractZone>(TEXT("InteractZone"));
	InteractZone->SetupAttachment(SceneRoot);
	InteractZone->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	InteractZone->SetBoxExtent(FVector(160.f, 160.f, 120.f));

	PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptText->SetupAttachment(SceneRoot);
	PromptText->SetMobility(EComponentMobility::Movable);
	PromptText->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	PromptText->SetHorizontalAlignment(EHTA_Center);
	PromptText->SetVerticalAlignment(EVRTA_TextCenter);
	PromptText->SetWorldSize(16.f);
	PromptText->SetHiddenInGame(true);

	OpenSound = DefaultOpenSound.Object;
	UnlockSound = DefaultUnlockSound.Object;
	LockedSound = DefaultLockedSound.Object;

	OpenPrompt = NSLOCTEXT("DungeonPuzzle", "DoorOpen", "Press E to open");
	ClosePrompt = NSLOCTEXT("DungeonPuzzle", "DoorClose", "Press E to close");
	LockedPrompt = NSLOCTEXT("DungeonPuzzle", "DoorLocked", "Locked");
}

void ADungeonDoor::BeginPlay()
{
	Super::BeginPlay();

	PivotClosedLocation = DoorPivot->GetRelativeLocation();
	PivotClosedRotation = DoorPivot->GetRelativeRotation();

	bLocked = bStartsLocked;
	bOpen = bStartsOpen;
	OpenAlpha = bOpen ? 1.f : 0.f;
	ApplyAlpha(OpenAlpha);

	InteractZone->SetPrompt(PromptText);
	InteractZone->OnInteract.AddDynamic(this, &ADungeonDoor::HandleInteract);
	UpdatePrompt();
}

void ADungeonDoor::HandleInteract(APawn* Interactor)
{
	if (bLocked)
	{
		PlayDoorSound(LockedSound);
		return;
	}

	if (bOpen)
	{
		Close();
	}
	else
	{
		Open(Interactor);
	}
}

void ADungeonDoor::Unlock(bool bPlaySound)
{
	if (!bLocked)
	{
		return;
	}

	bLocked = false;
	if (bPlaySound)
	{
		PlayDoorSound(UnlockSound);
	}
	OnUnlocked.Broadcast(this);
	UpdatePrompt();
}

void ADungeonDoor::Lock()
{
	bLocked = true;
	UpdatePrompt();
}

void ADungeonDoor::Open(AActor* Opener)
{
	GetWorldTimerManager().ClearTimer(OpenDelayTimer);
	if (bOpen)
	{
		return;
	}

	// Pick the swing direction only when starting from fully closed so it never reverses mid-swing.
	if (OpenAlpha <= 0.f)
	{
		SwingSign = 1.f;
		if (bSwingAwayFromPlayer && Opener)
		{
			const FVector ToOpener = Opener->GetActorLocation() - GetActorLocation();
			SwingSign = FVector::DotProduct(ToOpener, GetActorForwardVector()) >= 0.f ? 1.f : -1.f;
		}
	}

	bOpen = true;
	PlayDoorSound(OpenSound);
	SetActorTickEnabled(true);
	OnOpened.Broadcast(this);
	UpdatePrompt();
}

void ADungeonDoor::Close()
{
	GetWorldTimerManager().ClearTimer(OpenDelayTimer);
	if (!bOpen)
	{
		return;
	}

	bOpen = false;
	PlayDoorSound(CloseSound ? CloseSound.Get() : OpenSound.Get());
	SetActorTickEnabled(true);
	OnClosed.Broadcast(this);
	UpdatePrompt();
}

void ADungeonDoor::OnPuzzleActivated_Implementation(AActor* Activator)
{
	const bool bWasLocked = bLocked;
	Unlock(true);

	if (SignalResponse != EDungeonDoorSignalResponse::UnlockAndOpen)
	{
		return;
	}

	if (bWasLocked && OpenDelayAfterUnlock > 0.f)
	{
		GetWorldTimerManager().SetTimer(OpenDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Open(nullptr); }), OpenDelayAfterUnlock, false);
	}
	else
	{
		Open(nullptr);
	}
}

void ADungeonDoor::OnPuzzleDeactivated_Implementation(AActor* Activator)
{
	if (!bRelockWhenDeactivated)
	{
		return;
	}

	Close();
	Lock();
}

void ADungeonDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bOpen ? 1.f : 0.f;
	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, Target, DeltaSeconds, 1.f / OpenDuration);
	ApplyAlpha(OpenAlpha);

	if (FMath::IsNearlyEqual(OpenAlpha, Target))
	{
		SetActorTickEnabled(false);
	}
}

void ADungeonDoor::ApplyAlpha(float Alpha)
{
	const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);

	if (Motion == EDungeonDoorMotion::Swing)
	{
		DoorPivot->SetRelativeLocation(PivotClosedLocation);
		DoorPivot->SetRelativeRotation(PivotClosedRotation + FRotator(0.f, SwingAngle * SwingSign * Eased, 0.f));
	}
	else
	{
		DoorPivot->SetRelativeRotation(PivotClosedRotation);
		DoorPivot->SetRelativeLocation(PivotClosedLocation + SlideOffset * Eased);
	}
}

void ADungeonDoor::UpdatePrompt()
{
	if (!bPlayerCanUse)
	{
		InteractZone->SetInteractionEnabled(false);
		return;
	}

	InteractZone->SetInteractionEnabled(true);
	InteractZone->SetPromptText(bLocked ? LockedPrompt : (bOpen ? ClosePrompt : OpenPrompt));
}

void ADungeonDoor::PlayDoorSound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, DoorMesh->GetComponentLocation());
	}
}
