#pragma once

#include "CoreMinimal.h"
#include "../Puzzle/DungeonTrigger.h"
#include "DungeonRoom.generated.h"

class UBoxComponent;

/**
 * Scale Room Bounds to cover a room. Lights inside it (point/spot lights, torch and fire Blueprints)
 * fade on when the player walks in and off when they leave. It's also a trigger: Links are activated
 * on enter and deactivated on leave, so a room can close a door behind you or wake something up.
 *
 * Lights must be Movable or Stationary to switch at runtime. Select the room in the editor to see
 * yellow lines to every light it controls.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Room (Lights)"))
class BETH_STYLE_DUNGEON_API ADungeonRoom : public ADungeonTrigger
{
	GENERATED_BODY()

public:
	ADungeonRoom();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> RoomBounds;

	/** Control every light placed inside Room Bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Lights")
	bool bAutoFindLights = true;

	/** Extra actors whose lights belong to this room even though they sit outside the bounds (e.g. in a doorway). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Room Lights")
	TArray<TObjectPtr<AActor>> LightSources;

	/** Lights inside the bounds to leave alone, such as one that should always stay lit. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Room Lights")
	TArray<TObjectPtr<AActor>> IgnoredSources;

	/** Also stop fire, smoke and other particle effects on the same actors while the room is dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Lights")
	bool bToggleEffects = false;

	/** Seconds to fade lights in or out. 0 switches instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Lights", meta = (ClampMin = "0", Units = "s"))
	float FadeTime = 0.5f;

	UFUNCTION(BlueprintPure, Category = "Room Lights")
	bool IsPlayerInside() const { return !PlayersInside.IsEmpty(); }

	/** Turn the room's lights on or off by hand (e.g. from a lever). The next enter/leave takes over again. */
	UFUNCTION(BlueprintCallable, Category = "Room Lights")
	void SetLightsOn(bool bOn, bool bInstant = false);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void DrawEditorLinks() const override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void CheckInitialOverlaps();
	void PlayerEntered(APawn* Pawn);
	void PushAlpha();

	/** Light (and optionally FX) components this room drives, found fresh each time so streamed-in lights are picked up. */
	void GatherSources(TArray<TWeakObjectPtr<USceneComponent>>& OutSources) const;
	void AddSourcesFromActor(AActor* Actor, bool bOnlyInsideBounds, TArray<TWeakObjectPtr<USceneComponent>>& OutSources) const;
	bool IsInsideBounds(const FVector& WorldLocation) const;

	TSet<TWeakObjectPtr<APawn>> PlayersInside;
	TArray<TWeakObjectPtr<USceneComponent>> Sources;
	float LightAlpha = 1.f;
	float TargetAlpha = 0.f;
};
