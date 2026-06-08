#include "Interaction/RemainInteractionSetupLibrary.h"

#include "GameFramework/Actor.h"
#include "Interaction/RemainInteractionTraceProxyComponent.h"

URemainInteractionTraceProxyComponent* URemainInteractionSetupLibrary::EnsureInteractionTraceProxy(
	AActor* Actor,
	FVector RelativeLocation,
	FVector BoxExtent,
	FRotator RelativeRotation)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	Actor->Modify();

	URemainInteractionTraceProxyComponent* ProxyComponent = Actor->FindComponentByClass<URemainInteractionTraceProxyComponent>();
	if (!ProxyComponent)
	{
		ProxyComponent = NewObject<URemainInteractionTraceProxyComponent>(Actor, TEXT("RemainInteractionTraceProxyComponent"), RF_Transactional);
		if (!ProxyComponent)
		{
			return nullptr;
		}

		Actor->AddInstanceComponent(ProxyComponent);
		ProxyComponent->OnComponentCreated();
		ProxyComponent->RegisterComponent();
	}

	ProxyComponent->Modify();
	ProxyComponent->bEnableProxy = true;
	ProxyComponent->RelativeLocation = RelativeLocation;
	ProxyComponent->BoxExtent = BoxExtent;
	ProxyComponent->RelativeRotation = RelativeRotation;
	ProxyComponent->RebuildProxy();

	return ProxyComponent;
}
