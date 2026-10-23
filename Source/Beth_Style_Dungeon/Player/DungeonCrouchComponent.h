#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "DungeonCrouchComponent.generated.h"

class AController;
class APlayerController;
class UInputComponent;

/**
 * Add to the player character. Hold CrouchKey to crouch: the capsule shrinks to the Character Movement's
 * Crouched Half Height and speed drops to Max Walk Speed Crouched. The character stands back up on release,
 * once there's room above.
 *
 * Until there are crouch animations, Lower View While Crouched smoothly sinks the first-person mesh (and the
 * camera on its head) by the height the capsule loses. Turn it off once the Anim Blueprint has a crouch
 * state driven by Is Crouching.
 */
UCLASS(ClassGroup = (Dungeon), meta = (BlueprintSpawnableComponent, DisplayName = "Dungeon Crouch"))
class BETH_STYLE_DUNGEON_API UDungeonCrouchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDungeonCrouchComponent();

	/** Hold to crouch. Leave as None to only drive it from Blueprint (Crouch / Un Crouch on the character). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crouch")
	FKey CrouchKey = EKeys::C;

	/** Stopgap for missing crouch animations: sink the first-person mesh and camera while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch|View")
	bool bLowerViewWhileCrouched = true;

	/** The mesh the camera is attached to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crouch|View", meta = (EditCondition = "bLowerViewWhileCrouched"))
	FName ViewMeshName = TEXT("FirstPersonMesh");

	/** How fast the view moves down and up. Higher is snappier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch|View", meta = (EditCondition = "bLowerViewWhileCrouched", ClampMin = "0.1"))
	float ViewLowerSpeed = 10.f;

	/** True while crouched. Read this in the Anim Blueprint to switch to a crouch pose. */
	UFUNCTION(BlueprintPure, Category = "Crouch")
	bool IsCrouching() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void BindToController(AController* Controller);
	void UnbindFromController();

	void StartCrouch();
	void StopCrouch();

	/** World-space height the capsule loses when crouching. */
	float GetCrouchDrop() const;

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> CrouchInput;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ViewMesh;

	TWeakObjectPtr<APlayerController> BoundController;
	FVector ViewMeshDefaultLocation = FVector::ZeroVector;
	/** True if the view mesh hangs off the character's Mesh, which the engine keeps at standing height while crouched. */
	bool bViewMeshFollowsBodyMesh = false;
	float ViewDrop = 0.f;
};
