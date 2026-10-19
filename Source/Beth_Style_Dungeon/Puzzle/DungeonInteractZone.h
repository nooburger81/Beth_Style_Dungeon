#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InputCoreTypes.h"
#include "DungeonInteractZone.generated.h"

class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonInteractSignature, APawn*, Interactor);

/**
 * Box the player stands in to use something. While inside, pressing InteractKey fires OnInteract
 * and an optional floating prompt is shown, turned to face the camera.
 */
UCLASS(ClassGroup = (DungeonPuzzle), meta = (BlueprintSpawnableComponent))
class BETH_STYLE_DUNGEON_API UDungeonInteractZone : public UBoxComponent
{
	GENERATED_BODY()

public:
	UDungeonInteractZone();

	/** Key the player presses while inside the zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FKey InteractKey = EKeys::E;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FDungeonInteractSignature OnInteract;

	/** Hooks up the floating text this zone shows/hides. */
	void SetPrompt(UTextRenderComponent* InPrompt);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptText(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionEnabled() const { return bInteractionEnabled; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	APawn* GetPawnInside() const { return PawnInside.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void HandleKeyPressed();
	void RefreshPrompt();

	TWeakObjectPtr<APawn> PawnInside;
	TWeakObjectPtr<UTextRenderComponent> Prompt;
	FText PromptText;
	bool bInteractionEnabled = true;
	bool bKeyBound = false;
};
