#include "DungeonRoom.h"
#include "DungeonLightSubsystem.h"
#include "../Beth_Style_Dungeon.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

namespace
{
	/** The player's own torch or lantern must never be switched off by a room. */
	bool IsCarriedByPawn(const AActor* Actor)
	{
		for (const AActor* It = Actor; It; It = It->GetAttachParentActor())
		{
			if (It->IsA<APawn>() || (It->GetOwner() && It->GetOwner()->IsA<APawn>()))
			{
				return true;
			}
		}
		return false;
	}
}

ADungeonRoom::ADungeonRoom()
{
	bCanBeReversed = true;

	RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds"));
	RoomBounds->SetupAttachment(SceneRoot);
	RoomBounds->InitBoxExtent(FVector(500.f, 500.f, 250.f));
	RoomBounds->SetCollisionProfileName(TEXT("Trigger"));
	RoomBounds->SetGenerateOverlapEvents(true);
	RoomBounds->ShapeColor = FColor(255, 200, 60);
}

void ADungeonRoom::BeginPlay()
{
	Super::BeginPlay();

	RoomBounds->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::HandleBeginOverlap);
	RoomBounds->OnComponentEndOverlap.AddDynamic(this, &ADungeonRoom::HandleEndOverlap);

	SetLightsOn(false, true);

	// Overlaps for a player who spawns inside the room aren't ready yet during BeginPlay.
	GetWorldTimerManager().SetTimerForNextTick(this, &ADungeonRoom::CheckInitialOverlaps);
}

void ADungeonRoom::CheckInitialOverlaps()
{
	TArray<AActor*> Overlapping;
	RoomBounds->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		PlayerEntered(Cast<APawn>(Actor));
	}
}

void ADungeonRoom::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	PlayerEntered(Cast<APawn>(OtherActor));
}

void ADungeonRoom::PlayerEntered(APawn* Pawn)
{
	if (!Pawn || !Pawn->IsPlayerControlled() || PlayersInside.Contains(Pawn))
	{
		return;
	}

	PlayersInside.Add(Pawn);
	if (PlayersInside.Num() == 1)
	{
		SetLightsOn(true);
		SetOn(true, Pawn);
	}
}

void ADungeonRoom::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	// A pawn has several components; only leave once none of them overlap.
	if (!Pawn || !PlayersInside.Contains(Pawn) || RoomBounds->IsOverlappingActor(Pawn))
	{
		return;
	}

	PlayersInside.Remove(Pawn);
	if (PlayersInside.IsEmpty())
	{
		SetLightsOn(false);
		SetOn(false, Pawn);
	}
}

void ADungeonRoom::SetLightsOn(bool bOn, bool bInstant)
{
	GatherSources(Sources);
	TargetAlpha = bOn ? 1.f : 0.f;

	if (bInstant || FadeTime <= 0.f)
	{
		LightAlpha = TargetAlpha;
		PushAlpha();
		SetActorTickEnabled(false);
	}
	else
	{
		SetActorTickEnabled(true);
	}
}

void ADungeonRoom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	LightAlpha = FadeTime > 0.f ? FMath::FInterpConstantTo(LightAlpha, TargetAlpha, DeltaSeconds, 1.f / FadeTime) : TargetAlpha;
	PushAlpha();
	if (LightAlpha == TargetAlpha)
	{
		SetActorTickEnabled(false);
	}
}

void ADungeonRoom::PushAlpha()
{
	if (UDungeonLightSubsystem* Lights = GetWorld()->GetSubsystem<UDungeonLightSubsystem>())
	{
		Lights->SetRoomAlpha(this, Sources, LightAlpha);
	}
}

bool ADungeonRoom::IsInsideBounds(const FVector& WorldLocation) const
{
	const FVector Local = RoomBounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = RoomBounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

void ADungeonRoom::GatherSources(TArray<TWeakObjectPtr<USceneComponent>>& OutSources) const
{
	OutSources.Reset();

	for (AActor* Actor : LightSources)
	{
		if (IsValid(Actor))
		{
			AddSourcesFromActor(Actor, false, OutSources);
		}
	}

	if (bAutoFindLights)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor != this && !LightSources.Contains(Actor) && !IgnoredSources.Contains(Actor) && !IsCarriedByPawn(Actor))
			{
				AddSourcesFromActor(Actor, true, OutSources);
			}
		}
	}
}

void ADungeonRoom::AddSourcesFromActor(AActor* Actor, bool bOnlyInsideBounds, TArray<TWeakObjectPtr<USceneComponent>>& OutSources) const
{
	TArray<ULightComponent*> Lights;
	Actor->GetComponents(Lights);

	bool bAnyLight = false;
	for (ULightComponent* Light : Lights)
	{
		if (bOnlyInsideBounds && !IsInsideBounds(Light->GetComponentLocation()))
		{
			continue;
		}
		if (Light->Mobility == EComponentMobility::Static)
		{
			// Only in game: the editor preview runs this every frame.
			UE_CLOG(GetWorld()->IsGameWorld(),LogDungeonPuzzle, Warning, TEXT("%s: light '%s' on '%s' is Static and can't be switched at runtime. Set it to Movable or Stationary."),
				*GetName(), *Light->GetName(), *Actor->GetName());
			continue;
		}
		OutSources.Add(Light);
		bAnyLight = true;
	}

	// Effects follow their actor's light, so a torch's flame goes out with its glow.
	if (bToggleEffects && (bAnyLight || !bOnlyInsideBounds))
	{
		TArray<UFXSystemComponent*> Effects;
		Actor->GetComponents(Effects);
		OutSources.Append(Effects);
	}
}

void ADungeonRoom::DrawEditorLinks() const
{
	Super::DrawEditorLinks();

	TArray<TWeakObjectPtr<USceneComponent>> Preview;
	GatherSources(Preview);
	for (const TWeakObjectPtr<USceneComponent>& Source : Preview)
	{
		if (Source.IsValid() && Source->IsA<ULightComponent>())
		{
			DrawDebugLine(GetWorld(), GetActorLocation(), Source->GetComponentLocation(), FColor::Yellow, false, -1.f, 0, 2.f);
		}
	}
}
