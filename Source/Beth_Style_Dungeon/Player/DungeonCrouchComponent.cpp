#include "DungeonCrouchComponent.h"
#include "../Beth_Style_Dungeon.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"

UDungeonCrouchComponent::UDungeonCrouchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDungeonCrouchComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->GetCharacterMovement())
	{
		UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: Dungeon Crouch only works on a character."), *GetOwner()->GetName());
		return;
	}

	Character->GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	if (bLowerViewWhileCrouched)
	{
		TArray<USceneComponent*> Components;
		Character->GetComponents(Components);
		for (USceneComponent* Component : Components)
		{
			if (Component->GetFName() == ViewMeshName)
			{
				ViewMesh = Component;
				break;
			}
		}

		if (ViewMesh)
		{
			ViewMeshDefaultLocation = ViewMesh->GetRelativeLocation();
			// If the view mesh is the body Mesh itself, setting its location below replaces the engine's offset.
			bViewMeshFollowsBodyMesh = ViewMesh != Character->GetMesh() && ViewMesh->IsAttachedTo(Character->GetMesh());
			// Run after movement so the view settles in the same frame the capsule changes.
			AddTickPrerequisiteComponent(Character->GetCharacterMovement());
			SetComponentTickEnabled(true);
		}
		else
		{
			UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: no component named '%s' to lower while crouched."), *Character->GetName(), *ViewMeshName.ToString());
		}
	}

	// The pawn may be possessed before or after BeginPlay (respawns), so handle both.
	Character->ReceiveControllerChangedDelegate.AddDynamic(this, &UDungeonCrouchComponent::HandleControllerChanged);
	BindToController(Character->GetController());
}

void UDungeonCrouchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromController();
	Super::EndPlay(EndPlayReason);
}

void UDungeonCrouchComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	UnbindFromController();
	BindToController(NewController);
}

void UDungeonCrouchComponent::BindToController(AController* Controller)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController() || !CrouchKey.IsValid())
	{
		return;
	}

	if (!CrouchInput)
	{
		// Own input component so it doesn't depend on the character Blueprint's input setup.
		CrouchInput = NewObject<UInputComponent>(GetOwner(), UInputSettings::GetDefaultInputComponentClass(), TEXT("CrouchInput"));
		CrouchInput->RegisterComponent();
		CrouchInput->BindKey(CrouchKey, IE_Pressed, this, &UDungeonCrouchComponent::StartCrouch).bConsumeInput = false;
		CrouchInput->BindKey(CrouchKey, IE_Released, this, &UDungeonCrouchComponent::StopCrouch).bConsumeInput = false;
	}

	PC->PushInputComponent(CrouchInput);
	BoundController = PC;
}

void UDungeonCrouchComponent::UnbindFromController()
{
	if (BoundController.IsValid() && CrouchInput)
	{
		BoundController->PopInputComponent(CrouchInput);
	}
	BoundController = nullptr;
}

void UDungeonCrouchComponent::StartCrouch()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->Crouch();
	}
}

void UDungeonCrouchComponent::StopCrouch()
{
	// Character Movement keeps trying to stand until there's headroom.
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->UnCrouch();
	}
}

bool UDungeonCrouchComponent::IsCrouching() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character && Character->bIsCrouched;
}

float UDungeonCrouchComponent::GetCrouchDrop() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const ACharacter* Default = Character ? GetDefault<ACharacter>(Character->GetClass()) : nullptr;
	if (!Default || !Default->GetCapsuleComponent())
	{
		return 0.f;
	}

	const float HalfHeightLost = Default->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - Character->GetCharacterMovement()->GetCrouchedHalfHeight();
	return FMath::Max(0.f, HalfHeightLost * Character->GetCapsuleComponent()->GetShapeScale());
}

void UDungeonCrouchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ViewMesh || !ViewMesh->GetAttachParent())
	{
		return;
	}

	if (!bLowerViewWhileCrouched)
	{
		ViewMesh->SetRelativeLocation(ViewMeshDefaultLocation);
		SetComponentTickEnabled(false);
		return;
	}

	const float Drop = GetCrouchDrop();
	ViewDrop = FMath::FInterpTo(ViewDrop, IsCrouching() ? Drop : 0.f, DeltaTime, ViewLowerSpeed);

	// A mesh on the capsule already drops instantly with it; a mesh under the body Mesh is held at standing
	// height by the engine. Either way, offset it so the view sinks by ViewDrop and no more.
	const float AlreadyDropped = (!bViewMeshFollowsBodyMesh && IsCrouching()) ? Drop : 0.f;
	const FVector WorldOffset(0.f, 0.f, AlreadyDropped - ViewDrop);
	const FVector LocalOffset = ViewMesh->GetAttachParent()->GetComponentTransform().InverseTransformVector(WorldOffset);
	ViewMesh->SetRelativeLocation(ViewMeshDefaultLocation + LocalOffset);
}
