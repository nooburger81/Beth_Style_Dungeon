#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DungeonLightSubsystem.generated.h"

class USceneComponent;

/**
 * Owns the on/off state of lights and effects driven by Dungeon Rooms. Remembers each light's original
 * intensity the first time a room touches it, and when several rooms share a source it uses the
 * brightest room, so walking from one room into the next never switches off a light both rooms use.
 */
UCLASS()
class BETH_STYLE_DUNGEON_API UDungeonLightSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Sets how lit Room wants these sources (0 = off, 1 = original brightness). Lights and FX components only. */
	void SetRoomAlpha(const UObject* Room, TConstArrayView<TWeakObjectPtr<USceneComponent>> Sources, float Alpha);

private:
	struct FSourceRecord
	{
		float FullIntensity = 0.f;
		/** False if the source started hidden or inactive; rooms then leave it off. */
		bool bStartedEnabled = true;
		float AppliedAlpha = -1.f;
		TMap<TWeakObjectPtr<const UObject>, float> RoomAlphas;
	};

	FSourceRecord& FindOrAddRecord(USceneComponent* Source);
	static void Apply(USceneComponent* Source, FSourceRecord& Record);

	TMap<TWeakObjectPtr<USceneComponent>, FSourceRecord> Records;
};
