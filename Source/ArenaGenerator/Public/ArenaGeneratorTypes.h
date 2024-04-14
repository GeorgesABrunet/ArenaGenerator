/*
 * MIT License
 *
 * Copyright (c) 2024 Georges Brunet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ArenaGeneratorTypes.generated.h"

/* Arena Generator Types
* This file is meant to contain all defined data types for this plugin.
* They are consolidated here for convenience.
*/

#pragma region Enums

//Arena Build Order Rules determine what section of the arena should be generated first
//and how other sections should line up to it. A section leading by dimensions will use
//grid parameters from configurations. Leading by radius will override some configurations
//and align the arena to those dimensions.
UENUM(BlueprintType)
enum class EArenaBuildOrderRules : uint8
{
	GridLeadsByDimensions,
	GridLeadsByRadius,
	PolygonLeadByDimensions,
	PolygonLeadByRadius, 

};

/*
* Origin Placement Type determines where the local bounds of the
* relevant mesh/actor/object extends in relation to the origin.
*/
UENUM(BlueprintType)
enum class EOriginPlacementType : uint8
{
	XY_Positive,
	XY_Negative,
	X_Positive_Y_Negative,
	X_Negative_Y_Positive,
	Center,
	//Custom, // TODO - add behavior for custom origins
	//...
};

/*
* Arena Section Type determines how the pattern should be built. 
* HorizontalGrid = Floor or ceiling/roof in most cases,
* Polygon = Walls or Domed roofs and the like,
*/
UENUM(BlueprintType)
enum class EArenaSectionType : uint8
{
	HorizontalGrid,
	Polygon,
};

/*
* What orientation modification logic we should use during placement
*/
UENUM(BlueprintType)
enum class EPlacementOrientationRule : uint8
{
	None,
	RotateByYP,//
	RotateYawRandomly, //random float between 0-360 will be assigned to the yaw
	//...
};

UENUM(BlueprintType)
enum class ETypeToPlace : uint8
{
	StaticMeshes,
	Actors,
};
#pragma endregion

#pragma region Structs
USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaMesh : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOriginPlacementType OriginType = EOriginPlacementType::XY_Positive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;
};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaMeshGroupConfig : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MeshDimensions = FVector{ 500, 500, 500 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MeshScale = FVector{ 1 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArenaMesh> GroupMeshes;

};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaActorConfig : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ActorDimensions = FVector{ 100, 100, 100 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector ActorScale = FVector{ 1 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AActor>> ClassesToSpawn;
};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaSectionBuildRules : public FTableRowBase
{
	GENERATED_BODY()

	//Determine the type of section this will be.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	EArenaSectionType SectionType = EArenaSectionType::Polygon;

	//How many times should we repeat this pattern?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
		int SectionAmount = 1;

	//Type of object to place
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
		ETypeToPlace AssetToPlace = ETypeToPlace::StaticMeshes;

	//Which Object Group index should be referred to for object picking patterns
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
		int ObjectGroupId = 0;

	//Whether or not this section should update the origin offset as it iterates the SectionAmount
	//TODO - not yet implemented
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
		bool bUpdatesOriginOffsetHeight = true;

	// ROTATE PARAMS
	
	//Default rotation will be applied before placement logic affects rotation additively.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
		FRotator DefaultRotation = FRotator(0.f);

	//Should we rotated pieces in this section and how
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
		EPlacementOrientationRule RotationRule = EPlacementOrientationRule::None;

	//We divide 360 by this number and use the result as what to multiply by a random int value
	//to get clean rotations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orientation")
		int YawPossibilities = 4;

	//WARP PARAMS

	//Whether we should warp placement or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warping")
		bool bWarpPlacement = false;

	// This determines the per-axis range of warping.
	// For polygons this will respectively affect warping along forward =x, right=y, up=z vectors.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warping")
		FVector WarpRange = FVector(0.f, 0.f, 1.f);

	// This determines how concave should the section pieces be.  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warping")
		float WarpConcavityStrength = 0.f;

	//OFFSET PARAMS

	//How many times should we multiply the right vector of the placement direction by the mesh size
	//at start of placement for the section. This allows offseting pieces forward or backwards
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offsets")
		float InitOffsetByWidthScalar = 0.f;

	//For however many levels there are this section, how much should we offset the next level through the 
	// placement direction's right vector * the mesh width. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offsets")
		float OffsetByWidthIncrement = 0.f;


	//How many times should we initially offset this section by this scalar of the associated mesh groups height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offsets")
		float InitOffsetByHeightScalar = 0.f;

	//For however many levels there are this section, how much should we offset the next level's
	//height increment by in relation to its mesh height. Default is 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offsets")
		float OffsetByHeightIncrement = 1.f;
	
};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaSectionTargets : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetInscribedRadius = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetPolygonSides = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetTilesPerSide = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetGridDimensions = 15;

};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaSection : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EArenaBuildOrderRules SectionBuildOrderRules = EArenaBuildOrderRules::PolygonLeadByRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FArenaSectionTargets Targets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArenaSectionBuildRules> BuildRules;
};
#pragma endregion

