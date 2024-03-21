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

DECLARE_LOG_CATEGORY_EXTERN(LogArenaGenerator, All, All)

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Arena")
	void GenerateArena();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Arena")
	void WipeArena();

	void ParametrizeGeneration();

	//For Conventional 3-Piece Building
	void BuildFloor();
	void BuildWalls();
	void BuildRoof();


private:


	//Calculates the definitive parameters of arena generation for 1 to 3 section arenas.
	void CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules);

	FORCEINLINE float CalculateOpposite(float length, float angle);
	FORCEINLINE float CalculateAdjacent(float length, float angle);
	FORCEINLINE FVector ForwardVectorFromYaw(float yaw);

	FVector RotatedMeshOffset();

	FVector PlacementWarping(int ColMidpoint, int RowMidpoint, int Col, int Row, FVector OffsetRanges, float ConcavityStrength, FVector WarpDirection);


public:
//The values here are not meant to be directly modified by user input. 
//They are derived from calculations and visible for debugging purposes.
#pragma region Calculated Values

	//This represents the radius of the vertices of the polygon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	float InscribedRadius = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	float Apothem = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	float InteriorAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	float ExteriorAngle = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	float SideLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	int32 ArenaSides = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	int32 ArenaDimensions = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	int32 TilesPerArenaSide = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	FVector ArenaCenterLoc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	FVector FloorOriginOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	FVector WallOriginOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena | Geometry")
	FVector RoofOriginOffset;

#pragma endregion

//These are values the user can play around with. They will adhere to the arena build
// order rules and may not change any behavior depending on selected ruleset.
#pragma region User Inputs - Build Rules

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	EArenaBuildOrderRules ArenaBuildOrderRules = EArenaBuildOrderRules::FloorLeadsByDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bBuildArenaUsingPatternList = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bBuildArenaCenterOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 ArenaSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	FRandomStream ArenaStream;

	//This determines if we should load mesh assets in asynchronously during generation. 
	//By default we load assets in synchronously.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bLoadMeshesAsync = false;

	//TODO - map hierarchical instances as well
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bUseHierarchicalInstances = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	FThreePieceArenaBuildRules ArenaBuildRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	int32 DesiredArenaSides = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	int32 DesiredArenaFloorDimensions = 0;

	//Will determine the horizontal span of the arena. 
	//Final arena radius will differ based on build order rules.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	float DesiredInscribedRadius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	int32 DesiredTilesPerSide = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	int32 SideTileHeight = 1;

	//Determines how many sides can the polygonal arena have. 
	//WARNING: Consider Tiles per side and build rules! 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
	int32 MaxSides = 120;

	//WARNING: Consider Tiles per side and build rules! 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
		int32 MaxTilesPerSideRow = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets - 3 Section Arena")
		int32 RoofTileHeight = 1;

#pragma endregion

#pragma region User Inputs - Floor Configs

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		FFloorTransformRules FloorPlacementRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		FVector FloorMeshSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		FVector FloorMeshScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		TArray<UStaticMesh*> FloorMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		UMaterial* FloorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		FVector FloorWarpRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Floor")
		float FloorWarpConcavityStrength;

#pragma endregion

#pragma region User Inputs - Wall Configs

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		FWallTransformRules WallPlacementRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		FVector WallMeshSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		FVector WallMeshScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		TArray<UStaticMesh*> WallMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		UMaterial* WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		FVector WallWarpRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Wall")
		float WallWarpConcavityStrength;

#pragma endregion

#pragma region User Inputs - Roof Configs

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		FRoofTransformRules RoofPlacementRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		FVector RoofMeshSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		FVector RoofMeshScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		TArray<UStaticMesh*> RoofMeshes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		UMaterial* RoofMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		FVector RoofWarpRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		float RoofWarpConcavityStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		float StartingPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		int AdjustLeanEachIdxInc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		float PitchLeanAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | 3 Section Arena - Roof")
		bool AdjustPlacementToLean;

#pragma endregion

	

private:
	

	//Floors
	bool bFloorRotates = false;
	bool bMoveFloorWhenRotated = false;
	bool bWarpFloorPlacement = false;
	bool bWarpFloorScale = false;

	TArray<UInstancedStaticMeshComponent*> FloorMeshInstances;

	//Walls
	bool bWarpWallPlacement = false;
	bool bAddWallRotation = false;

	TArray<UInstancedStaticMeshComponent*> WallMeshInstances;

	//Roofs
	bool bBuildRoofAsCone = false;
	bool bBringRoofForward = false;
	bool bRoofIncrementsForwardEachLevel = false;
	bool bRoofRotates = false;
	bool bMoveRoofWhenRotated = false;
	bool bFlipRoofMeshes = false;
	bool bWarpRoofPlacement = false;

	TArray<UInstancedStaticMeshComponent*> RoofMeshInstances;
};
