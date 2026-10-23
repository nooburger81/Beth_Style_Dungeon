#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "DungeonTorchHolderComponent.generated.h"

class AController;
class APlayerController;
class UAnimMontage;
class UInputComponent;
class UPointLightComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonTorchSignature, bool, bIsHoldingTorch);

/**
 * Add to the player character. Pressing ToggleKey raises or lowers a torch: a looping Hold Torch
 * montage plays on the body mesh and a torch (Torch Class, or Torch Mesh plus a light) appears in the hand.
 *
 * The montage must use a slot the Anim Blueprint plays (ABP_Unarmed has DefaultSlot). For an upper-body-only
 * pose that keeps walk legs, use an UpperBody slot behind a Layered Blend Per Bone in the Anim Blueprint,
 * or read Is Holding Torch there and blend the pose yourself.
 */
UCLASS(ClassGroup = (Dungeon), meta = (BlueprintSpawnableComponent, DisplayName = "Dungeon Torch Holder"))
class BETH_STYLE_DUNGEON_API UDungeonTorchHolderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDungeonTorchHolderComponent();

	/** Key that raises/lowers the torch. Leave as None to only drive it from Blueprint (e.g. an Input Action). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch")
	FKey ToggleKey = EKeys::T;

	/** Looping pose played while the torch is held. The first section is set to loop on itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Animation")
	TObjectPtr<UAnimMontage> HoldTorchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Animation", meta = (ClampMin = "0", Units = "s"))
	float BlendOutTime = 0.25f;

	/** Skeletal mesh that plays the montage. None uses the character's Mesh (the first-person arms copy its pose). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch|Animation")
	FName AnimatedMeshName = NAME_None;

	/** Actor spawned in the hand while held (a Blueprint with a mesh, light and fire). Takes priority over Torch Mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Prop")
	TSubclassOf<AActor> TorchClass;

	/** Used when no Torch Class is set: this mesh is shown in the hand with a point light on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Prop")
	TObjectPtr<UStaticMesh> TorchMesh;

	/** Skeletal mesh the torch is attached to. Falls back to the character's Mesh if not found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch|Prop")
	FName AttachMeshName = TEXT("FirstPersonMesh");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch|Prop")
	FName HandSocketName = TEXT("hand_l");

	/** Offset of the torch from the hand socket. Tweak in Play mode with the details panel, then copy it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Prop")
	FTransform TorchOffset;

	/** Where the flame sits on Torch Mesh, relative to the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Light", meta = (EditCondition = "TorchMesh != nullptr"))
	FVector LightOffset = FVector(0.f, 0.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Light", meta = (EditCondition = "TorchMesh != nullptr", ClampMin = "0"))
	float LightIntensity = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Light", meta = (EditCondition = "TorchMesh != nullptr", ClampMin = "0"))
	float LightRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|Light", meta = (EditCondition = "TorchMesh != nullptr"))
	FLinearColor LightColor = FLinearColor(1.f, 0.55f, 0.2f);

	/**
	 * Where the hand holding the torch sits in front of the camera (X forward, Y right, Z up, in cm).
	 * Feed Get First Person Hand Target into a Two Bone IK node in the first-person Anim Blueprint to use it.
	 * Tweak in Play mode with the details panel, then copy it back.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|First Person")
	FTransform HandHoldOffset = FTransform(FVector(40.f, -20.f, -18.f));

	/** Where the elbow points, relative to the camera. Keeps the arm bending outward and down instead of flipping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torch|First Person")
	FVector ElbowHintOffset = FVector(10.f, -50.f, -50.f);

	UPROPERTY(BlueprintAssignable, Category = "Torch")
	FDungeonTorchSignature OnTorchToggled;

	/**
	 * World-space IK targets for the hand and elbow, placed in front of the camera. Alpha follows the
	 * Hold Torch montage's blend in and out, so plug it into the IK node's Alpha.
	 */
	UFUNCTION(BlueprintPure, Category = "Torch|First Person")
	void GetFirstPersonHandTarget(FVector& HandLocation, FRotator& HandRotation, FVector& ElbowLocation, float& Alpha) const;

	UFUNCTION(BlueprintCallable, Category = "Torch")
	void SetHoldingTorch(bool bHold);

	UFUNCTION(BlueprintCallable, Category = "Torch")
	void ToggleTorch() { SetHoldingTorch(!bHoldingTorch); }

	/** Read this in the Anim Blueprint (Try Get Pawn Owner > Get Component By Class) to blend a pose yourself. */
	UFUNCTION(BlueprintPure, Category = "Torch")
	bool IsHoldingTorch() const { return bHoldingTorch; }

	UFUNCTION(BlueprintPure, Category = "Torch")
	AActor* GetTorchActor() const { return TorchActor; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void BindToController(AController* Controller);
	void UnbindFromController();

	USkeletalMeshComponent* FindSkeletalMesh(FName ComponentName) const;
	void PlayPose(bool bHold);
	void ShowProp(bool bHold);

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> TorchInput;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TorchActor;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TorchMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> TorchLight;

	TWeakObjectPtr<APlayerController> BoundController;
	bool bHoldingTorch = false;
};
