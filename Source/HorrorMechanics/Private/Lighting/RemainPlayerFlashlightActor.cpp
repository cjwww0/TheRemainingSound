#include "Lighting/RemainPlayerFlashlightActor.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARemainPlayerFlashlightActor::ARemainPlayerFlashlightActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PlayerFlashlight"));
	Flashlight->SetupAttachment(Root);
	Flashlight->SetMobility(EComponentMobility::Movable);
	Flashlight->SetVisibility(false);
	Flashlight->SetCastShadows(true);
}

void ARemainPlayerFlashlightActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureFlashlight();
	TryAttachToPlayerCamera();
	BindToggleInput();
	SetFlashlightEnabled(bStartEnabled);
}

void ARemainPlayerFlashlightActor::SetFlashlightEnabled(bool bEnabled)
{
	if (Flashlight)
	{
		Flashlight->SetVisibility(bEnabled, true);
	}
}

void ARemainPlayerFlashlightActor::ToggleFlashlight()
{
	if (Flashlight)
	{
		SetFlashlightEnabled(!Flashlight->IsVisible());
	}
}

void ARemainPlayerFlashlightActor::ConfigureFlashlight()
{
	if (!Flashlight)
	{
		return;
	}

	Flashlight->SetIntensity(Intensity);
	Flashlight->SetAttenuationRadius(AttenuationRadius);
	Flashlight->SetInnerConeAngle(InnerConeAngle);
	Flashlight->SetOuterConeAngle(OuterConeAngle);
	Flashlight->SetLightColor(LightColor);
	Flashlight->SetCastShadows(bCastFlashlightShadows);
	Flashlight->SetRelativeLocation(CameraRelativeLocation);
	Flashlight->SetRelativeRotation(CameraRelativeRotation);
}

void ARemainPlayerFlashlightActor::TryAttachToPlayerCamera()
{
	if (bAttachedToCamera || !Flashlight)
	{
		return;
	}

	if (UCameraComponent* CameraComponent = ResolvePlayerCamera())
	{
		Flashlight->AttachToComponent(CameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Flashlight->SetRelativeLocation(CameraRelativeLocation);
		Flashlight->SetRelativeRotation(CameraRelativeRotation);
		bAttachedToCamera = true;
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AttachRetryCount < MaxAttachRetries)
		{
			++AttachRetryCount;
			World->GetTimerManager().SetTimer(AttachRetryTimerHandle, this, &ARemainPlayerFlashlightActor::TryAttachToPlayerCamera, AttachRetryDelay, false);
		}
	}
}

void ARemainPlayerFlashlightActor::BindToggleInput()
{
	if (!bBindToggleInput)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	EnableInput(PlayerController);
	if (InputComponent)
	{
		InputComponent->BindAction(ToggleActionName, IE_Pressed, this, &ARemainPlayerFlashlightActor::ToggleFlashlight);
	}
}

UCameraComponent* ARemainPlayerFlashlightActor::ResolvePlayerCamera() const
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const APawn* Pawn = nullptr;
	if (PlayerController)
	{
		Pawn = PlayerController->GetPawn();
	}
	if (!Pawn)
	{
		Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	if (!Pawn)
	{
		return nullptr;
	}

	TInlineComponentArray<UCameraComponent*> CameraComponents;
	Pawn->GetComponents(CameraComponents);

	for (UCameraComponent* CameraComponent : CameraComponents)
	{
		if (CameraComponent && CameraComponent->GetName().Contains(TEXT("FirstPersonCamera")))
		{
			return CameraComponent;
		}
	}

	return CameraComponents.Num() > 0 ? CameraComponents[0] : nullptr;
}
