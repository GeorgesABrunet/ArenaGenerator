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
#include "GameFramework/Actor.h"
#include "ArenaGeneratorTypes.h"
#include "BaseArenaGenerator.generated.h"

UCLASS(Blueprintable, ClassGroup = "Arena Generator")
class ARENAGENERATOR_API ABaseArenaGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseArenaGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	

	//Will generate an Arena based on provided patterns in PatternList
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Arena")
	virtual void GenerateArena();

	//Deletes all instances associated with actor. Does not clear parameters.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Arena")
	virtual void WipeArena();

	//Specific function for building out PatternList
	void BuildPatterns();

	UFUNCTION(BlueprintCallable, Category = "Arena")
	virtual void BuildSection(FArenaSectionBuildRules& Section);


private:


	//Calculates the definitive parameters of arena generation for 1 to 3 section arenas.
	virtual void CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules);

	FORCEINLINE float CalculateOpposite(float length, float angle);
	FORCEINLINE float CalculateAdjacent(float length, float angle);
	FORCEINLINE FVector ForwardVectorFromYaw(float yaw);

	//Returns a scalar vector to multiply a mesh size to get the necessary off such that the mesh spans positively across X and Y axes from the origin. Optimized for absolute directions, incorrect for angled directions.
	FVector RotatedMeshOffset(EOriginPlacementType OriginType, FVector& MeshSize, int RotationIndex);

	//Offsets mesh along FV and RV based on origin type. Optimized for angled directions.
	FVector OffsetMeshAlongDirections(const FVector& FV, const FVector& RV, EOriginPlacementType OriginType, const FVector& MeshSize, int RotationIndex);

	//Returns a scalar vector to multiply a mesh size with to get the necessary offset such that the origin sits in the center of the mesh.
	FVector MeshOriginOffsetScalar(EOriginPlacementType OriginType);

	FVector PlacementWarping(int ColMidpoint, int RowMidpoint, int Col, int Row, FVector OffsetRanges, float ConcavityStrength, FVector WarpDirection);


public:
//The values here are not meant to be directly modified by user input. 
//They are derived from calculations and visible for debugging purposes.
#pragma region Calculated Values

	//This represents the radius of the vertices of the polygon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	float InscribedRadius = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	float Apothem = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	float InteriorAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	float ExteriorAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	float SideLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	int32 ArenaSides = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	int32 ArenaDimensions = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Parameters | Geometry")
	int32 TilesPerArenaSide = 0;

#pragma endregion

//These are values the user can play around with. They will adhere to the arena build
// order rules and may not change any behavior depending on selected ruleset.
#pragma region User Inputs - Build Rules

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	EArenaBuildOrderRules ArenaBuildOrderRules = EArenaBuildOrderRules::PolygonLeadByDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	EOriginPlacementType ArenaPlacementOnActor = EOriginPlacementType::Center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	int32 ArenaSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	FRandomStream ArenaStream;

	//This determines if we should load mesh assets in asynchronously during generation. 
	//By default we load assets in synchronously. TODO - async loading
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	bool bLoadMeshesAsync = false;

	//TODO - map hierarchical instances as well
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets")
	bool bUseHierarchicalInstances = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
	int32 DesiredArenaSides = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
	int32 DesiredArenaFloorDimensions = 0;

	//Will determine the horizontal span of the arena. 
	//Final arena radius will differ based on build order rules.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
	float DesiredInscribedRadius = 0;

	//How many tiles should there be in the polygons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
	int32 DesiredTilesPerSide = 1;

	//Determines how many sides can the polygonal arena have. 
	//WARNING: Consider Tiles per side and build rules! 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
	int32 MaxSides = 120;

	//WARNING: Consider Tiles per side and build rules! 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Build Rule Targets - Dimensions")
		int32 MaxTilesPerSideRow = 100;

#pragma endregion

#pragma region User Inputs - Patterns

	//Based on build order rules, arena parameters are calculated with dependencies from user-input parameters.
	//At the moment this is necessary.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Patterns")
		int FocusGridIndex = 0;

	//Based on build order rules, arena parameters are calculated with dependencies from user-input parameters.
	//Using polygon based build order rules requires the first patterns with polygon build rules to be used as reference.
	//Will use index 1 if it fails to find a polygon pattern with polygonal build rules.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Patterns")
		int FocusPolygonIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Patterns")
		TArray<FArenaSectionBuildRules> PatternList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Parameters | Patterns")
		TArray<FArenaMeshGroupConfig> MeshGroups;

#pragma endregion

private:

#pragma region Section Exclusives

	FVector OriginOffset = FVector(0);
	TArray<TArray<UInstancedStaticMeshComponent*>> MeshInstances;
	TArray<int32> UsedGroupIndices;

	//Cached Values
	FVector PreviousMeshSize = FVector(0.f);
	int PreviousTilesPerSide = 0;
	FVector PreviousLastPosition = FVector(0.f);
	int TotalInstances;

#pragma endregion
};
