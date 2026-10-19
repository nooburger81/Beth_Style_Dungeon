#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonActivatable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonActivatable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that can be driven by a Dungeon Trigger (valve, enemy trigger, relay...).
 * Doors and movers implement this in C++. Any Blueprint can also add it under
 * Class Settings > Implemented Interfaces and handle the two events itself.
 */
class BETH_STYLE_DUNGEON_API IDungeonActivatable
{
	GENERATED_BODY()

public:
	/** A linked trigger turned on. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dungeon Puzzle")
	void OnPuzzleActivated(AActor* Activator);

	/** A linked trigger turned back off. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dungeon Puzzle")
	void OnPuzzleDeactivated(AActor* Activator);
};
