#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "RemainFocusedInteractionHighlighterSubsystem.generated.h"

class UMaterialInterface;
class UPrimitiveComponent;
class ARemainInteractionHighlightConfigActor;
class APostProcessVolume;
class UCameraComponent;
class APawn;
class UClass;

USTRUCT()
struct FRemainHighlightedPrimitiveState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Component = nullptr;

	UPROPERTY(Transient)
	bool bRenderCustomDepth = false;

	UPROPERTY(Transient)
	int32 CustomDepthStencilValue = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OverlayMaterial = nullptr;
};

UCLASS()
class HORRORMECHANICS_API URemainFocusedInteractionHighlighterSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bEnableFocusedHighlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight", meta=(ClampMin="0"))
	int32 HighlightStencilValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TObjectPtr<UMaterialInterface> HighlightOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TObjectPtr<UMaterialInterface> PostProcessMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bAutoApplyPostProcessMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TArray<FName> IgnoredActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TArray<FName> IgnoredComponentTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bClearHighlightWhenHUDHasActiveScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight|Debug")
	bool bDebugFocusedHighlight = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> HighlightedActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ARemainInteractionHighlightConfigActor> CachedConfigActor = nullptr;

	UPROPERTY(Transient)
	TArray<FRemainHighlightedPrimitiveState> HighlightedPrimitiveStates;

	TWeakObjectPtr<UCameraComponent> CachedPostProcessCamera;
	TWeakObjectPtr<UMaterialInterface> CachedAppliedPostProcessMaterial;
	TWeakObjectPtr<APostProcessVolume> RuntimePostProcessVolume;
	bool bLoggedPostProcessStatus = false;

	void UpdateFocusedActor();
	AActor* ResolveFocusedActor() const;
	AActor* ResolveTemplateFocusedActor() const;
	bool TraceFocusedInteractiveActor(AActor*& OutActor, UPrimitiveComponent*& OutHitComponent) const;
	float ResolveInteractTraceDistance(const APawn* Pawn) const;
	bool IsInteractionCandidate(const AActor* Actor) const;
	bool IsInteractionDisabledForActor(AActor* Actor, UPrimitiveComponent* HitComponent) const;
	int32 SetActorHighlighted(AActor* Actor, bool bHighlighted);
	void ClearHighlight();
	void ApplyConfigIfAvailable();
	void ApplyPostProcessMaterialIfNeeded();
	bool EnsureRuntimePostProcessVolume(UMaterialInterface* Material);
	UMaterialInterface* ResolvePostProcessMaterial();
	UMaterialInterface* ResolveOverlayMaterial();
	void RemoveKnownPostProcessBlendables() const;
	bool ShouldClearForUIState() const;
	bool ShouldSkipActor(const AActor* Actor) const;
	bool ShouldHighlightComponent(const UPrimitiveComponent* PrimitiveComponent) const;
	void DebugFocusedHighlight(const AActor* Actor, int32 HighlightedComponentCount) const;
	void DebugPostProcessStatus(const FString& Message, FColor Color = FColor::Green);
};
