#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RemainWorkbenchPartIdProvider.generated.h"

UINTERFACE(BlueprintType)
class HORRORMECHANICS_API URemainWorkbenchPartIdProvider : public UInterface
{
	GENERATED_BODY()
};

class HORRORMECHANICS_API IRemainWorkbenchPartIdProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Remain|Workbench")
	FName GetWorkbenchPartId() const;

	virtual FName GetWorkbenchPartId_Implementation() const
	{
		return NAME_None;
	}
};
