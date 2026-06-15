#include "Documents/RemainReadableDocumentActor.h"

#include "Components/BoxComponent.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Widget.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* DocumentRefStructPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/Data/DocumentDT_Ref_Struct.DocumentDT_Ref_Struct");
	const TCHAR* DocumentTypeEnumPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/Data/DocumentType_Enum.DocumentType_Enum");
	const TCHAR* ActiveScreenEnumPath = TEXT("/Game/HorrorMechanics/Blueprint/UI/Screens/ActiveScreen_Enum.ActiveScreen_Enum");
	const TCHAR* BookPageWidgetClassPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/UI/UI_BookPage.UI_BookPage_C");
	const TCHAR* GenericDocumentPageWidgetClassPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/UI/UI_GenericDocumentPage.UI_GenericDocumentPage_C");
	const TCHAR* BookDocumentClassPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/Data/BP_BookData.BP_BookData_C");
	const TCHAR* GenericDocumentClassPath = TEXT("/Game/HorrorMechanics/Blueprint/Documents/Data/BP_GenericDocumentData.BP_GenericDocumentData_C");

	bool NameMatches(const FString& Candidate, const FName DesiredName)
	{
		const FString Desired = DesiredName.ToString();
		return Candidate.Equals(Desired, ESearchCase::IgnoreCase) || Candidate.Contains(Desired, ESearchCase::IgnoreCase);
	}

	int64 ResolveEnumValue(const UEnum* Enum, const FName DesiredName)
	{
		if (!Enum)
		{
			return 0;
		}

		const int32 EnumCount = FMath::Max(0, Enum->NumEnums() - 1);
		for (int32 Index = 0; Index < EnumCount; ++Index)
		{
			if (NameMatches(Enum->GetNameStringByIndex(Index), DesiredName) ||
				NameMatches(Enum->GetDisplayNameTextByIndex(Index).ToString(), DesiredName))
			{
				return Enum->GetValueByIndex(Index);
			}
		}

		// The template enum is Note, GenericDocument, Book. Use Book as a safe fallback for diary entries.
		if (DesiredName.ToString().Equals(TEXT("Book"), ESearchCase::IgnoreCase) && EnumCount > 2)
		{
			return Enum->GetValueByIndex(2);
		}

		return EnumCount > 0 ? Enum->GetValueByIndex(0) : 0;
	}

	template<typename IntPropertyType>
	bool SetIntegerParamValue(FProperty* Property, void* Params, const int64 Value)
	{
		if (IntPropertyType* IntProperty = CastField<IntPropertyType>(Property))
		{
			IntProperty->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params), Value);
			return true;
		}
		return false;
	}
}

ARemainReadableDocumentActor::ARemainReadableDocumentActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	InputPriority = 1000;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Mesh);
	InteractionBox->SetBoxExtent(FVector(70.0f, 45.0f, 30.0f));
	InteractionBox->SetRelativeLocation(FVector::ZeroVector);
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionBox->SetGenerateOverlapEvents(false);
	InteractionBox->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UDataTable> BooksTableFinder(TEXT("/Game/HorrorMechanics/Database/Documents/Books_DataTable.Books_DataTable"));
	if (BooksTableFinder.Succeeded())
	{
		DocumentDataTable = BooksTableFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> OpenBookMeshFinder(TEXT("/Game/HorrorMechanics/ExampleAssets/Documents/Book/Meshes/SM_OpenBook.SM_OpenBook"));
	if (OpenBookMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(OpenBookMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<AActor> TemplateDocumentFinder(TEXT("/Game/HorrorMechanics/Blueprint/BP_Document"));
	if (TemplateDocumentFinder.Succeeded())
	{
		TemplateDocumentClass = TemplateDocumentFinder.Class;
	}
}

void ARemainReadableDocumentActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionBox)
	{
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	if (bSpawnTemplateDocumentProxy && !IsConfiguredAsBookDocument())
	{
		SpawnTemplateDocumentProxy();
	}

	SetActorTickEnabled(bEnableDirectInteractInputFallback);
}

void ARemainReadableDocumentActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDirectInteractInputFallback();
}

void ARemainReadableDocumentActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisableDirectInteractInput();

	if (IsValid(SpawnedTemplateDocument))
	{
		SpawnedTemplateDocument->Destroy();
		SpawnedTemplateDocument = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ARemainReadableDocumentActor::OpenReadableDocument(APlayerController* PlayerController)
{
	if (!OpenDocument(PlayerController))
	{
		DebugMessage(TEXT("Failed to open readable document"));
	}
}

bool ARemainReadableDocumentActor::OpenDocument(APlayerController* PlayerController)
{
	if (bDocumentOpened && bDestroyAfterOpen)
	{
		return true;
	}

	if (!IsValid(PlayerController))
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (!IsValid(GetEffectiveDocumentDataTable()) || DocumentRowName.IsNone())
	{
		DebugMessage(TEXT("DocumentDataTable or DocumentRowName is not set"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Opening configured document | Actor=%s | Table=%s | Row=%s | Type=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetEffectiveDocumentDataTable()),
		*DocumentRowName.ToString(),
		*DocumentTypeName.ToString());

	UObject* Inventory = ResolveInventory();
	if (!IsValid(Inventory))
	{
		DebugMessage(TEXT("Could not resolve BP_HorrorGameState.Inventory"));
		return false;
	}

	int32 DocumentIndex = CachedDocumentIndex;
	UObject* AddedDocument = GetInventoryDocumentObject(Inventory, DocumentIndex);
	if (!IsValid(AddedDocument))
	{
		DocumentIndex = INDEX_NONE;
		if (!AddDocumentToInventory(Inventory, DocumentIndex) || DocumentIndex < 0)
		{
			DebugMessage(TEXT("Inventory.AddDocumentAsDTRef failed"));
			return false;
		}

		CachedDocumentIndex = DocumentIndex;
		AddedDocument = GetInventoryDocumentObject(Inventory, DocumentIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Added document to inventory | Index=%d | Class=%s"),
		DocumentIndex,
		*GetInventoryDocumentClassName(Inventory, DocumentIndex));

	if (IsConfiguredAsBookDocument() && OpenBookDocumentDirect(PlayerController, AddedDocument))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Opened book document through direct UI path | Document=%s"),
			*GetNameSafe(AddedDocument));
	}
	else if (IsConfiguredAsGenericDocument() && OpenGenericDocumentDirect(PlayerController, AddedDocument))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Opened generic document through direct UI path | Document=%s"),
			*GetNameSafe(AddedDocument));
	}
	else
	{
		if (!ShowDocument(PlayerController, DocumentIndex))
		{
			DebugMessage(TEXT("HUD.ShowDocument failed"));
			return false;
		}

		if (IsConfiguredAsBookDocument())
		{
			if (!TryPatchBookDocumentScreen(PlayerController, AddedDocument))
			{
				UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Book UI patch failed; falling back to existing ShowDocument result | Document=%s"),
					*GetNameSafe(AddedDocument));
			}
		}
		else if (IsConfiguredAsGenericDocument())
		{
			if (!TryPatchGenericDocumentScreen(PlayerController, AddedDocument))
			{
				UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Generic document UI patch failed; falling back to existing ShowDocument result | Document=%s"),
					*GetNameSafe(AddedDocument));
			}
		}
	}

	if (bPlayCollectSound && CollectSound)
	{
		UGameplayStatics::PlaySound2D(this, CollectSound);
	}

	const bool bShouldDestroyAfterOpen = bDestroyAfterOpen && !IsConfiguredAsBookDocument() && !IsConfiguredAsGenericDocument();
	bDocumentOpened = bShouldDestroyAfterOpen;
	DisableDirectInteractInput();

	if (IsValid(SpawnedTemplateDocument) && bShouldDestroyAfterOpen)
	{
		SpawnedTemplateDocument->Destroy();
		SpawnedTemplateDocument = nullptr;
	}

	if (bShouldDestroyAfterOpen)
	{
		Destroy();
	}

	return true;
}

bool ARemainReadableDocumentActor::IsReadableDocumentInteractionDisabled(UPrimitiveComponent* Component) const
{
	return false;
}

void ARemainReadableDocumentActor::SpawnTemplateDocumentProxy()
{
	if (!GetWorld() || !TemplateDocumentClass || IsValid(SpawnedTemplateDocument))
	{
		return;
	}

	AActor* ProxyActor = GetWorld()->SpawnActorDeferred<AActor>(
		TemplateDocumentClass,
		GetActorTransform(),
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(ProxyActor))
	{
		DebugMessage(TEXT("Failed to spawn template BP_Document proxy"));
		return;
	}

	ConfigureTemplateDocumentProxy(ProxyActor);
	UGameplayStatics::FinishSpawningActor(ProxyActor, GetActorTransform());
	ConfigureTemplateDocumentProxy(ProxyActor);
	HideTemplateDocumentProxyVisuals(ProxyActor);

	SpawnedTemplateDocument = ProxyActor;
	DisableNativeInteractionCollision();

	if (bDebugDocumentOpen)
	{
		DebugMessage(FString::Printf(TEXT("Spawned template BP_Document proxy: %s"), *GetNameSafe(ProxyActor)));
	}
}

bool ARemainReadableDocumentActor::ConfigureTemplateDocumentProxy(AActor* ProxyActor) const
{
	UDataTable* EffectiveDataTable = GetEffectiveDocumentDataTable();
	if (!IsValid(ProxyActor) || !IsValid(EffectiveDataTable) || DocumentRowName.IsNone())
	{
		return false;
	}

	bool bConfigured = false;

	auto SetRowHandleProperty = [this, EffectiveDataTable, ProxyActor, &bConfigured](FStructProperty* StructProperty)
	{
		if (!StructProperty || StructProperty->Struct != TBaseStructure<FDataTableRowHandle>::Get())
		{
			return;
		}

		FDataTableRowHandle* Handle = StructProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(ProxyActor);
		Handle->DataTable = EffectiveDataTable;
		Handle->RowName = DocumentRowName;
		bConfigured = true;

		if (bDebugDocumentOpen)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Proxy handle set | Proxy=%s | Property=%s | Table=%s | Row=%s"),
				*GetNameSafe(ProxyActor),
				*StructProperty->GetName(),
				*GetNameSafe(EffectiveDataTable),
				*DocumentRowName.ToString());
		}
	};

	if (FStructProperty* SelectDocumentProperty = FindFProperty<FStructProperty>(ProxyActor->GetClass(), TEXT("SelectDocument")))
	{
		SetRowHandleProperty(SelectDocumentProperty);
	}

	// BP-generated names can change. On this proxy actor it is safer to force every row handle to the diary entry.
	for (TFieldIterator<FStructProperty> It(ProxyActor->GetClass()); It; ++It)
	{
		SetRowHandleProperty(*It);
	}

	const FName EffectiveDocumentTypeName = IsConfiguredAsBookDocument() ? FName(TEXT("Book")) : DocumentTypeName;
	UEnum* DocumentTypeEnum = LoadObject<UEnum>(nullptr, DocumentTypeEnumPath);
	const int64 DocumentTypeValue = ResolveEnumValue(DocumentTypeEnum, EffectiveDocumentTypeName);

	if (FEnumProperty* DocumentTypeProperty = FindFProperty<FEnumProperty>(ProxyActor->GetClass(), TEXT("DocumentType")))
	{
		DocumentTypeProperty->GetUnderlyingProperty()->SetIntPropertyValue(
			DocumentTypeProperty->ContainerPtrToValuePtr<void>(ProxyActor),
			DocumentTypeValue);
		bConfigured = true;
	}

	if (FByteProperty* DocumentTypeByteProperty = FindFProperty<FByteProperty>(ProxyActor->GetClass(), TEXT("DocumentType")))
	{
		DocumentTypeByteProperty->SetIntPropertyValue(
			DocumentTypeByteProperty->ContainerPtrToValuePtr<void>(ProxyActor),
			DocumentTypeByteProperty->Enum ? ResolveEnumValue(DocumentTypeByteProperty->Enum, EffectiveDocumentTypeName) : DocumentTypeValue);
		bConfigured = true;
	}

	for (TFieldIterator<FProperty> It(ProxyActor->GetClass()); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property || !Property->GetName().Contains(TEXT("DocumentType")))
		{
			continue;
		}

		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(
				EnumProperty->ContainerPtrToValuePtr<void>(ProxyActor),
				ResolveEnumValue(EnumProperty->GetEnum(), EffectiveDocumentTypeName));
			bConfigured = true;
			continue;
		}

		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			ByteProperty->SetIntPropertyValue(
				ByteProperty->ContainerPtrToValuePtr<void>(ProxyActor),
				ByteProperty->Enum ? ResolveEnumValue(ByteProperty->Enum, EffectiveDocumentTypeName) : DocumentTypeValue);
			bConfigured = true;
		}
	}

	if (bDebugDocumentOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Proxy document type forced | Proxy=%s | Type=%s | Value=%lld"),
			*GetNameSafe(ProxyActor),
			*EffectiveDocumentTypeName.ToString(),
			DocumentTypeValue);
	}

	if (FBoolProperty* PlaySoundProperty = FindFProperty<FBoolProperty>(ProxyActor->GetClass(), TEXT("PlaySound")))
	{
		PlaySoundProperty->SetPropertyValue_InContainer(ProxyActor, bPlayCollectSound);
	}

	if (FObjectPropertyBase* CollectSoundProperty = FindFProperty<FObjectPropertyBase>(ProxyActor->GetClass(), TEXT("CollectSound")))
	{
		CollectSoundProperty->SetObjectPropertyValue_InContainer(ProxyActor, CollectSound);
	}

	return bConfigured;
}

void ARemainReadableDocumentActor::HideTemplateDocumentProxyVisuals(AActor* ProxyActor) const
{
	if (!IsValid(ProxyActor))
	{
		return;
	}

	TArray<UActorComponent*> Components;
	ProxyActor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
		if (!Primitive)
		{
			continue;
		}

		// Keep the template document's interaction box alive so the original HUD trace still finds BP_Document.
		if (Cast<UBoxComponent>(Primitive) || Primitive->GetName().Contains(TEXT("Box")))
		{
			Primitive->SetHiddenInGame(true);
			Primitive->SetVisibility(false, true);
			Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Primitive->SetCollisionResponseToAllChannels(ECR_Ignore);
			Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			continue;
		}

		Primitive->SetHiddenInGame(true);
		Primitive->SetVisibility(false, true);
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ARemainReadableDocumentActor::DisableNativeInteractionCollision()
{
	if (InteractionBox)
	{
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true, true);
	}
}

void ARemainReadableDocumentActor::UpdateDirectInteractInputFallback()
{
	if (!bEnableDirectInteractInputFallback || bDocumentOpened || !GetWorld())
	{
		DisableDirectInteractInput();
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PlayerController))
	{
		DisableDirectInteractInput();
		return;
	}

	if (IsPlayerLookingAtReadableDocument(PlayerController))
	{
		EnableDirectInteractInput(PlayerController);
	}
	else
	{
		DisableDirectInteractInput();
	}
}

bool ARemainReadableDocumentActor::IsPlayerLookingAtReadableDocument(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerCameraManager) || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector TraceEnd = TraceStart + PlayerController->PlayerCameraManager->GetCameraRotation().Vector() * DirectInteractTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RemainReadableDocumentDirectInteractTrace), false);
	if (APawn* Pawn = PlayerController->GetPawn())
	{
		QueryParams.AddIgnoredActor(Pawn);
	}

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, DirectInteractTraceChannel, QueryParams);
	return IsTraceHitReadableDocument(Hit);
}

bool ARemainReadableDocumentActor::IsTraceHitReadableDocument(const FHitResult& Hit) const
{
	AActor* HitActor = Hit.GetActor();
	if (HitActor == this || HitActor == SpawnedTemplateDocument)
	{
		return true;
	}

	UActorComponent* HitComponent = Hit.GetComponent();
	if (!HitComponent)
	{
		return false;
	}

	AActor* ComponentOwner = HitComponent->GetOwner();
	return ComponentOwner == this || ComponentOwner == SpawnedTemplateDocument;
}

void ARemainReadableDocumentActor::EnableDirectInteractInput(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || DirectInteractActionName.IsNone())
	{
		return;
	}

	if (!bDirectInputEnabled || DirectInputPlayerController != PlayerController)
	{
		DirectInputPlayerController = PlayerController;
		EnableInput(PlayerController);
		bDirectInputEnabled = true;
	}

	if (!InputComponent || bDirectInputBound)
	{
		return;
	}

	InputComponent->Priority = InputPriority;
	FInputActionBinding& Binding = InputComponent->BindAction(DirectInteractActionName, IE_Pressed, this, &ARemainReadableDocumentActor::HandleDirectInteractInput);
	Binding.bConsumeInput = true;
	bDirectInputBound = true;
}

void ARemainReadableDocumentActor::DisableDirectInteractInput()
{
	if (!bDirectInputEnabled)
	{
		return;
	}

	if (IsValid(DirectInputPlayerController))
	{
		DisableInput(DirectInputPlayerController);
	}

	DirectInputPlayerController = nullptr;
	bDirectInputEnabled = false;
}

void ARemainReadableDocumentActor::HandleDirectInteractInput()
{
	APlayerController* PlayerController = DirectInputPlayerController;
	if (!IsValid(PlayerController))
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Direct interact input handled | Actor=%s | PlayerController=%s"),
		*GetNameSafe(this),
		*GetNameSafe(PlayerController));

	OpenReadableDocument(PlayerController);
}

UDataTable* ARemainReadableDocumentActor::GetEffectiveDocumentDataTable() const
{
	if (IsValid(DocumentDataTable))
	{
		return DocumentDataTable;
	}

	return LoadObject<UDataTable>(nullptr, TEXT("/Game/HorrorMechanics/Database/Documents/Books_DataTable.Books_DataTable"));
}

UObject* ARemainReadableDocumentActor::ResolveInventory() const
{
	AGameStateBase* GameState = UGameplayStatics::GetGameState(this);
	if (!IsValid(GameState))
	{
		return nullptr;
	}

	if (FObjectPropertyBase* InventoryProperty = FindFProperty<FObjectPropertyBase>(GameState->GetClass(), TEXT("Inventory")))
	{
		return InventoryProperty->GetObjectPropertyValue_InContainer(GameState);
	}

	for (TFieldIterator<FObjectPropertyBase> It(GameState->GetClass()); It; ++It)
	{
		FObjectPropertyBase* Property = *It;
		if (Property && Property->GetName().Contains(TEXT("Inventory")))
		{
			if (UObject* Candidate = Property->GetObjectPropertyValue_InContainer(GameState))
			{
				return Candidate;
			}
		}
	}

	return nullptr;
}

bool ARemainReadableDocumentActor::BuildDocumentRefStruct(void* StructMemory, UScriptStruct* StructType) const
{
	if (!StructMemory || !StructType)
	{
		return false;
	}

	const FName EffectiveDocumentTypeName = IsConfiguredAsBookDocument() ? FName(TEXT("Book")) : DocumentTypeName;
	UEnum* DocumentTypeEnum = LoadObject<UEnum>(nullptr, DocumentTypeEnumPath);
	const int64 DocumentTypeValue = ResolveEnumValue(DocumentTypeEnum, EffectiveDocumentTypeName);
	bool bSetAnyField = false;

	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
		{
			continue;
		}

		if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			if (Property->GetName().Contains(TEXT("RowName")) || Property->GetName().Contains(TEXT("Name")))
			{
				NameProperty->SetPropertyValue_InContainer(StructMemory, DocumentRowName);
				bSetAnyField = true;
			}
			continue;
		}

		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			UDataTable* EffectiveDataTable = GetEffectiveDocumentDataTable();
			if (ObjectProperty->PropertyClass && IsValid(EffectiveDataTable) && EffectiveDataTable->IsA(ObjectProperty->PropertyClass))
			{
				ObjectProperty->SetObjectPropertyValue_InContainer(StructMemory, EffectiveDataTable);
				bSetAnyField = true;
			}
			continue;
		}

		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == TBaseStructure<FDataTableRowHandle>::Get())
			{
				UDataTable* EffectiveDataTable = GetEffectiveDocumentDataTable();
				FDataTableRowHandle* Handle = StructProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(StructMemory);
				Handle->DataTable = EffectiveDataTable;
				Handle->RowName = DocumentRowName;
				bSetAnyField = true;
			}
			continue;
		}

		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(
				EnumProperty->ContainerPtrToValuePtr<void>(StructMemory),
				DocumentTypeValue);
			bSetAnyField = true;
			continue;
		}

		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			if (ByteProperty->Enum)
			{
				ByteProperty->SetIntPropertyValue(ByteProperty->ContainerPtrToValuePtr<void>(StructMemory), ResolveEnumValue(ByteProperty->Enum, EffectiveDocumentTypeName));
				bSetAnyField = true;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Built DocumentDT_Ref | Struct=%s | Table=%s | Row=%s | Type=%s | TypeValue=%lld"),
		*GetNameSafe(StructType),
		*GetNameSafe(GetEffectiveDocumentDataTable()),
		*DocumentRowName.ToString(),
		*EffectiveDocumentTypeName.ToString(),
		DocumentTypeValue);

	return bSetAnyField;
}

bool ARemainReadableDocumentActor::AddDocumentToInventory(UObject* Inventory, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;

	if (!IsValid(Inventory))
	{
		return false;
	}

	if (IsConfiguredAsBookDocument())
	{
		return AddBookDocumentToInventory(Inventory, OutIndex);
	}

	if (IsConfiguredAsGenericDocument())
	{
		return AddGenericDocumentToInventory(Inventory, OutIndex);
	}

	UFunction* AddDocumentFunction = Inventory->FindFunction(TEXT("AddDocumentAsDTRef"));
	UScriptStruct* DocumentRefStruct = LoadObject<UScriptStruct>(nullptr, DocumentRefStructPath);
	if (!AddDocumentFunction || !DocumentRefStruct)
	{
		return false;
	}

	FStructOnScope DocumentRef(DocumentRefStruct);
	if (!BuildDocumentRefStruct(DocumentRef.GetStructMemory(), DocumentRefStruct))
	{
		return false;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(AddDocumentFunction->ParmsSize);

	FProperty* IndexOutputProperty = nullptr;
	for (TFieldIterator<FProperty> It(AddDocumentFunction); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm))
		{
			continue;
		}

		if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			IndexOutputProperty = Property;
			continue;
		}

		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (!Property->HasAnyPropertyFlags(CPF_OutParm) && StructProperty->Struct == DocumentRefStruct)
			{
				DocumentRefStruct->CopyScriptStruct(StructProperty->ContainerPtrToValuePtr<void>(Params.GetData()), DocumentRef.GetStructMemory());
			}
			continue;
		}

		// Blueprint function return pins are reflected as normal OutParm values, not CPF_ReturnParm.
		if (Property->HasAnyPropertyFlags(CPF_OutParm) &&
			(CastField<FIntProperty>(Property) || CastField<FInt64Property>(Property) || CastField<FByteProperty>(Property)))
		{
			if (!IndexOutputProperty || Property->GetName().Contains(TEXT("Index")))
			{
				IndexOutputProperty = Property;
			}
		}
	}

	Inventory->ProcessEvent(AddDocumentFunction, Params.GetData());

	if (FIntProperty* IntReturn = CastField<FIntProperty>(IndexOutputProperty))
	{
		OutIndex = IntReturn->GetPropertyValue(IndexOutputProperty->ContainerPtrToValuePtr<void>(Params.GetData()));
		return true;
	}

	if (FInt64Property* Int64Return = CastField<FInt64Property>(IndexOutputProperty))
	{
		OutIndex = static_cast<int32>(Int64Return->GetPropertyValue(IndexOutputProperty->ContainerPtrToValuePtr<void>(Params.GetData())));
		return true;
	}

	if (FByteProperty* ByteReturn = CastField<FByteProperty>(IndexOutputProperty))
	{
		OutIndex = static_cast<int32>(ByteReturn->GetPropertyValue(IndexOutputProperty->ContainerPtrToValuePtr<void>(Params.GetData())));
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] AddDocumentAsDTRef had no readable integer output parameter | Inventory=%s | Function=%s"),
		*GetNameSafe(Inventory),
		*GetNameSafe(AddDocumentFunction));

	return false;
}

bool ARemainReadableDocumentActor::AddGenericDocumentToInventory(UObject* Inventory, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;

	if (!IsValid(Inventory))
	{
		return false;
	}

	FArrayProperty* DocumentsProperty = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("Documents"));
	FObjectPropertyBase* DocumentObjectProperty = DocumentsProperty ? CastField<FObjectPropertyBase>(DocumentsProperty->Inner) : nullptr;
	if (!DocumentsProperty || !DocumentObjectProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Inventory.Documents array was not found for manual generic document injection | Inventory=%s"),
			*GetNameSafe(Inventory));
		return false;
	}

	if (CachedDocumentIndex >= 0)
	{
		if (UObject* ExistingDocument = GetInventoryDocumentObject(Inventory, CachedDocumentIndex))
		{
			OutIndex = CachedDocumentIndex;
			return true;
		}
	}

	UObject* GenericDocument = CreateGenericDocumentObject(Inventory);
	if (!IsValid(GenericDocument))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Failed to create BP_GenericDocumentData object for row %s"), *DocumentRowName.ToString());
		return false;
	}

	FScriptArrayHelper DocumentsArray(DocumentsProperty, DocumentsProperty->ContainerPtrToValuePtr<void>(Inventory));
	OutIndex = DocumentsArray.AddValue();
	DocumentObjectProperty->SetObjectPropertyValue(DocumentsArray.GetRawPtr(OutIndex), GenericDocument);

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Manually injected BP_GenericDocumentData into inventory | Index=%d | Class=%s | Row=%s"),
		OutIndex,
		*GetNameSafe(GenericDocument->GetClass()),
		*DocumentRowName.ToString());

	return true;
}

bool ARemainReadableDocumentActor::AddBookDocumentToInventory(UObject* Inventory, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;

	if (!IsValid(Inventory))
	{
		return false;
	}

	FArrayProperty* DocumentsProperty = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("Documents"));
	FObjectPropertyBase* DocumentObjectProperty = DocumentsProperty ? CastField<FObjectPropertyBase>(DocumentsProperty->Inner) : nullptr;
	if (!DocumentsProperty || !DocumentObjectProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Inventory.Documents array was not found for manual book injection | Inventory=%s"),
			*GetNameSafe(Inventory));
		return false;
	}

	if (CachedDocumentIndex >= 0)
	{
		if (UObject* ExistingDocument = GetInventoryDocumentObject(Inventory, CachedDocumentIndex))
		{
			OutIndex = CachedDocumentIndex;
			return true;
		}
	}

	UObject* BookDocument = CreateBookDocumentObject(Inventory);
	if (!IsValid(BookDocument))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Failed to create BP_BookData object for row %s"), *DocumentRowName.ToString());
		return false;
	}

	FScriptArrayHelper DocumentsArray(DocumentsProperty, DocumentsProperty->ContainerPtrToValuePtr<void>(Inventory));
	OutIndex = DocumentsArray.AddValue();
	DocumentObjectProperty->SetObjectPropertyValue(DocumentsArray.GetRawPtr(OutIndex), BookDocument);

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Manually injected BP_BookData into inventory | Index=%d | Class=%s | Row=%s"),
		OutIndex,
		*GetNameSafe(BookDocument->GetClass()),
		*DocumentRowName.ToString());

	return true;
}

UObject* ARemainReadableDocumentActor::CreateGenericDocumentObject(UObject* Outer) const
{
	UClass* GenericDocumentClass = LoadClass<UObject>(nullptr, GenericDocumentClassPath);
	if (!GenericDocumentClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Could not load BP_GenericDocumentData class at %s"), GenericDocumentClassPath);
		return nullptr;
	}

	UObject* GenericDocument = NewObject<UObject>(Outer ? Outer : GetTransientPackage(), GenericDocumentClass);
	if (!IsValid(GenericDocument))
	{
		return nullptr;
	}

	if (FNameProperty* RowNameProperty = FindFProperty<FNameProperty>(GenericDocument->GetClass(), TEXT("DataTableRowName")))
	{
		RowNameProperty->SetPropertyValue_InContainer(GenericDocument, DocumentRowName);
	}

	for (TFieldIterator<FNameProperty> It(GenericDocument->GetClass()); It; ++It)
	{
		FNameProperty* NameProperty = *It;
		if (!NameProperty)
		{
			continue;
		}

		if (NameProperty->GetName().Contains(TEXT("DataTableRowName")) || NameProperty->GetName().Contains(TEXT("RowName")))
		{
			NameProperty->SetPropertyValue_InContainer(GenericDocument, DocumentRowName);
		}
	}

	return GenericDocument;
}

UObject* ARemainReadableDocumentActor::CreateBookDocumentObject(UObject* Outer) const
{
	UClass* BookDocumentClass = LoadClass<UObject>(nullptr, BookDocumentClassPath);
	if (!BookDocumentClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Could not load BP_BookData class at %s"), BookDocumentClassPath);
		return nullptr;
	}

	UObject* BookDocument = NewObject<UObject>(Outer ? Outer : GetTransientPackage(), BookDocumentClass);
	if (!IsValid(BookDocument))
	{
		return nullptr;
	}

	if (FNameProperty* RowNameProperty = FindFProperty<FNameProperty>(BookDocument->GetClass(), TEXT("DataTableRowName")))
	{
		RowNameProperty->SetPropertyValue_InContainer(BookDocument, DocumentRowName);
	}

	for (TFieldIterator<FNameProperty> It(BookDocument->GetClass()); It; ++It)
	{
		FNameProperty* NameProperty = *It;
		if (!NameProperty)
		{
			continue;
		}

		if (NameProperty->GetName().Contains(TEXT("DataTableRowName")) || NameProperty->GetName().Contains(TEXT("RowName")))
		{
			NameProperty->SetPropertyValue_InContainer(BookDocument, DocumentRowName);
		}
	}

	return BookDocument;
}

bool ARemainReadableDocumentActor::OpenGenericDocumentDirect(APlayerController* PlayerController, UObject* DocumentObject) const
{
	if (!IsValid(DocumentObject))
	{
		return false;
	}

	if (!ActivateDocumentScreen(PlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Direct generic document open failed: could not activate Document UI | Document=%s"),
			*GetNameSafe(DocumentObject));
		return false;
	}

	if (!TryPatchGenericDocumentScreen(PlayerController, DocumentObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Direct generic document open failed: could not patch UI_GenericDocumentPage | Document=%s"),
			*GetNameSafe(DocumentObject));
		return false;
	}

	return true;
}

bool ARemainReadableDocumentActor::OpenBookDocumentDirect(APlayerController* PlayerController, UObject* DocumentObject) const
{
	if (!IsValid(DocumentObject))
	{
		return false;
	}

	if (!ActivateDocumentScreen(PlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Direct book open failed: could not activate Document UI | Document=%s"),
			*GetNameSafe(DocumentObject));
		return false;
	}

	if (!TryPatchBookDocumentScreen(PlayerController, DocumentObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Direct book open failed: could not patch UI_BookPage | Document=%s"),
			*GetNameSafe(DocumentObject));
		return false;
	}

	return true;
}

bool ARemainReadableDocumentActor::ActivateDocumentScreen(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController))
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (!IsValid(PlayerController))
	{
		return false;
	}

	AHUD* HUD = PlayerController->GetHUD();
	if (!IsValid(HUD))
	{
		return false;
	}

	UFunction* SetActiveUIFunction = HUD->FindFunction(TEXT("SetActiveUI"));
	if (!SetActiveUIFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] HUD.SetActiveUI was not found | HUD=%s"), *GetNameSafe(HUD));
		return false;
	}

	UEnum* ActiveScreenEnum = LoadObject<UEnum>(nullptr, ActiveScreenEnumPath);
	const int64 FallbackDocumentValue = ResolveEnumValue(ActiveScreenEnum, TEXT("Document"));

	TArray<uint8> Params;
	Params.SetNumZeroed(SetActiveUIFunction->ParmsSize);

	bool bSetScreenParam = false;
	for (TFieldIterator<FProperty> It(SetActiveUIFunction); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			// SetActiveUI's bool input is ShowMouseCursor in the template HUD.
			BoolProperty->SetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(Params.GetData()), true);
			continue;
		}

		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(
				EnumProperty->ContainerPtrToValuePtr<void>(Params.GetData()),
				ResolveEnumValue(EnumProperty->GetEnum(), TEXT("Document")));
			bSetScreenParam = true;
			continue;
		}

		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			if (ByteProperty->Enum)
			{
				ByteProperty->SetIntPropertyValue(
					ByteProperty->ContainerPtrToValuePtr<void>(Params.GetData()),
					ResolveEnumValue(ByteProperty->Enum, TEXT("Document")));
				bSetScreenParam = true;
			}
			else if (!bSetScreenParam && Property->GetName().Contains(TEXT("UI")))
			{
				ByteProperty->SetIntPropertyValue(ByteProperty->ContainerPtrToValuePtr<void>(Params.GetData()), FallbackDocumentValue);
				bSetScreenParam = true;
			}
		}
	}

	if (!bSetScreenParam)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] HUD.SetActiveUI had no enum/byte screen parameter | HUD=%s | Function=%s"),
			*GetNameSafe(HUD),
			*GetNameSafe(SetActiveUIFunction));
		return false;
	}

	HUD->ProcessEvent(SetActiveUIFunction, Params.GetData());

	FObjectPropertyBase* DocumentScreenProperty = FindFProperty<FObjectPropertyBase>(HUD->GetClass(), TEXT("DocumentScreen"));
	UObject* DocumentScreen = DocumentScreenProperty ? DocumentScreenProperty->GetObjectPropertyValue_InContainer(HUD) : nullptr;
	const bool bActivatedDocumentScreen = IsValid(DocumentScreen);
	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Activated Document UI for book | HUD=%s | DocumentScreen=%s"),
		*GetNameSafe(HUD),
		*GetNameSafe(DocumentScreen));

	return bActivatedDocumentScreen;
}

bool ARemainReadableDocumentActor::ShowDocument(APlayerController* PlayerController, int32 Index) const
{
	if (!IsValid(PlayerController))
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (!IsValid(PlayerController))
	{
		return false;
	}

	AHUD* HUD = PlayerController->GetHUD();
	if (!IsValid(HUD))
	{
		return false;
	}

	UFunction* ShowDocumentFunction = HUD->FindFunction(TEXT("ShowDocument"));
	if (!ShowDocumentFunction)
	{
		return false;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(ShowDocumentFunction->ParmsSize);

	bool bSetIndex = false;
	for (TFieldIterator<FProperty> It(ShowDocumentFunction); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		if (SetIntegerParamValue<FIntProperty>(Property, Params.GetData(), Index) ||
			SetIntegerParamValue<FInt64Property>(Property, Params.GetData(), Index) ||
			SetIntegerParamValue<FByteProperty>(Property, Params.GetData(), Index))
		{
			bSetIndex = true;
			break;
		}
	}

	if (!bSetIndex)
	{
		return false;
	}

	HUD->ProcessEvent(ShowDocumentFunction, Params.GetData());
	return true;
}

FString ARemainReadableDocumentActor::GetInventoryDocumentClassName(UObject* Inventory, int32 Index) const
{
	UObject* DocumentObject = GetInventoryDocumentObject(Inventory, Index);
	if (IsValid(DocumentObject))
	{
		return GetNameSafe(DocumentObject->GetClass());
	}

	if (!IsValid(Inventory) || Index < 0)
	{
		return TEXT("<invalid>");
	}

	return TEXT("<not found>");
}

UObject* ARemainReadableDocumentActor::GetInventoryDocumentObject(UObject* Inventory, int32 Index) const
{
	if (!IsValid(Inventory) || Index < 0)
	{
		return nullptr;
	}

	if (FArrayProperty* DocumentsProperty = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("Documents")))
	{
		FScriptArrayHelper ArrayHelper(DocumentsProperty, DocumentsProperty->ContainerPtrToValuePtr<void>(Inventory));
		if (ArrayHelper.IsValidIndex(Index))
		{
			if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(DocumentsProperty->Inner))
			{
				return ObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index));
			}
		}
	}

	for (TFieldIterator<FArrayProperty> It(Inventory->GetClass()); It; ++It)
	{
		FArrayProperty* ArrayProperty = *It;
		if (!ArrayProperty || !ArrayProperty->GetName().Contains(TEXT("Document")))
		{
			continue;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Inventory));
		if (!ArrayHelper.IsValidIndex(Index))
		{
			continue;
		}

		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner))
		{
			return ObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index));
		}
	}

	return nullptr;
}

bool ARemainReadableDocumentActor::TryPatchGenericDocumentScreen(APlayerController* PlayerController, UObject* DocumentObject) const
{
	if (!IsValid(PlayerController))
	{
		return false;
	}

	AHUD* HUD = PlayerController->GetHUD();
	if (!IsValid(HUD))
	{
		return false;
	}

	FObjectPropertyBase* DocumentScreenProperty = FindFProperty<FObjectPropertyBase>(HUD->GetClass(), TEXT("DocumentScreen"));
	UObject* DocumentScreen = DocumentScreenProperty ? DocumentScreenProperty->GetObjectPropertyValue_InContainer(HUD) : nullptr;
	if (!IsValid(DocumentScreen))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] HUD has no valid DocumentScreen to patch generic document | HUD=%s"), *GetNameSafe(HUD));
		return false;
	}

	UUserWidget* GenericDocumentPage = CreateGenericDocumentPageWidget(PlayerController, DocumentScreen, DocumentObject);
	if (!IsValid(GenericDocumentPage))
	{
		return false;
	}

	FObjectPropertyBase* NamedSlotProperty = FindFProperty<FObjectPropertyBase>(DocumentScreen->GetClass(), TEXT("NamedSlot_0"));
	UWidget* NamedSlotWidget = NamedSlotProperty ? Cast<UWidget>(NamedSlotProperty->GetObjectPropertyValue_InContainer(DocumentScreen)) : nullptr;
	if (!IsValid(NamedSlotWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] UI_DocumentScreen.NamedSlot_0 was not found for generic document | Screen=%s"),
			*GetNameSafe(DocumentScreen));
		return false;
	}

	SetDocumentScreenDocument(DocumentScreen, DocumentObject);
	SetDocumentScreenTypeToGeneric(DocumentScreen);
	SetObjectProperty(DocumentScreen, TEXT("DocumentPage"), GenericDocumentPage);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("Content"), ESlateVisibility::Visible);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("Empty"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TranscriptionText"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TextCursor"), ESlateVisibility::Collapsed);

	if (FBoolProperty* TranscriptionToggleableProperty = FindFProperty<FBoolProperty>(DocumentScreen->GetClass(), TEXT("TranscriptionToggleable")))
	{
		TranscriptionToggleableProperty->SetPropertyValue_InContainer(DocumentScreen, false);
	}

	bool bInsertedPage = false;
	if (UPanelWidget* PanelSlot = Cast<UPanelWidget>(NamedSlotWidget))
	{
		PanelSlot->ClearChildren();
		PanelSlot->AddChild(GenericDocumentPage);
		bInsertedPage = true;
	}
	else if (UContentWidget* ContentSlot = Cast<UContentWidget>(NamedSlotWidget))
	{
		ContentSlot->SetContent(GenericDocumentPage);
		bInsertedPage = true;
	}

	if (!bInsertedPage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] UI_DocumentScreen.NamedSlot_0 has unsupported widget class for generic document | Screen=%s | Slot=%s | SlotClass=%s"),
			*GetNameSafe(DocumentScreen),
			*GetNameSafe(NamedSlotWidget),
			*GetNameSafe(NamedSlotWidget->GetClass()));
		return false;
	}

	if (UFunction* GenericPageUpdateFunction = GenericDocumentPage->FindFunction(TEXT("Update")))
	{
		GenericDocumentPage->ProcessEvent(GenericPageUpdateFunction, nullptr);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Patched document screen to UI_GenericDocumentPage | Screen=%s | Page=%s | Document=%s"),
		*GetNameSafe(DocumentScreen),
		*GetNameSafe(GenericDocumentPage),
		*GetNameSafe(DocumentObject));

	return true;
}

bool ARemainReadableDocumentActor::TryPatchBookDocumentScreen(APlayerController* PlayerController, UObject* DocumentObject) const
{
	if (!IsValid(PlayerController))
	{
		return false;
	}

	AHUD* HUD = PlayerController->GetHUD();
	if (!IsValid(HUD))
	{
		return false;
	}

	FObjectPropertyBase* DocumentScreenProperty = FindFProperty<FObjectPropertyBase>(HUD->GetClass(), TEXT("DocumentScreen"));
	UObject* DocumentScreen = DocumentScreenProperty ? DocumentScreenProperty->GetObjectPropertyValue_InContainer(HUD) : nullptr;
	if (!IsValid(DocumentScreen))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] HUD has no valid DocumentScreen to patch | HUD=%s"), *GetNameSafe(HUD));
		return false;
	}

	UUserWidget* BookPage = CreateBookPageWidget(PlayerController, DocumentScreen, DocumentObject);
	if (!IsValid(BookPage))
	{
		return false;
	}

	FObjectPropertyBase* NamedSlotProperty = FindFProperty<FObjectPropertyBase>(DocumentScreen->GetClass(), TEXT("NamedSlot_0"));
	UWidget* NamedSlotWidget = NamedSlotProperty ? Cast<UWidget>(NamedSlotProperty->GetObjectPropertyValue_InContainer(DocumentScreen)) : nullptr;
	if (!IsValid(NamedSlotWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] UI_DocumentScreen.NamedSlot_0 was not found | Screen=%s"),
			*GetNameSafe(DocumentScreen));
		return false;
	}

	SetDocumentScreenDocument(DocumentScreen, DocumentObject);
	SetDocumentScreenTypeToBook(DocumentScreen);
	SetObjectProperty(DocumentScreen, TEXT("DocumentPage"), BookPage);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("Content"), ESlateVisibility::Visible);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("Empty"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TranscriptionText"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TextCursor"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("PreviousPageArrow"), ESlateVisibility::Visible);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("NextPageArrow"), ESlateVisibility::Visible);

	if (FBoolProperty* TranscriptionToggleableProperty = FindFProperty<FBoolProperty>(DocumentScreen->GetClass(), TEXT("TranscriptionToggleable")))
	{
		TranscriptionToggleableProperty->SetPropertyValue_InContainer(DocumentScreen, false);
	}

	bool bInsertedBookPage = false;
	if (UPanelWidget* PanelSlot = Cast<UPanelWidget>(NamedSlotWidget))
	{
		PanelSlot->ClearChildren();
		PanelSlot->AddChild(BookPage);
		bInsertedBookPage = true;
	}
	else if (UContentWidget* ContentSlot = Cast<UContentWidget>(NamedSlotWidget))
	{
		ContentSlot->SetContent(BookPage);
		bInsertedBookPage = true;
	}

	if (!bInsertedBookPage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] UI_DocumentScreen.NamedSlot_0 has unsupported widget class | Screen=%s | Slot=%s | SlotClass=%s"),
			*GetNameSafe(DocumentScreen),
			*GetNameSafe(NamedSlotWidget),
			*GetNameSafe(NamedSlotWidget->GetClass()));
		return false;
	}

	if (UFunction* BookPageUpdateFunction = BookPage->FindFunction(TEXT("Update")))
	{
		BookPage->ProcessEvent(BookPageUpdateFunction, nullptr);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Patched document screen to UI_BookPage | Screen=%s | Page=%s | Document=%s"),
		*GetNameSafe(DocumentScreen),
		*GetNameSafe(BookPage),
		*GetNameSafe(DocumentObject));

	return true;
}

bool ARemainReadableDocumentActor::IsBookDocumentType() const
{
	return DocumentTypeName.ToString().Equals(TEXT("Book"), ESearchCase::IgnoreCase);
}

bool ARemainReadableDocumentActor::IsConfiguredAsBookDocument() const
{
	if (IsBookDocumentType())
	{
		return true;
	}

	const FString RowName = DocumentRowName.ToString();
	if (RowName.Contains(TEXT("Diary"), ESearchCase::IgnoreCase) ||
		RowName.Contains(TEXT("Dairy"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	const UDataTable* EffectiveTable = GetEffectiveDocumentDataTable();
	const FString TableName = GetNameSafe(EffectiveTable);
	return TableName.Contains(TEXT("Books_DataTable"), ESearchCase::IgnoreCase) ||
		TableName.Contains(TEXT("Book"), ESearchCase::IgnoreCase);
}

bool ARemainReadableDocumentActor::IsGenericDocumentType() const
{
	return DocumentTypeName.ToString().Equals(TEXT("GenericDocument"), ESearchCase::IgnoreCase) ||
		DocumentTypeName.ToString().Equals(TEXT("Generic Document"), ESearchCase::IgnoreCase);
}

bool ARemainReadableDocumentActor::IsConfiguredAsGenericDocument() const
{
	if (IsConfiguredAsBookDocument())
	{
		return false;
	}

	if (IsGenericDocumentType())
	{
		return true;
	}

	const UDataTable* EffectiveTable = GetEffectiveDocumentDataTable();
	const FString TableName = GetNameSafe(EffectiveTable);
	return TableName.Contains(TEXT("GenericDocuments_DataTable"), ESearchCase::IgnoreCase) ||
		TableName.Contains(TEXT("GenericDocument"), ESearchCase::IgnoreCase);
}

bool ARemainReadableDocumentActor::LoadDiaryPageTextures(TArray<UTexture2D*>& OutTextures) const
{
	OutTextures.Reset();

	UDataTable* EffectiveTable = GetEffectiveDocumentDataTable();
	if (!IsValid(EffectiveTable) || DocumentRowName.IsNone())
	{
		return false;
	}

	const UScriptStruct* RowStruct = EffectiveTable->GetRowStruct();
	const uint8* RowMemory = EffectiveTable->FindRowUnchecked(DocumentRowName);
	if (!RowStruct || !RowMemory)
	{
		return false;
	}

	FArrayProperty* PagesProperty = nullptr;
	for (TFieldIterator<FArrayProperty> It(RowStruct); It; ++It)
	{
		FArrayProperty* Candidate = *It;
		if (Candidate && Candidate->GetName().Contains(TEXT("Pages")))
		{
			PagesProperty = Candidate;
			break;
		}
	}

	if (!PagesProperty)
	{
		return false;
	}

	FStructProperty* PageStructProperty = CastField<FStructProperty>(PagesProperty->Inner);
	if (!PageStructProperty || !PageStructProperty->Struct)
	{
		return false;
	}

	FScriptArrayHelper PageArray(PagesProperty, PagesProperty->ContainerPtrToValuePtr<void>(RowMemory));
	for (int32 PageIndex = 0; PageIndex < PageArray.Num(); ++PageIndex)
	{
		void* PageMemory = PageArray.GetRawPtr(PageIndex);
		if (!PageMemory)
		{
			continue;
		}

		for (TFieldIterator<FProperty> PageIt(PageStructProperty->Struct); PageIt; ++PageIt)
		{
			FProperty* Property = *PageIt;
			if (!Property || !Property->GetName().Contains(TEXT("Texture")))
			{
				continue;
			}

			UTexture2D* LoadedTexture = nullptr;
			if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
			{
				FSoftObjectPtr SoftObject = SoftObjectProperty->GetPropertyValue_InContainer(PageMemory);
				LoadedTexture = Cast<UTexture2D>(SoftObject.LoadSynchronous());
			}
			else if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
			{
				LoadedTexture = Cast<UTexture2D>(ObjectProperty->GetObjectPropertyValue_InContainer(PageMemory));
			}

			if (IsValid(LoadedTexture))
			{
				OutTextures.Add(LoadedTexture);
			}

			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Loaded diary page textures | Row=%s | Count=%d"),
		*DocumentRowName.ToString(),
		OutTextures.Num());

	return OutTextures.Num() > 0;
}

void ARemainReadableDocumentActor::HideDocumentScreenChrome(UObject* DocumentScreen) const
{
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("ActionList"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("ActionListRef"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("PreviousPageArrow"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("NextPageArrow"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TranscriptionText"), ESlateVisibility::Collapsed);
	SetWidgetVisibilityProperty(DocumentScreen, TEXT("TextCursor"), ESlateVisibility::Collapsed);
}

bool ARemainReadableDocumentActor::SetDocumentScreenDocument(UObject* DocumentScreen, UObject* DocumentObject) const
{
	return SetObjectProperty(DocumentScreen, TEXT("Document"), DocumentObject);
}

bool ARemainReadableDocumentActor::SetDocumentScreenTypeToGeneric(UObject* DocumentScreen) const
{
	if (!IsValid(DocumentScreen))
	{
		return false;
	}

	UEnum* DocumentTypeEnum = LoadObject<UEnum>(nullptr, DocumentTypeEnumPath);
	const int64 GenericValue = ResolveEnumValue(DocumentTypeEnum, TEXT("GenericDocument"));

	if (FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(DocumentScreen->GetClass(), TEXT("CurrentDocumentType")))
	{
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(DocumentScreen), GenericValue);
		return true;
	}

	if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(DocumentScreen->GetClass(), TEXT("CurrentDocumentType")))
	{
		ByteProperty->SetIntPropertyValue(
			ByteProperty->ContainerPtrToValuePtr<void>(DocumentScreen),
			ByteProperty->Enum ? ResolveEnumValue(ByteProperty->Enum, TEXT("GenericDocument")) : GenericValue);
		return true;
	}

	return false;
}

bool ARemainReadableDocumentActor::SetDocumentScreenTypeToBook(UObject* DocumentScreen) const
{
	if (!IsValid(DocumentScreen))
	{
		return false;
	}

	UEnum* DocumentTypeEnum = LoadObject<UEnum>(nullptr, DocumentTypeEnumPath);
	const int64 BookValue = ResolveEnumValue(DocumentTypeEnum, TEXT("Book"));

	if (FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(DocumentScreen->GetClass(), TEXT("CurrentDocumentType")))
	{
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(DocumentScreen), BookValue);
		return true;
	}

	if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(DocumentScreen->GetClass(), TEXT("CurrentDocumentType")))
	{
		ByteProperty->SetIntPropertyValue(
			ByteProperty->ContainerPtrToValuePtr<void>(DocumentScreen),
			ByteProperty->Enum ? ResolveEnumValue(ByteProperty->Enum, TEXT("Book")) : BookValue);
		return true;
	}

	return false;
}

bool ARemainReadableDocumentActor::SetObjectProperty(UObject* Target, FName PropertyName, UObject* Value) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Target->GetClass(), PropertyName);
	if (!ObjectProperty)
	{
		return false;
	}

	ObjectProperty->SetObjectPropertyValue_InContainer(Target, Value);
	return true;
}

bool ARemainReadableDocumentActor::SetWidgetVisibilityProperty(UObject* Target, FName PropertyName, ESlateVisibility Visibility) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Target->GetClass(), PropertyName);
	UWidget* Widget = ObjectProperty ? Cast<UWidget>(ObjectProperty->GetObjectPropertyValue_InContainer(Target)) : nullptr;
	if (!IsValid(Widget))
	{
		return false;
	}

	Widget->SetVisibility(Visibility);
	return true;
}

UUserWidget* ARemainReadableDocumentActor::CreateGenericDocumentPageWidget(APlayerController* PlayerController, UObject* DocumentScreen, UObject* DocumentObject) const
{
	if (!IsValid(PlayerController) || !IsValid(DocumentScreen))
	{
		return nullptr;
	}

	UClass* GenericPageClass = LoadClass<UUserWidget>(nullptr, GenericDocumentPageWidgetClassPath);
	if (!GenericPageClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Could not load UI_GenericDocumentPage class at %s"), GenericDocumentPageWidgetClassPath);
		return nullptr;
	}

	UUserWidget* GenericPage = CreateWidget<UUserWidget>(PlayerController, GenericPageClass);
	if (!IsValid(GenericPage))
	{
		return nullptr;
	}

	if (IsValid(DocumentObject))
	{
		SetObjectProperty(GenericPage, TEXT("Document"), DocumentObject);
	}
	SetObjectProperty(GenericPage, TEXT("DocumentScreenRef"), DocumentScreen);
	SetObjectProperty(GenericPage, TEXT("Document Screen Ref"), DocumentScreen);

	FObjectPropertyBase* ActionListProperty = FindFProperty<FObjectPropertyBase>(DocumentScreen->GetClass(), TEXT("ActionList"));
	UObject* ActionList = ActionListProperty ? ActionListProperty->GetObjectPropertyValue_InContainer(DocumentScreen) : nullptr;
	if (IsValid(ActionList))
	{
		SetObjectProperty(GenericPage, TEXT("ActionListRef"), ActionList);
		SetObjectProperty(GenericPage, TEXT("ActionList Ref"), ActionList);
	}

	return GenericPage;
}

UUserWidget* ARemainReadableDocumentActor::CreateBookPageWidget(APlayerController* PlayerController, UObject* DocumentScreen, UObject* DocumentObject) const
{
	if (!IsValid(PlayerController) || !IsValid(DocumentScreen))
	{
		return nullptr;
	}

	UClass* BookPageClass = LoadClass<UUserWidget>(nullptr, BookPageWidgetClassPath);
	if (!BookPageClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] Could not load UI_BookPage class at %s"), BookPageWidgetClassPath);
		return nullptr;
	}

	UUserWidget* BookPage = CreateWidget<UUserWidget>(PlayerController, BookPageClass);
	if (!IsValid(BookPage))
	{
		return nullptr;
	}

	if (IsValid(DocumentObject))
	{
		SetObjectProperty(BookPage, TEXT("Document"), DocumentObject);
	}
	SetObjectProperty(BookPage, TEXT("DocumentScreenRef"), DocumentScreen);
	SetObjectProperty(BookPage, TEXT("Document Screen Ref"), DocumentScreen);

	FObjectPropertyBase* ActionListProperty = FindFProperty<FObjectPropertyBase>(DocumentScreen->GetClass(), TEXT("ActionList"));
	UObject* ActionList = ActionListProperty ? ActionListProperty->GetObjectPropertyValue_InContainer(DocumentScreen) : nullptr;
	if (IsValid(ActionList))
	{
		SetObjectProperty(BookPage, TEXT("ActionListRef"), ActionList);
		SetObjectProperty(BookPage, TEXT("ActionList Ref"), ActionList);
	}

	return BookPage;
}

void ARemainReadableDocumentActor::DebugMessage(const FString& Message) const
{
	UE_LOG(LogTemp, Warning, TEXT("[RemainReadableDocumentActor] %s | Actor=%s | Row=%s | Type=%s"),
		*Message,
		*GetNameSafe(this),
		*DocumentRowName.ToString(),
		*DocumentTypeName.ToString());

	if (bDebugDocumentOpen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, FString::Printf(TEXT("ReadableDocument: %s"), *Message));
	}
}
