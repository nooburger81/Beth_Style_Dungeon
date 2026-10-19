#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonActivatable.h"
#include "DungeonDoor.generated.h"

class UDungeonInteractZone;
class UStaticMeshComponent;
class UTextRenderComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EDungeonDoorMotion : uint8
{
	/** Rotates around the DoorPivot like a normal hinged door. */
	Swing,
	/** Moves by Slide Offset: use Z for a portcullis or a secret wall that rises. */
	Slide
};

UENUM(BlueprintType)
enum class EDungeonDoorSignalResponse : uint8
{
	/** Unlocks with a sound; the player then opens it with E. */
	Unlock,
	/** Unlocks (if locked) and opens by itself. */
	UnlockAndOpen
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonDoorSignature, ADungeonDoor*, Door);

/**
 * A door that can be locked, opened by the player, or driven by triggers. Swap the meshes on
 * DoorMesh / FrameMesh. For hinged doors, move DoorMesh so its hinge edge sits on DoorPivot.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Door"))
class BETH_STYLE_DUNGEON_API ADungeonDoor : public AActor, public IDungeonActivatable
{
	GENERATED_BODY()

public:
	ADungeonDoor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EDungeonDoorMotion Motion = EDungeonDoorMotion::Swing;

	/** Degrees to swing open. Use a negative value to swing the other way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "Motion == EDungeonDoorMotion::Swing", EditConditionHides))
	float SwingAngle = 90.f;

	/** When the player opens it from the back side, swing the other way so it opens away from them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "Motion == EDungeonDoorMotion::Swing", EditConditionHides))
	bool bSwingAwayFromPlayer = true;

	/** How far the door moves when open, relative to the actor. (0, 0, 400) rises 4 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "Motion == EDungeonDoorMotion::Slide", EditConditionHides))
	FVector SlideOffset = FVector(0.f, 0.f, 400.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.05", Units = "s"))
	float OpenDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bStartsLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bStartsOpen = false;

	/** Can the player open/close it with E? Turn off for secret walls that only puzzles move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bPlayerCanUse = true;

	/** What happens when a linked trigger turns on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Puzzle")
	EDungeonDoorSignalResponse SignalResponse = EDungeonDoorSignalResponse::UnlockAndOpen;

	/** Pause between the unlock sound and the door starting to move (Unlock And Open only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Puzzle", meta = (ClampMin = "0", Units = "s"))
	float OpenDelayAfterUnlock = 0.6f;

	/** When a linked trigger turns back off, close the door and lock it again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Puzzle")
	bool bRelockWhenDeactivated = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Sound")
	TObjectPtr<USoundBase> OpenSound;

	/** Leave empty to reuse Open Sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Sound")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Sound")
	TObjectPtr<USoundBase> UnlockSound;

	/** Played when the player tries a locked door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Sound")
	TObjectPtr<USoundBase> LockedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Prompt")
	FText OpenPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Prompt")
	FText ClosePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Prompt")
	FText LockedPrompt;

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FDungeonDoorSignature OnUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FDungeonDoorSignature OnOpened;

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FDungeonDoorSignature OnClosed;

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Unlock(bool bPlaySound = true);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Lock();

	/** Opens even if locked; use Unlock first if the lock should matter. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Open(AActor* Opener = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Close();

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bOpen; }

	virtual void OnPuzzleActivated_Implementation(AActor* Activator) override;
	virtual void OnPuzzleDeactivated_Implementation(AActor* Activator) override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleInteract(APawn* Interactor);

	void ApplyAlpha(float Alpha);
	void UpdatePrompt();
	void PlayDoorSound(USoundBase* Sound) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Optional static frame/arch around the door. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	/** The hinge (Swing) or the part that slides (Slide). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDungeonInteractZone> InteractZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> PromptText;

private:
	FTimerHandle OpenDelayTimer;
	FVector PivotClosedLocation = FVector::ZeroVector;
	FRotator PivotClosedRotation = FRotator::ZeroRotator;
	float OpenAlpha = 0.f;
	float SwingSign = 1.f;
	bool bLocked = false;
	bool bOpen = false;
};
