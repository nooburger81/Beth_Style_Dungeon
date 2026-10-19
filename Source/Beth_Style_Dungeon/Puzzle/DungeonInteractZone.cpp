#include "DungeonInteractZone.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UDungeonInteractZone::UDungeonInteractZone()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	InitBoxExtent(FVector(120.f, 120.f, 100.f));
	SetMobility(EComponentMobility::Movable);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	ShapeColor = FColor(80, 200, 255);
}

void UDungeonInteractZone::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UDungeonInteractZone::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UDungeonInteractZone::HandleEndOverlap);
	RefreshPrompt();
}

void UDungeonInteractZone::SetPrompt(UTextRenderComponent* InPrompt)
{
	Prompt = InPrompt;
	RefreshPrompt();
}

void UDungeonInteractZone::SetPromptText(const FText& NewText)
{
	PromptText = NewText;
	if (Prompt.IsValid())
	{
		Prompt->SetText(NewText);
	}
	RefreshPrompt();
}

void UDungeonInteractZone::SetInteractionEnabled(bool bEnabled)
{
	bInteractionEnabled = bEnabled;
	RefreshPrompt();
}

void UDungeonInteractZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || PawnInside.Get() == Pawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	AActor* Owner = GetOwner();
	if (!PC || !Owner)
	{
		return;
	}

	PawnInside = Pawn;
	Owner->EnableInput(PC);

	// The owner's InputComponent only exists after the first EnableInput call.
	if (!bKeyBound && Owner->InputComponent)
	{
		FInputKeyBinding& Binding = Owner->InputComponent->BindKey(InteractKey, IE_Pressed, this, &UDungeonInteractZone::HandleKeyPressed);
		Binding.bConsumeInput = false;
		bKeyBound = true;
	}

	SetComponentTickEnabled(true);
	RefreshPrompt();
}

void UDungeonInteractZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// A pawn has several components; only leave once none of them overlap.
	if (OtherActor != PawnInside.Get() || IsOverlappingActor(OtherActor))
	{
		return;
	}

	APawn* Pawn = PawnInside.Get();
	PawnInside = nullptr;

	if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		GetOwner()->DisableInput(PC);
	}

	SetComponentTickEnabled(false);
	RefreshPrompt();
}

void UDungeonInteractZone::HandleKeyPressed()
{
	if (bInteractionEnabled && PawnInside.IsValid())
	{
		OnInteract.Broadcast(PawnInside.Get());
	}
}

void UDungeonInteractZone::RefreshPrompt()
{
	if (!Prompt.IsValid())
	{
		return;
	}

	const bool bShow = PawnInside.IsValid() && bInteractionEnabled && !PromptText.IsEmpty();
	Prompt->SetHiddenInGame(!bShow);
}

void UDungeonInteractZone::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Prompt.IsValid() || !PawnInside.IsValid())
	{
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(PawnInside->GetController());
	if (PC && PC->PlayerCameraManager)
	{
		FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - Prompt->GetComponentLocation();
		ToCamera.Z = 0.f;
		if (!ToCamera.IsNearlyZero())
		{
			Prompt->SetWorldRotation(ToCamera.Rotation());
		}
	}
}
