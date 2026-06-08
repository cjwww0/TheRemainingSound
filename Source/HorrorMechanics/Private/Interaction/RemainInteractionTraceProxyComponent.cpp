#include "Interaction/RemainInteractionTraceProxyComponent.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"

URemainInteractionTraceProxyComponent::URemainInteractionTraceProxyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URemainInteractionTraceProxyComponent::OnRegister()
{
	Super::OnRegister();
	RebuildProxy();
}

void URemainInteractionTraceProxyComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildProxy();
}

void URemainInteractionTraceProxyComponent::RebuildProxy()
{
	if (!bEnableProxy)
	{
		if (ProxyBox)
		{
			ProxyBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ProxyBox->SetVisibility(false, true);
		}
		return;
	}

	EnsureProxyBox();
	ConfigureProxyBox();
}

void URemainInteractionTraceProxyComponent::EnsureProxyBox()
{
	if (ProxyBox || !GetOwner())
	{
		return;
	}

	ProxyBox = NewObject<UBoxComponent>(GetOwner(), TEXT("RemainInteractionTraceProxy"));
	if (!ProxyBox)
	{
		return;
	}

	if (USceneComponent* Root = GetOwner()->GetRootComponent())
	{
		ProxyBox->SetupAttachment(Root);
	}

	GetOwner()->AddInstanceComponent(ProxyBox);
	ProxyBox->RegisterComponent();
}

void URemainInteractionTraceProxyComponent::ConfigureProxyBox()
{
	if (!ProxyBox)
	{
		return;
	}

	ProxyBox->SetMobility(EComponentMobility::Movable);
	ProxyBox->SetRelativeLocation(RelativeLocation);
	ProxyBox->SetRelativeRotation(RelativeRotation);
	ProxyBox->SetBoxExtent(BoxExtent, true);
	ProxyBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProxyBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProxyBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ProxyBox->SetGenerateOverlapEvents(false);
	ProxyBox->SetHiddenInGame(bHiddenInGame);
	ProxyBox->SetVisibility(!bHiddenInGame, true);
	ProxyBox->SetCanEverAffectNavigation(false);
}
