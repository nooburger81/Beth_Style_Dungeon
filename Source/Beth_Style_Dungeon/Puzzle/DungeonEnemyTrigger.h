#pragma once

#include "CoreMinimal.h"
#include "DungeonTrigger.h"
#include "DungeonEnemyTrigger.generated.h"

class USoundBase;

/**
 * Turns on once the listed enemies are defeated. An enemy counts as defeated when it is destroyed,
 * when its health variable (Health Property Name) drops to 0 or below, when a bool variable with that
 * name becomes true, or when something calls Report Enemy Defeated on this trigger.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Enemy Trigger"))
class BETH_STYLE_DUNGEON_API ADungeonEnemyTrigger : public ADungeonTrigger
{
	GENERATED_BODY()

public:
	ADungeonEnemyTrigger();

	/** Enemies placed in the level that must be defeated. Use the eyedropper to pick them. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemies")
	TArray<TObjectPtr<AActor>> Enemies;

	/**
	 * Variable read on each enemy to decide if it's dead. Number: dead at 0 or below. Bool: dead when true.
	 * The Level Design Kit enemies use "Life".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies")
	FName HealthPropertyName = TEXT("Life");

	/** How many enemies must be defeated. 0 means all of them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies", meta = (ClampMin = "0"))
	int32 RequiredDefeats = 0;

	/** Played once when the requirement is met (e.g. a lock clunk or a sting). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies")
	TObjectPtr<USoundBase> CompletedSound;

	/** Call from your own enemy Blueprint's death logic if it doesn't use a health variable. */
	UFUNCTION(BlueprintCallable, Category = "Enemies")
	void ReportEnemyDefeated(AActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "Enemies")
	int32 GetDefeatedCount() const;

	UFUNCTION(BlueprintPure, Category = "Enemies")
	int32 GetRequiredCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void DrawEditorLinks() const override;

private:
	void CheckEnemies();
	bool IsEnemyDefeated(const TWeakObjectPtr<AActor>& Enemy) const;

	TArray<TWeakObjectPtr<AActor>> TrackedEnemies;
	TSet<TWeakObjectPtr<AActor>> ReportedDefeats;
	FTimerHandle CheckTimer;
};
