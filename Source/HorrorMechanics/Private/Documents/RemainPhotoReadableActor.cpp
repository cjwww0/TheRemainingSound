#include "Documents/RemainPhotoReadableActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARemainPhotoReadableActor::ARemainPhotoReadableActor()
{
	DocumentTypeName = TEXT("GenericDocument");
	bDestroyAfterOpen = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PictureMeshFinder(TEXT("/Game/HorrorMechanics/ExampleAssets/Examinables/More/Picture/Meshes/SM_Picture.SM_Picture"));
	if (PictureMeshFinder.Succeeded() && Mesh)
	{
		Mesh->SetStaticMesh(PictureMeshFinder.Object);
	}
}
