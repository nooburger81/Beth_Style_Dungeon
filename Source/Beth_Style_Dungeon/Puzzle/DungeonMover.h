#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonActivatable.h"
#include "DungeonMover.generated.h"

class UAudioComponent;
class UBillboardComponent;
class UStaticMeshComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonMoverSignature, bool, bNowActivated);

/**
 * Smoothly moves itself and a list of other actors by an offset when activated, and back when
 * deactivated. Built for raising/lowering water: put the water surface mesh, the water physics
 * volume and any underwater post-process volume in Actors To Move.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Mover"))
class BETH_STYLE_DUNGEON_API ADungeonMover : public AActor, public IDungeonActivatable
{
	GENERATED_BODY()

public:
	ADungeonMover();

	/**
	 * Level actors that move together with this one. Don't also attach them to this actor, or they'll move twice.
	 * They are switched to Movable automatically when play starts.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Mover")
	TArray<TObjectPtr<AActor>> ActorsToMove;

	/** How far everything moves when activated. Negative Z lowers the water. Select the mover to preview it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	FVector ActivatedOffset = FVector(0.f, 0.f, -300.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover", meta = (ClampMin = "0.05", Units = "s"))
	float MoveDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sound")
	TObjectPtr<USoundBase> StartSound;

	/** Plays while moving (use a looping sound, e.g. rushing water). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sound")
	TObjectPtr<USoundBase> LoopSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover|Sound")
	TObjectPtr<USoundBase> StopSound;

	/** Fires when a move finishes. */
	UPROPERTY(BlueprintAssignable, Category = "Mover")
	FDungeonMoverSignature OnMoveFinished;

	UFUNCTION(BlueprintCallable, Category = "Mover")
	void SetActivated(bool bNewActivated);

	UFUNCTION(BlueprintPure, Category = "Mover")
	bool IsActivated() const { return bActivated; }

	/** 0 = start position, 1 = fully offset. */
	UFUNCTION(BlueprintPure, Category = "Mover")
	float GetMoveAlpha() const { return MoveAlpha; }

	virtual void OnPuzzleActivated_Implementation(AActor* Activator) override { SetActivated(true); }
	virtual void OnPuzzleDeactivated_Implementation(AActor* Activator) override { SetActivated(false); }

	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Optional mesh carried by the mover itself (e.g. a water plane). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MoverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> LoopAudio;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif

private:
	void ApplyAlpha(float Alpha);
	void DrawEditorPreview() const;

	TArray<TPair<TWeakObjectPtr<AActor>, FVector>> StartLocations;
	FVector MyStartLocation = FVector::ZeroVector;
	float MoveAlpha = 0.f;
	bool bActivated = false;
};
