#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonActivatable.h"
#include "DungeonTrigger.generated.h"

class UBillboardComponent;

/** One thing a trigger drives. */
USTRUCT(BlueprintType)
struct FDungeonTargetLink
{
	GENERATED_BODY()

	/** The actor that reacts: a Dungeon Door, Dungeon Mover, another trigger, or any Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	TObjectPtr<AActor> Target = nullptr;

	/** Seconds to wait before this target reacts. Use it to stagger events (unlock, then a wall rises). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link", meta = (ClampMin = "0", Units = "s"))
	float Delay = 0.f;

	/** Send the opposite signal: when the trigger turns on, this target is deactivated (e.g. close a door). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
	bool bInvert = false;

	/**
	 * Optional. Name of a custom event (with no inputs) to call on the target instead of using the
	 * Dungeon Activatable interface. Lets you drive existing Blueprints like the Level Design Kit ones.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link", AdvancedDisplay)
	FName CustomEventOnActivate;

	/** Optional. Custom event called when the signal turns off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link", AdvancedDisplay)
	FName CustomEventOnDeactivate;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonTriggerStateSignature, bool, bIsOn, AActor*, Activator);

/**
 * Sends on/off signals to every linked target. Placed on its own it works as a relay: link a
 * trigger to it and it forwards the signal (with its own delays) to its targets. Valves and enemy
 * triggers are subclasses that decide when to turn on.
 *
 * Select one in the editor to see lines to its targets (green = normal, red = inverted).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Trigger (Relay)"))
class BETH_STYLE_DUNGEON_API ADungeonTrigger : public AActor, public IDungeonActivatable
{
	GENERATED_BODY()

public:
	ADungeonTrigger();

	/** Everything this trigger affects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Trigger", meta = (TitleProperty = "Target"))
	TArray<FDungeonTargetLink> Links;

	/** If true, it can be switched back off (sending Deactivate to targets). If false, it fires once and stays on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Trigger")
	bool bCanBeReversed = false;

	/** Fires whenever the trigger turns on or off. Bind in the Level Blueprint or a subclass for custom extras. */
	UPROPERTY(BlueprintAssignable, Category = "Dungeon Trigger")
	FDungeonTriggerStateSignature OnStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Dungeon Trigger")
	void SetOn(bool bNewOn, AActor* Activator);

	UFUNCTION(BlueprintCallable, Category = "Dungeon Trigger")
	void Toggle(AActor* Activator) { SetOn(!bIsOn, Activator); }

	UFUNCTION(BlueprintPure, Category = "Dungeon Trigger")
	bool IsOn() const { return bIsOn; }

	/** True if the state can still change in that direction (one-shot triggers lock after firing). */
	UFUNCTION(BlueprintPure, Category = "Dungeon Trigger")
	bool CanSwitchTo(bool bNewOn) const;

	// IDungeonActivatable: lets triggers be chained.
	virtual void OnPuzzleActivated_Implementation(AActor* Activator) override { SetOn(true, Activator); }
	virtual void OnPuzzleDeactivated_Implementation(AActor* Activator) override { SetOn(false, Activator); }

	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

protected:
	virtual void BeginPlay() override;

	/** Blueprint subclasses can react here (play an effect, change a material...). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dungeon Trigger", meta = (DisplayName = "On Trigger State Changed"))
	void ReceiveStateChanged(bool bNowOn, AActor* Activator);

	/** Extra editor-only lines drawn while selected. Subclasses add their own. */
	virtual void DrawEditorLinks() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dungeon Trigger")
	bool bIsOn = false;

	bool bHasFired = false;

private:
	void DispatchLink(const FDungeonTargetLink& Link, bool bOn, AActor* Activator);
	void SendSignal(const FDungeonTargetLink& Link, bool bOn, AActor* Activator);
};
