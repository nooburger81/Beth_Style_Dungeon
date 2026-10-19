#include "DungeonEnemyTrigger.h"
#include "../Beth_Style_Dungeon.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr float EnemyCheckInterval = 0.25f;
}

ADungeonEnemyTrigger::ADungeonEnemyTrigger()
{
	bCanBeReversed = false;
}

void ADungeonEnemyTrigger::BeginPlay()
{
	Super::BeginPlay();

	for (AActor* Enemy : Enemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		TrackedEnemies.Add(Enemy);
		if (!HealthPropertyName.IsNone() && !Enemy->GetClass()->FindPropertyByName(HealthPropertyName))
		{
			UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: enemy '%s' has no variable '%s'; it will only count as defeated when destroyed or reported."),
				*GetName(), *Enemy->GetName(), *HealthPropertyName.ToString());
		}
	}

	if (TrackedEnemies.IsEmpty())
	{
		UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: no enemies assigned, so it will never fire."), *GetName());
		return;
	}

	GetWorldTimerManager().SetTimer(CheckTimer, this, &ADungeonEnemyTrigger::CheckEnemies, EnemyCheckInterval, true);
}

void ADungeonEnemyTrigger::ReportEnemyDefeated(AActor* Enemy)
{
	ReportedDefeats.Add(Enemy);
	CheckEnemies();
}

bool ADungeonEnemyTrigger::IsEnemyDefeated(const TWeakObjectPtr<AActor>& Enemy) const
{
	if (!Enemy.IsValid() || ReportedDefeats.Contains(Enemy))
	{
		return true;
	}

	const FProperty* Property = Enemy->GetClass()->FindPropertyByName(HealthPropertyName);
	if (!Property)
	{
		return false;
	}

	const void* Value = Property->ContainerPtrToValuePtr<void>(Enemy.Get());
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		return BoolProperty->GetPropertyValue(Value);
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		const double Health = NumericProperty->IsFloatingPoint()
			? NumericProperty->GetFloatingPointPropertyValue(Value)
			: static_cast<double>(NumericProperty->GetSignedIntPropertyValue(Value));
		return Health <= 0.0;
	}
	return false;
}

int32 ADungeonEnemyTrigger::GetDefeatedCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Enemy : TrackedEnemies)
	{
		Count += IsEnemyDefeated(Enemy) ? 1 : 0;
	}
	return Count;
}

int32 ADungeonEnemyTrigger::GetRequiredCount() const
{
	return RequiredDefeats > 0 ? FMath::Min(RequiredDefeats, TrackedEnemies.Num()) : TrackedEnemies.Num();
}

void ADungeonEnemyTrigger::CheckEnemies()
{
	if (bIsOn || TrackedEnemies.IsEmpty() || GetDefeatedCount() < GetRequiredCount())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(CheckTimer);

	if (CompletedSound)
	{
		UGameplayStatics::PlaySound2D(this, CompletedSound);
	}
	SetOn(true, this);
}

void ADungeonEnemyTrigger::DrawEditorLinks() const
{
	Super::DrawEditorLinks();

	for (const AActor* Enemy : Enemies)
	{
		if (IsValid(Enemy))
		{
			DrawDebugLine(GetWorld(), GetActorLocation(), Enemy->GetActorLocation(), FColor::Orange, false, -1.f, 0, 2.f);
		}
	}
}
