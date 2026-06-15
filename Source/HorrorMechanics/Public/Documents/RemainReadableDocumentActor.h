#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/Actor.h"
#include "RemainReadableDocumentActor.generated.h"

class APlayerController;
struct FHitResult;
class UAudioComponent;
class UBoxComponent;
class UDataTable;
class UPrimitiveComponent;
class USoundBase;
class UStaticMeshComponent;
class UTexture2D;
class UUserWidget;

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainReadableDocumentActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainReadableDocumentActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Document")
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Document")
	TObjectPtr<UBoxComponent> InteractionBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	TObjectPtr<UDataTable> DocumentDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	FName DocumentRowName = TEXT("Dairy_01");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	FName DocumentTypeName = TEXT("Book");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	bool bDestroyAfterOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	bool bPlayCollectSound = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document")
	TObjectPtr<USoundBase> CollectSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Debug")
	bool bDebugDocumentOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Template Proxy")
	bool bSpawnTemplateDocumentProxy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Template Proxy", meta=(EditCondition="bSpawnTemplateDocumentProxy"))
	TSubclassOf<AActor> TemplateDocumentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Direct Input")
	bool bEnableDirectInteractInputFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Direct Input", meta=(EditCondition="bEnableDirectInteractInputFallback"))
	FName DirectInteractActionName = TEXT("Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Direct Input", meta=(EditCondition="bEnableDirectInteractInputFallback", ClampMin="50.0"))
	float DirectInteractTraceDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Direct Input", meta=(EditCondition="bEnableDirectInteractInputFallback"))
	TEnumAsByte<ECollisionChannel> DirectInteractTraceChannel = ECC_Visibility;

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	void OpenReadableDocument(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	bool OpenDocument(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category="Remain|Interaction")
	UPARAM(DisplayName="Yes") bool IsReadableDocumentInteractionDisabled(UPrimitiveComponent* Component) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedTemplateDocument = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> DirectInputPlayerController = nullptr;

	bool bDirectInputEnabled = false;
	bool bDirectInputBound = false;
	bool bDocumentOpened = false;
	int32 CachedDocumentIndex = INDEX_NONE;

	void SpawnTemplateDocumentProxy();
	bool ConfigureTemplateDocumentProxy(AActor* ProxyActor) const;
	void HideTemplateDocumentProxyVisuals(AActor* ProxyActor) const;
	void DisableNativeInteractionCollision();
	void UpdateDirectInteractInputFallback();
	bool IsPlayerLookingAtReadableDocument(APlayerController* PlayerController) const;
	bool IsTraceHitReadableDocument(const FHitResult& Hit) const;
	void EnableDirectInteractInput(APlayerController* PlayerController);
	void DisableDirectInteractInput();
	void HandleDirectInteractInput();
	UDataTable* GetEffectiveDocumentDataTable() const;
	UObject* ResolveInventory() const;
	bool BuildDocumentRefStruct(void* StructMemory, UScriptStruct* StructType) const;
	bool AddDocumentToInventory(UObject* Inventory, int32& OutIndex) const;
	bool AddGenericDocumentToInventory(UObject* Inventory, int32& OutIndex) const;
	bool AddBookDocumentToInventory(UObject* Inventory, int32& OutIndex) const;
	UObject* CreateGenericDocumentObject(UObject* Outer) const;
	UObject* CreateBookDocumentObject(UObject* Outer) const;
	bool OpenGenericDocumentDirect(APlayerController* PlayerController, UObject* DocumentObject) const;
	bool OpenBookDocumentDirect(APlayerController* PlayerController, UObject* DocumentObject) const;
	bool ActivateDocumentScreen(APlayerController* PlayerController) const;
	bool ShowDocument(APlayerController* PlayerController, int32 Index) const;
	UObject* GetInventoryDocumentObject(UObject* Inventory, int32 Index) const;
	FString GetInventoryDocumentClassName(UObject* Inventory, int32 Index) const;
	bool TryPatchGenericDocumentScreen(APlayerController* PlayerController, UObject* DocumentObject) const;
	bool TryPatchBookDocumentScreen(APlayerController* PlayerController, UObject* DocumentObject) const;
	bool IsBookDocumentType() const;
	bool IsConfiguredAsBookDocument() const;
	bool IsGenericDocumentType() const;
	bool IsConfiguredAsGenericDocument() const;
	bool LoadDiaryPageTextures(TArray<UTexture2D*>& OutTextures) const;
	void HideDocumentScreenChrome(UObject* DocumentScreen) const;
	bool SetDocumentScreenDocument(UObject* DocumentScreen, UObject* DocumentObject) const;
	bool SetDocumentScreenTypeToGeneric(UObject* DocumentScreen) const;
	bool SetDocumentScreenTypeToBook(UObject* DocumentScreen) const;
	bool SetObjectProperty(UObject* Target, FName PropertyName, UObject* Value) const;
	bool SetWidgetVisibilityProperty(UObject* Target, FName PropertyName, ESlateVisibility Visibility) const;
	UUserWidget* CreateGenericDocumentPageWidget(APlayerController* PlayerController, UObject* DocumentScreen, UObject* DocumentObject) const;
	UUserWidget* CreateBookPageWidget(APlayerController* PlayerController, UObject* DocumentScreen, UObject* DocumentObject) const;
	void DebugMessage(const FString& Message) const;
};
