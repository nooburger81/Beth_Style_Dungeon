#include "DungeonMover.h"
#include "Components/AudioComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ADungeonMover::ADungeonMover()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	MoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoverMesh"));
	MoverMesh->SetupAttachment(SceneRoot);
	MoverMesh->SetMobility(EComponentMobility::Movable);

	LoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("LoopAudio"));
	LoopAudio->SetupAttachment(SceneRoot);
	LoopAudio->bAutoActivate = false;

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (EditorSprite)
	{
		EditorSprite->SetupAttachment(SceneRoot);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif
}

void ADungeonMover::BeginPlay()
{
	Super::BeginPlay();

	MyStartLocation = GetActorLocation();
	for (AActor* Actor : ActorsToMove)
	{
		if (!IsValid(Actor) || Actor == this)
		{
			continue;
		}
		if (USceneComponent* Root = Actor->GetRootComponent())
		{
			Root->SetMobility(EComponentMobility::Movable);
		}
		StartLocations.Emplace(Actor, Actor->GetActorLocation());
	}

	SetActorTickEnabled(false);
}

void ADungeonMover::SetActivated(bool bNewActivated)
{
	if (bActivated == bNewActivated)
	{
		return;
	}

	bActivated = bNewActivated;

	if (StartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StartSound, GetActorLocation());
	}
	if (LoopSound)
	{
		LoopAudio->SetSound(LoopSound);
		if (!LoopAudio->IsPlaying())
		{
			LoopAudio->FadeIn(0.5f);
		}
	}

	SetActorTickEnabled(true);
}

void ADungeonMover::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (World && !World->IsGameWorld())
	{
#if WITH_EDITOR
		if (IsSelected())
		{
			DrawEditorPreview();
		}
#endif
		return;
	}

	const float Target = bActivated ? 1.f : 0.f;
	MoveAlpha = FMath::FInterpConstantTo(MoveAlpha, Target, DeltaSeconds, 1.f / MoveDuration);
	ApplyAlpha(MoveAlpha);

	if (FMath::IsNearlyEqual(MoveAlpha, Target))
	{
		SetActorTickEnabled(false);
		LoopAudio->FadeOut(0.5f, 0.f);
		if (StopSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, StopSound, GetActorLocation());
		}
		OnMoveFinished.Broadcast(bActivated);
	}
}

void ADungeonMover::ApplyAlpha(float Alpha)
{
	const FVector Offset = ActivatedOffset * FMath::SmoothStep(0.f, 1.f, Alpha);

	SetActorLocation(MyStartLocation + Offset);
	for (const TPair<TWeakObjectPtr<AActor>, FVector>& Entry : StartLocations)
	{
		if (AActor* Actor = Entry.Key.Get())
		{
			Actor->SetActorLocation(Entry.Value + Offset);
		}
	}
}

void ADungeonMover::DrawEditorPreview() const
{
	const FVector Start = GetActorLocation();
	DrawDebugDirectionalArrow(GetWorld(), Start, Start + ActivatedOffset, 60.f, FColor::Cyan, false, -1.f, 0, 3.f);

	for (const AActor* Actor : ActorsToMove)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		// Show where each moved actor ends up.
		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		DrawDebugLine(GetWorld(), Start, Actor->GetActorLocation(), FColor::Cyan, false, -1.f, 0, 1.5f);
		DrawDebugBox(GetWorld(), Origin + ActivatedOffset, Extent, FColor::Cyan, false, -1.f, 0, 2.f);
	}
}
