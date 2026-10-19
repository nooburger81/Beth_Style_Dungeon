#pragma once

#include "CoreMinimal.h"
#include "DungeonTrigger.h"
#include "DungeonValve.generated.h"

class UDungeonInteractZone;
class UStaticMeshComponent;
class UTextRenderComponent;
class USoundBase;

/**
 * A wheel the player turns with E. Turning it switches the trigger on; if Can Be Reversed is
 * ticked, turning it again spins it back and switches it off (good for raising/lowering water).
 */
UCLASS(Blueprintable, meta = (DisplayName = "Dungeon Valve"))
class BETH_STYLE_DUNGEON_API ADungeonValve : public ADungeonTrigger
{
	GENERATED_BODY()

public:
	ADungeonValve();

	/** How far the wheel spins (degrees) when turned on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float TurnAngle = 720.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve", meta = (ClampMin = "0.05", Units = "s"))
	float TurnDuration = 1.5f;

	/** If true, targets react once the wheel stops. If false, they react the moment the player starts turning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	bool bSignalWhenTurnFinishes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	TObjectPtr<USoundBase> TurnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	FText PromptTurnOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	FText PromptTurnOff;

	/** Turn the valve as if the player used it. */
	UFUNCTION(BlueprintCallable, Category = "Valve")
	void Use(AActor* User);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleInteract(APawn* Interactor);

	void UpdatePrompt();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StemMesh;

	/** Rotates around its X axis (the red arrow). Swap the wheel mesh on WheelMesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WheelPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WheelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDungeonInteractZone> InteractZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> PromptText;

private:
	TWeakObjectPtr<AActor> PendingUser;
	bool bTurning = false;
	bool bTurningOn = false;
	float TurnElapsed = 0.f;
	float StartAngle = 0.f;
	float CurrentAngle = 0.f;
};
