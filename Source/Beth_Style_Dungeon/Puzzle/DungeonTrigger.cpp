#include "DungeonTrigger.h"
#include "../Beth_Style_Dungeon.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

ADungeonTrigger::ADungeonTrigger()
{
	// Ticks in the editor only, to draw link lines; gameplay subclasses re-enable it while animating.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (EditorSprite)
	{
		EditorSprite->SetupAttachment(SceneRoot);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif
}

void ADungeonTrigger::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
}

bool ADungeonTrigger::CanSwitchTo(bool bNewOn) const
{
	if (bIsOn == bNewOn)
	{
		return false;
	}
	return bCanBeReversed || !bHasFired;
}

void ADungeonTrigger::SetOn(bool bNewOn, AActor* Activator)
{
	if (!CanSwitchTo(bNewOn))
	{
		return;
	}

	bIsOn = bNewOn;
	bHasFired |= bNewOn;

	for (const FDungeonTargetLink& Link : Links)
	{
		DispatchLink(Link, bNewOn, Activator);
	}

	OnStateChanged.Broadcast(bIsOn, Activator);
	ReceiveStateChanged(bIsOn, Activator);
}

void ADungeonTrigger::DispatchLink(const FDungeonTargetLink& Link, bool bOn, AActor* Activator)
{
	if (Link.Delay <= 0.f)
	{
		SendSignal(Link, bOn, Activator);
		return;
	}

	FTimerHandle Handle;
	TWeakObjectPtr<AActor> WeakActivator = Activator;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Link, bOn, WeakActivator]()
	{
		SendSignal(Link, bOn, WeakActivator.Get());
	}), Link.Delay, false);
}

void ADungeonTrigger::SendSignal(const FDungeonTargetLink& Link, bool bOn, AActor* Activator)
{
	AActor* Target = Link.Target;
	if (!IsValid(Target))
	{
		return;
	}

	const bool bSignal = Link.bInvert ? !bOn : bOn;

	const FName EventName = bSignal ? Link.CustomEventOnActivate : Link.CustomEventOnDeactivate;
	if (!EventName.IsNone())
	{
		UFunction* Function = Target->FindFunction(EventName);
		if (Function && Function->ParmsSize == 0)
		{
			Target->ProcessEvent(Function, nullptr);
		}
		else
		{
			UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: '%s' has no input-less event named '%s'."), *GetName(), *Target->GetName(), *EventName.ToString());
		}
		return;
	}

	if (!Target->Implements<UDungeonActivatable>())
	{
		// No interface and no custom event for this direction: nothing to do. Only warn if neither event was set.
		if (Link.CustomEventOnActivate.IsNone() && Link.CustomEventOnDeactivate.IsNone())
		{
			UE_LOG(LogDungeonPuzzle, Warning, TEXT("%s: '%s' doesn't implement Dungeon Activatable. Set a custom event name on the link."), *GetName(), *Target->GetName());
		}
		return;
	}

	if (bSignal)
	{
		IDungeonActivatable::Execute_OnPuzzleActivated(Target, Activator);
	}
	else
	{
		IDungeonActivatable::Execute_OnPuzzleDeactivated(Target, Activator);
	}
}

void ADungeonTrigger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (World && !World->IsGameWorld() && IsSelected())
	{
		DrawEditorLinks();
	}
#endif
}

void ADungeonTrigger::DrawEditorLinks() const
{
	for (const FDungeonTargetLink& Link : Links)
	{
		if (IsValid(Link.Target))
		{
			const FColor Color = Link.bInvert ? FColor::Red : FColor::Green;
			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), Link.Target->GetActorLocation(), 60.f, Color, false, -1.f, 0, 3.f);
		}
	}
}
