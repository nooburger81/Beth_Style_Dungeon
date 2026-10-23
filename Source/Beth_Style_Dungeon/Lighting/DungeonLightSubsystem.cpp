#include "DungeonLightSubsystem.h"
#include "Components/LightComponent.h"
#include "Particles/ParticleSystemComponent.h"

void UDungeonLightSubsystem::SetRoomAlpha(const UObject* Room, TConstArrayView<TWeakObjectPtr<USceneComponent>> Sources, float Alpha)
{
	for (const TWeakObjectPtr<USceneComponent>& WeakSource : Sources)
	{
		if (USceneComponent* Source = WeakSource.Get())
		{
			FSourceRecord& Record = FindOrAddRecord(Source);
			Record.RoomAlphas.Add(Room, Alpha);
			Apply(Source, Record);
		}
	}
}

UDungeonLightSubsystem::FSourceRecord& UDungeonLightSubsystem::FindOrAddRecord(USceneComponent* Source)
{
	if (FSourceRecord* Existing = Records.Find(Source))
	{
		return *Existing;
	}

	FSourceRecord& Record = Records.Add(Source);
	if (const ULightComponent* Light = Cast<ULightComponent>(Source))
	{
		Record.FullIntensity = Light->Intensity;
		Record.bStartedEnabled = Light->IsVisible();
	}
	else if (const UFXSystemComponent* Effect = Cast<UFXSystemComponent>(Source))
	{
		Record.bStartedEnabled = Effect->IsActive() || Effect->bAutoActivate;
	}
	return Record;
}

void UDungeonLightSubsystem::Apply(USceneComponent* Source, FSourceRecord& Record)
{
	float Alpha = 0.f;
	for (auto It = Record.RoomAlphas.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
		Alpha = FMath::Max(Alpha, It->Value);
	}

	if (Alpha == Record.AppliedAlpha)
	{
		return;
	}
	Record.AppliedAlpha = Alpha;

	const bool bEnabled = Record.bStartedEnabled && Alpha > 0.f;
	if (ULightComponent* Light = Cast<ULightComponent>(Source))
	{
		Light->SetIntensity(Record.FullIntensity * Alpha);
		Light->SetVisibility(bEnabled);
	}
	else if (UFXSystemComponent* Effect = Cast<UFXSystemComponent>(Source))
	{
		if (bEnabled && !Effect->IsActive())
		{
			Effect->Activate(true);
		}
		else if (!bEnabled && Effect->IsActive())
		{
			// Lets existing flames and smoke die out naturally instead of popping.
			Effect->Deactivate();
		}
	}
}
