#include "Interaction/RemainFocusedInteractionHighlighterSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ChildActorComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Documents/RemainReadableDocumentActor.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Events/RemainRadioActor.h"
#include "Interaction/RemainInteractionHighlightConfigActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Interface.h"
#include "UObject/UnrealType.h"

bool URemainFocusedInteractionHighlighterSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URemainFocusedInteractionHighlighterSubsystem::Tick(float DeltaTime)
{
	ApplyConfigIfAvailable();

	if (!bEnableFocusedHighlight)
	{
		ClearHighlight();
		return;
	}

	RemoveKnownPostProcessBlendables();
	ApplyPostProcessMaterialIfNeeded();

	if (ShouldClearForUIState())
	{
		ClearHighlight();
		return;
	}

	UpdateFocusedActor();
}

TStatId URemainFocusedInteractionHighlighterSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URemainFocusedInteractionHighlighterSubsystem, STATGROUP_Tickables);
}

bool URemainFocusedInteractionHighlighterSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

bool URemainFocusedInteractionHighlighterSubsystem::IsTickableWhenPaused() const
{
	return false;
}

void URemainFocusedInteractionHighlighterSubsystem::Deinitialize()
{
	ClearHighlight();
	if (APostProcessVolume* PostProcessVolume = RuntimePostProcessVolume.Get())
	{
		PostProcessVolume->Destroy();
	}
	RuntimePostProcessVolume = nullptr;
	Super::Deinitialize();
}

void URemainFocusedInteractionHighlighterSubsystem::UpdateFocusedActor()
{
	AActor* FocusedActor = ResolveFocusedActor();
	if (!IsValid(FocusedActor) || ShouldSkipActor(FocusedActor))
	{
		FocusedActor = nullptr;
	}

	if (FocusedActor == HighlightedActor)
	{
		return;
	}

	ClearHighlight();

	if (IsValid(FocusedActor))
	{
		HighlightedActor = FocusedActor;
		const int32 HighlightedComponentCount = SetActorHighlighted(FocusedActor, true);
		DebugFocusedHighlight(FocusedActor, HighlightedComponentCount);
	}
}

AActor* URemainFocusedInteractionHighlighterSubsystem::ResolveFocusedActor() const
{
	if (AActor* TemplateFocusedActor = ResolveTemplateFocusedActor())
	{
		return TemplateFocusedActor;
	}

	AActor* FocusedActor = nullptr;
	UPrimitiveComponent* HitComponent = nullptr;
	return TraceFocusedInteractiveActor(FocusedActor, HitComponent) ? FocusedActor : nullptr;
}

AActor* URemainFocusedInteractionHighlighterSubsystem::ResolveTemplateFocusedActor() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return nullptr;
	}

	const FObjectPropertyBase* FocusedActorProperty = FindFProperty<FObjectPropertyBase>(Pawn->GetClass(), TEXT("PlayerFocusedActor"));
	if (!FocusedActorProperty)
	{
		return nullptr;
	}

	UObject* FocusedObject = FocusedActorProperty->GetObjectPropertyValue_InContainer(Pawn);
	AActor* FocusedActor = Cast<AActor>(FocusedObject);
	if (!IsValid(FocusedActor) || !IsInteractionCandidate(FocusedActor))
	{
		return nullptr;
	}

	return IsInteractionDisabledForActor(FocusedActor, nullptr) ? nullptr : FocusedActor;
}

bool URemainFocusedInteractionHighlighterSubsystem::TraceFocusedInteractiveActor(AActor*& OutActor, UPrimitiveComponent*& OutHitComponent) const
{
	OutActor = nullptr;
	OutHitComponent = nullptr;

	const UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
	if (!IsValid(World) || !IsValid(PlayerController) || !IsValid(CameraManager))
	{
		return false;
	}

	const FVector TraceStart = CameraManager->GetCameraLocation();
	const FVector TraceEnd = TraceStart + CameraManager->GetCameraRotation().Vector() * ResolveInteractTraceDistance(Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RemainFocusedInteractionTrace), false);
	if (IsValid(Pawn))
	{
		QueryParams.AddIgnoredActor(Pawn);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	AActor* HitActor = Hit.GetActor();
	UPrimitiveComponent* HitPrimitive = Hit.GetComponent();
	if (!IsValid(HitActor) && IsValid(HitPrimitive))
	{
		HitActor = HitPrimitive->GetOwner();
	}

	if (!IsValid(HitActor) || !IsInteractionCandidate(HitActor))
	{
		return false;
	}

	if (IsInteractionDisabledForActor(HitActor, HitPrimitive))
	{
		return false;
	}

	OutActor = HitActor;
	OutHitComponent = HitPrimitive;
	return true;
}

float URemainFocusedInteractionHighlighterSubsystem::ResolveInteractTraceDistance(const APawn* Pawn) const
{
	constexpr float DefaultInteractDistance = 450.0f;
	if (!IsValid(Pawn))
	{
		return DefaultInteractDistance;
	}

	const FProperty* DistanceProperty = FindFProperty<FProperty>(Pawn->GetClass(), TEXT("MaxInteractDistance"));
	const FNumericProperty* NumericProperty = CastField<FNumericProperty>(DistanceProperty);
	if (!NumericProperty)
	{
		return DefaultInteractDistance;
	}

	const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Pawn);
	const double Distance = NumericProperty->IsFloatingPoint()
		? NumericProperty->GetFloatingPointPropertyValue(ValuePtr)
		: static_cast<double>(NumericProperty->GetSignedIntPropertyValue(ValuePtr));

	return Distance > 0.0 ? static_cast<float>(Distance) : DefaultInteractDistance;
}

bool URemainFocusedInteractionHighlighterSubsystem::IsInteractionCandidate(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor->IsA<ARemainRadioActor>())
	{
		return true;
	}

	if (Actor->IsA<ARemainReadableDocumentActor>())
	{
		return true;
	}

	static TWeakObjectPtr<UClass> CachedInteractiveInterfaceClass;
	UClass* InterfaceClass = CachedInteractiveInterfaceClass.Get();
	if (!InterfaceClass)
	{
		InterfaceClass = StaticLoadClass(
			UInterface::StaticClass(),
			nullptr,
			TEXT("/Game/HorrorMechanics/Blueprint/Interfaces/BP_Interactive.BP_Interactive_C"));
		CachedInteractiveInterfaceClass = InterfaceClass;
	}

	if (InterfaceClass && Actor->GetClass()->ImplementsInterface(InterfaceClass))
	{
		return true;
	}

	// Some Blueprint Interface assets can be hard to load from native code in editor utility contexts.
	// Matching the event/function names keeps the highlighter aligned with the template interaction graph.
	return Actor->GetClass()->FindFunctionByName(TEXT("Interaction")) != nullptr
		|| Actor->GetClass()->FindFunctionByName(TEXT("IsInteractionDisabled")) != nullptr;
}

bool URemainFocusedInteractionHighlighterSubsystem::IsInteractionDisabledForActor(AActor* Actor, UPrimitiveComponent* HitComponent) const
{
	if (!IsValid(Actor))
	{
		return true;
	}

	if (const ARemainRadioActor* RadioActor = Cast<ARemainRadioActor>(Actor))
	{
		return !RadioActor->IsRadioInteractionAvailable(HitComponent);
	}

	UFunction* IsDisabledFunction = Actor->FindFunction(TEXT("IsInteractionDisabled"));
	if (!IsDisabledFunction)
	{
		return false;
	}

	struct FRemainIsInteractionDisabledParams
	{
		UPrimitiveComponent* Component = nullptr;
		bool Yes = false;
	};

	FRemainIsInteractionDisabledParams Params;
	Params.Component = HitComponent;
	Actor->ProcessEvent(IsDisabledFunction, &Params);
	return Params.Yes;
}

int32 URemainFocusedInteractionHighlighterSubsystem::SetActorHighlighted(AActor* Actor, bool bHighlighted)
{
	if (!IsValid(Actor))
	{
		return 0;
	}

	int32 HighlightedComponentCount = 0;
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	TSet<AActor*> VisitedActors;
	TSet<UPrimitiveComponent*> VisitedComponents;
	CollectHighlightableComponents(Actor, VisitedActors, VisitedComponents, PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (bHighlighted)
		{
			FRemainHighlightedPrimitiveState State;
			State.Component = PrimitiveComponent;
			State.bRenderCustomDepth = PrimitiveComponent->bRenderCustomDepth;
			State.CustomDepthStencilValue = PrimitiveComponent->CustomDepthStencilValue;

			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(PrimitiveComponent))
			{
				State.OverlayMaterial = MeshComponent->GetOverlayMaterial();
				if (UMaterialInterface* OverlayMaterial = ResolveOverlayMaterial())
				{
					MeshComponent->SetOverlayMaterial(OverlayMaterial);
				}
			}

			HighlightedPrimitiveStates.Add(State);

			PrimitiveComponent->SetRenderCustomDepth(true);
			PrimitiveComponent->SetCustomDepthStencilValue(HighlightStencilValue);
			PrimitiveComponent->MarkRenderStateDirty();
			++HighlightedComponentCount;
		}
	}

	return HighlightedComponentCount;
}

void URemainFocusedInteractionHighlighterSubsystem::CollectHighlightableComponents(
	AActor* Actor,
	TSet<AActor*>& VisitedActors,
	TSet<UPrimitiveComponent*>& VisitedComponents,
	TArray<UPrimitiveComponent*>& OutComponents) const
{
	if (!IsValid(Actor) || VisitedActors.Contains(Actor) || ShouldSkipActor(Actor))
	{
		return;
	}

	VisitedActors.Add(Actor);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!ShouldHighlightComponent(PrimitiveComponent) || VisitedComponents.Contains(PrimitiveComponent))
		{
			continue;
		}

		VisitedComponents.Add(PrimitiveComponent);
		OutComponents.Add(PrimitiveComponent);
	}

	TInlineComponentArray<UChildActorComponent*> ChildActorComponents;
	Actor->GetComponents(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		if (!IsValid(ChildActorComponent))
		{
			continue;
		}

		CollectHighlightableComponents(
			ChildActorComponent->GetChildActor(),
			VisitedActors,
			VisitedComponents,
			OutComponents);
	}

	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		CollectHighlightableComponents(
			AttachedActor,
			VisitedActors,
			VisitedComponents,
			OutComponents);
	}
}

void URemainFocusedInteractionHighlighterSubsystem::ClearHighlight()
{
	for (const FRemainHighlightedPrimitiveState& State : HighlightedPrimitiveStates)
	{
		UPrimitiveComponent* PrimitiveComponent = State.Component.Get();
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(PrimitiveComponent))
		{
			MeshComponent->SetOverlayMaterial(State.OverlayMaterial);
		}

		PrimitiveComponent->SetRenderCustomDepth(State.bRenderCustomDepth);
		PrimitiveComponent->SetCustomDepthStencilValue(State.CustomDepthStencilValue);
		PrimitiveComponent->MarkRenderStateDirty();
	}

	HighlightedPrimitiveStates.Reset();
	HighlightedActor = nullptr;
}

void URemainFocusedInteractionHighlighterSubsystem::ApplyConfigIfAvailable()
{
	if (IsValid(CachedConfigActor.Get()))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ARemainInteractionHighlightConfigActor::StaticClass(), FoundActors);
	if (FoundActors.Num() <= 0)
	{
		return;
	}

	CachedConfigActor = Cast<ARemainInteractionHighlightConfigActor>(FoundActors[0]);
	if (!IsValid(CachedConfigActor.Get()))
	{
		return;
	}

	bEnableFocusedHighlight = CachedConfigActor->bEnableFocusedHighlight;
	const int32 ConfigStencilValue = CachedConfigActor->HighlightStencilValue;
	HighlightStencilValue = ConfigStencilValue >= 1 && ConfigStencilValue <= 6 ? ConfigStencilValue : 1;
	HighlightOverlayMaterial = CachedConfigActor->HighlightOverlayMaterial;
	PostProcessMaterial = CachedConfigActor->PostProcessMaterial;
	if (IsValid(PostProcessMaterial) && !GetPathNameSafe(PostProcessMaterial).Contains(TEXT("SoftOutline")))
	{
		PostProcessMaterial = nullptr;
	}
	bAutoApplyPostProcessMaterial = true;
	IgnoredActorTags = CachedConfigActor->IgnoredActorTags;
	IgnoredComponentTags = CachedConfigActor->IgnoredComponentTags;
	bClearHighlightWhenHUDHasActiveScreen = CachedConfigActor->bClearHighlightWhenHUDHasActiveScreen;
	bDebugFocusedHighlight = CachedConfigActor->bDebugFocusedHighlight;
	bLoggedPostProcessStatus = false;
}

void URemainFocusedInteractionHighlighterSubsystem::ApplyPostProcessMaterialIfNeeded()
{
	if (!bAutoApplyPostProcessMaterial)
	{
		return;
	}

	UMaterialInterface* Material = ResolvePostProcessMaterial();
	if (!IsValid(Material))
	{
		DebugPostProcessStatus(TEXT("InteractionHighlight SoftOutline material failed to load"), FColor::Red);
		return;
	}

	if (!EnsureRuntimePostProcessVolume(Material))
	{
		DebugPostProcessStatus(FString::Printf(
			TEXT("InteractionHighlight failed to create runtime PPV | Material=%s"),
			*GetPathNameSafe(Material)), FColor::Red);
		return;
	}

	CachedAppliedPostProcessMaterial = Material;
	DebugPostProcessStatus(FString::Printf(
		TEXT("InteractionHighlight runtime PPV active | Material=%s | Stencil=%d"),
		*GetPathNameSafe(Material),
		HighlightStencilValue));
}

bool URemainFocusedInteractionHighlighterSubsystem::EnsureRuntimePostProcessVolume(UMaterialInterface* Material)
{
	if (!IsValid(Material))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	APostProcessVolume* PostProcessVolume = RuntimePostProcessVolume.Get();
	if (!IsValid(PostProcessVolume))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = MakeUniqueObjectName(World, APostProcessVolume::StaticClass(), TEXT("RemainRuntimeSoftOutlinePPV"));
		SpawnParams.ObjectFlags |= RF_Transient;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PostProcessVolume = World->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FTransform::Identity,
			SpawnParams);

		if (!IsValid(PostProcessVolume))
		{
			RuntimePostProcessVolume = nullptr;
			return false;
		}

		PostProcessVolume->SetActorHiddenInGame(true);
		PostProcessVolume->bEnabled = true;
		PostProcessVolume->bUnbound = true;
		PostProcessVolume->Priority = 10000.0f;
		PostProcessVolume->BlendRadius = 0.0f;
		PostProcessVolume->BlendWeight = 1.0f;
		RuntimePostProcessVolume = PostProcessVolume;
	}

	PostProcessVolume->Settings.WeightedBlendables.Array.RemoveAll(
		[](const FWeightedBlendable& Blendable)
		{
			const UObject* BlendableObject = Blendable.Object.Get();
			const FString PathName = GetPathNameSafe(BlendableObject);
			return PathName.Contains(TEXT("M_PP_SubtleInteractOutline"))
				|| PathName.Contains(TEXT("M_PP_InteractOutline_StencilOnly"));
		});

	const bool bAlreadyApplied = PostProcessVolume->Settings.WeightedBlendables.Array.ContainsByPredicate(
		[Material](const FWeightedBlendable& Blendable)
		{
			return Blendable.Object.Get() == Material;
		});

	if (!bAlreadyApplied)
	{
		PostProcessVolume->Settings.AddBlendable(Material, 1.0f);
	}

	PostProcessVolume->bEnabled = true;
	PostProcessVolume->bUnbound = true;
	PostProcessVolume->BlendWeight = 1.0f;
	return true;
}

UMaterialInterface* URemainFocusedInteractionHighlighterSubsystem::ResolvePostProcessMaterial()
{
	if (IsValid(PostProcessMaterial))
	{
		return PostProcessMaterial;
	}

	PostProcessMaterial = Cast<UMaterialInterface>(StaticLoadObject(
		UMaterialInterface::StaticClass(),
		nullptr,
		TEXT("/Game/SoftOutline/Styles/MI_TRS_InteractOutline_WarmElectric.MI_TRS_InteractOutline_WarmElectric")));

	if (!IsValid(PostProcessMaterial))
	{
		PostProcessMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/SoftOutline/Styles/SoftOutline_Electric.SoftOutline_Electric")));
	}

	if (!IsValid(PostProcessMaterial))
	{
		PostProcessMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/SoftOutline/Styles/MI_TRS_InteractOutline_Warm.MI_TRS_InteractOutline_Warm")));
	}

	if (!IsValid(PostProcessMaterial))
	{
		PostProcessMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/SoftOutline/Styles/SoftOutline_Simple.SoftOutline_Simple")));
	}

	if (!IsValid(PostProcessMaterial))
	{
		PostProcessMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/SoftOutline/SoftOutline.SoftOutline")));
	}

	return PostProcessMaterial;
}

UMaterialInterface* URemainFocusedInteractionHighlighterSubsystem::ResolveOverlayMaterial()
{
	if (IsValid(HighlightOverlayMaterial))
	{
		return HighlightOverlayMaterial;
	}

	HighlightOverlayMaterial = Cast<UMaterialInterface>(StaticLoadObject(
		UMaterialInterface::StaticClass(),
		nullptr,
		TEXT("/Game/HorrorMechanics/Materials/M_InteractWarmOverlay.M_InteractWarmOverlay")));

	return HighlightOverlayMaterial;
}

void URemainFocusedInteractionHighlighterSubsystem::RemoveKnownPostProcessBlendables() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return;
	}

	TInlineComponentArray<UCameraComponent*> CameraComponents;
	Pawn->GetComponents(CameraComponents);
	for (UCameraComponent* CameraComponent : CameraComponents)
	{
		if (!IsValid(CameraComponent))
		{
			continue;
		}

		CameraComponent->PostProcessSettings.WeightedBlendables.Array.RemoveAll(
			[](const FWeightedBlendable& Blendable)
			{
				const UObject* BlendableObject = Blendable.Object.Get();
				const FString PathName = GetPathNameSafe(BlendableObject);
				return PathName.Contains(TEXT("M_PP_SubtleInteractOutline"))
					|| PathName.Contains(TEXT("M_PP_InteractOutline_StencilOnly"));
			});
	}

	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		APostProcessVolume* PostProcessVolume = *It;
		if (!IsValid(PostProcessVolume))
		{
			continue;
		}

		PostProcessVolume->Settings.WeightedBlendables.Array.RemoveAll(
			[](const FWeightedBlendable& Blendable)
			{
				const UObject* BlendableObject = Blendable.Object.Get();
				const FString PathName = GetPathNameSafe(BlendableObject);
				return PathName.Contains(TEXT("M_PP_SubtleInteractOutline"))
					|| PathName.Contains(TEXT("M_PP_InteractOutline_StencilOnly"));
			});
	}
}

bool URemainFocusedInteractionHighlighterSubsystem::ShouldClearForUIState() const
{
	if (!bClearHighlightWhenHUDHasActiveScreen)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const AHUD* HUD = PlayerController ? PlayerController->GetHUD() : nullptr;
	if (!IsValid(HUD))
	{
		return false;
	}

	const FProperty* ActiveScreenProperty = FindFProperty<FProperty>(HUD->GetClass(), TEXT("CurrentActiveScreen"));
	if (!ActiveScreenProperty)
	{
		return false;
	}

	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(ActiveScreenProperty))
	{
		const int64 RawValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(HUD));
		return RawValue != 0;
	}

	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(ActiveScreenProperty))
	{
		return ByteProperty->GetPropertyValue_InContainer(HUD) != 0;
	}

	if (const FIntProperty* IntProperty = CastField<FIntProperty>(ActiveScreenProperty))
	{
		return IntProperty->GetPropertyValue_InContainer(HUD) != 0;
	}

	return false;
}

bool URemainFocusedInteractionHighlighterSubsystem::ShouldSkipActor(const AActor* Actor) const
{
	if (!IsValid(Actor) || Actor->IsHidden())
	{
		return true;
	}

	for (const FName& IgnoredTag : IgnoredActorTags)
	{
		if (!IgnoredTag.IsNone() && Actor->ActorHasTag(IgnoredTag))
		{
			return true;
		}
	}

	return false;
}

bool URemainFocusedInteractionHighlighterSubsystem::ShouldHighlightComponent(const UPrimitiveComponent* PrimitiveComponent) const
{
	if (!IsValid(PrimitiveComponent) || !PrimitiveComponent->IsVisible())
	{
		return false;
	}

	if (!PrimitiveComponent->GetOwner() || PrimitiveComponent->GetOwner()->IsHidden())
	{
		return false;
	}

	if (PrimitiveComponent->IsA<UShapeComponent>())
	{
		return false;
	}

	if (!PrimitiveComponent->IsA<UMeshComponent>())
	{
		return false;
	}

	for (const FName& IgnoredTag : IgnoredComponentTags)
	{
		if (!IgnoredTag.IsNone() && PrimitiveComponent->ComponentHasTag(IgnoredTag))
		{
			return false;
		}
	}

	return true;
}

void URemainFocusedInteractionHighlighterSubsystem::DebugFocusedHighlight(const AActor* Actor, int32 HighlightedComponentCount) const
{
	if (!bDebugFocusedHighlight)
	{
		return;
	}

	const FString Message = FString::Printf(
		TEXT("InteractionHighlight Focus=%s Components=%d Stencil=%d"),
		*GetNameSafe(Actor),
		HighlightedComponentCount,
		HighlightStencilValue);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			reinterpret_cast<uint64>(this),
			1.5f,
			HighlightedComponentCount > 0 ? FColor::Cyan : FColor::Orange,
			Message);
	}
}

void URemainFocusedInteractionHighlighterSubsystem::DebugPostProcessStatus(const FString& Message, FColor Color)
{
	if (!bDebugFocusedHighlight || bLoggedPostProcessStatus)
	{
		return;
	}

	bLoggedPostProcessStatus = true;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			reinterpret_cast<uint64>(this) + 1,
			5.0f,
			Color,
			Message);
	}
}
