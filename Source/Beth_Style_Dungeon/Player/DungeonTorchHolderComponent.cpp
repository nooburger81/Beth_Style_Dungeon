#include "DungeonTorchHolderComponent.h"
#include "../Beth_Style_Dungeon.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"

UDungeonTorchHolderComponent::UDungeonTorchHolderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonTorchHolderComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: Dungeon Torch Holder only works on a pawn or character."), *GetOwner()->GetName());
		return;
	}

	// The pawn may be possessed before or after BeginPlay (respawns), so handle both.
	Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UDungeonTorchHolderComponent::HandleControllerChanged);
	BindToController(Pawn->GetController());
}

void UDungeonTorchHolderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromController();
	if (IsValid(TorchActor))
	{
		TorchActor->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

void UDungeonTorchHolderComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	UnbindFromController();
	BindToController(NewController);
}

void UDungeonTorchHolderComponent::BindToController(AController* Controller)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController() || !ToggleKey.IsValid())
	{
		return;
	}

	if (!TorchInput)
	{
		// Own input component so it doesn't depend on the character Blueprint's input setup.
		TorchInput = NewObject<UInputComponent>(GetOwner(), UInputSettings::GetDefaultInputComponentClass(), TEXT("TorchHolderInput"));
		TorchInput->RegisterComponent();
		FInputKeyBinding& Binding = TorchInput->BindKey(ToggleKey, IE_Pressed, this, &UDungeonTorchHolderComponent::ToggleTorch);
		Binding.bConsumeInput = false;
	}

	PC->PushInputComponent(TorchInput);
	BoundController = PC;
}

void UDungeonTorchHolderComponent::UnbindFromController()
{
	if (BoundController.IsValid() && TorchInput)
	{
		BoundController->PopInputComponent(TorchInput);
	}
	BoundController = nullptr;
}

void UDungeonTorchHolderComponent::SetHoldingTorch(bool bHold)
{
	if (bHoldingTorch == bHold)
	{
		return;
	}

	bHoldingTorch = bHold;
	PlayPose(bHold);
	ShowProp(bHold);
	OnTorchToggled.Broadcast(bHoldingTorch);
}

void UDungeonTorchHolderComponent::GetFirstPersonHandTarget(FVector& HandLocation, FRotator& HandRotation, FVector& ElbowLocation, float& Alpha) const
{
	Alpha = 0.f;
	const UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!Camera)
	{
		HandLocation = ElbowLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		HandRotation = FRotator::ZeroRotator;
		return;
	}

	const FTransform CameraTransform = Camera->GetComponentTransform();
	const FTransform Hand = HandHoldOffset * CameraTransform;
	HandLocation = Hand.GetLocation();
	HandRotation = Hand.Rotator();
	ElbowLocation = CameraTransform.TransformPosition(ElbowHintOffset);

	Alpha = bHoldingTorch ? 1.f : 0.f;
	if (HoldTorchMontage)
	{
		const USkeletalMeshComponent* Mesh = FindSkeletalMesh(AnimatedMeshName);
		if (UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr)
		{
			// The active instance is gone once blending out starts, so fall back to the fading one.
			const FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(HoldTorchMontage);
			if (!Instance)
			{
				Instance = AnimInstance->GetInstanceForMontage(HoldTorchMontage);
			}
			Alpha = Instance ? Instance->GetWeight() : 0.f;
		}
	}
}

USkeletalMeshComponent* UDungeonTorchHolderComponent::FindSkeletalMesh(FName ComponentName) const
{
	if (!ComponentName.IsNone())
	{
		TArray<USkeletalMeshComponent*> Meshes;
		GetOwner()->GetComponents(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh->GetFName() == ComponentName)
			{
				return Mesh;
			}
		}
	}

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}
	return GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
}

void UDungeonTorchHolderComponent::PlayPose(bool bHold)
{
	if (!HoldTorchMontage)
	{
		return;
	}

	const USkeletalMeshComponent* Mesh = FindSkeletalMesh(AnimatedMeshName);
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: no Anim Blueprint found to play the Hold Torch montage."), *GetOwner()->GetName());
		return;
	}

	if (!bHold)
	{
		AnimInstance->Montage_Stop(BlendOutTime, HoldTorchMontage);
		return;
	}

	if (AnimInstance->Montage_Play(HoldTorchMontage) > 0.f && HoldTorchMontage->CompositeSections.Num() > 0)
	{
		// Hold the pose until told to stop, whatever the montage's own section links say.
		const FName Section = HoldTorchMontage->GetSectionName(0);
		AnimInstance->Montage_SetNextSection(Section, Section, HoldTorchMontage);
	}
}

void UDungeonTorchHolderComponent::ShowProp(bool bHold)
{
	if (TorchClass)
	{
		if (!bHold)
		{
			if (IsValid(TorchActor))
			{
				TorchActor->Destroy();
			}
			TorchActor = nullptr;
			return;
		}

		USkeletalMeshComponent* Hand = FindSkeletalMesh(AttachMeshName);
		if (!Hand)
		{
			return;
		}

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TorchActor = GetWorld()->SpawnActor<AActor>(TorchClass, Hand->GetSocketTransform(HandSocketName), Params);
		if (TorchActor)
		{
			TorchActor->SetActorEnableCollision(false);
			TorchActor->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);
			TorchActor->GetRootComponent()->SetRelativeTransform(TorchOffset);

			// Match the arms' first-person FOV and scale, or the torch drifts away from the hand on screen.
			TArray<UPrimitiveComponent*> Primitives;
			TorchActor->GetComponents(Primitives);
			for (UPrimitiveComponent* Primitive : Primitives)
			{
				Primitive->SetFirstPersonPrimitiveType(Hand->FirstPersonPrimitiveType);
			}
		}
		return;
	}

	if (!TorchMesh)
	{
		return;
	}

	if (!TorchMeshComponent && bHold)
	{
		USkeletalMeshComponent* Hand = FindSkeletalMesh(AttachMeshName);
		if (!Hand)
		{
			return;
		}
		if (!Hand->DoesSocketExist(HandSocketName))
		{
			UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: mesh '%s' has no socket or bone '%s'; the torch will sit at its root."),
				*GetOwner()->GetName(), *Hand->GetName(), *HandSocketName.ToString());
		}

		TorchMeshComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("HeldTorchMesh"));
		TorchMeshComponent->SetStaticMesh(TorchMesh);
		TorchMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Match the arms so the torch gets the same first-person FOV and doesn't clip into walls differently.
		TorchMeshComponent->SetFirstPersonPrimitiveType(Hand->FirstPersonPrimitiveType);
		TorchMeshComponent->SetupAttachment(Hand, HandSocketName);
		TorchMeshComponent->SetRelativeTransform(TorchOffset);
		TorchMeshComponent->RegisterComponent();

		TorchLight = NewObject<UPointLightComponent>(GetOwner(), TEXT("HeldTorchLight"));
		TorchLight->SetupAttachment(TorchMeshComponent);
		TorchLight->SetRelativeLocation(LightOffset);
		TorchLight->SetIntensity(LightIntensity);
		TorchLight->SetAttenuationRadius(LightRadius);
		TorchLight->SetLightColor(LightColor);
		TorchLight->RegisterComponent();
	}

	if (TorchMeshComponent)
	{
		TorchMeshComponent->SetVisibility(bHold, true);
	}
}
