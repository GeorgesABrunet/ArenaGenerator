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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void GenerateArena();

	//void WipeArena();

	void ParametrizeGeneration();
	void BuildFloor();
	void BuildWalls();


private:


	//Calculates the definitive parameters of arena generation.
	void CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules);

	FORCEINLINE float CalculateRightTriangleOpposite(float length, float angle);
	FORCEINLINE float CalculateRightTriangleAdjacent(float length, float angle);
	FORCEINLINE FVector ForwardVectorFromYaw(float yaw);

	FVector RotatedMeshOffset();


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

#pragma endregion

//These are values the user can play around with. They will adhere to the arena build
// order rules and may not change any behavior depending on selected ruleset.
#pragma region User Inputs - Build Rules

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	EArenaBuildOrderRules ArenaBuildOrderRules = EArenaBuildOrderRules::FloorLeadsByDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bBuildArenaCenterOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 DesiredArenaSides = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 DesiredArenaFloorDimensions = 0;

	//Will determine the horizontal span of the arena. 
	//Final arena radius will differ based on build order rules.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	float DesiredInscribedRadius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 DesiredTilesPerSide = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 SideTileHeight = 1;

	//Determines how many sides can the polygonal arena have. 
	//WARNING: Consider Tiles per side and build rules! 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 MaxSides = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	int32 ArenaSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	FRandomStream ArenaStream;

	//This determines if we should load mesh assets in asynchronously during generation. 
	//By default we load assets in synchronously.
	UPROPERTY(BlueprintReadWrite, Category = "Arena | Build Rule Targets")
	bool bLoadMeshesAsync = false;

#pragma endregion

#pragma region User Inputs - Mesh Configs
	UPROPERTY(BlueprintReadWrite, Category = "Arena | Floor")
		FVector FloorMeshSize;

	UPROPERTY(BlueprintReadWrite, Category = "Arena | Floor")
		FVector FloorMeshScale;

	UPROPERTY(BlueprintReadWrite, Category = "Arena | Wall")
		FVector WallMeshSize;

	UPROPERTY(BlueprintReadWrite, Category = "Arena | Wall")
		FVector WallMeshScale;

	UPROPERTY(BlueprintReadWrite, Category = "Arena | Roof")
		FVector RoofMeshSize;

	UPROPERTY(BlueprintReadWrite, Category = "Arena | Roof")
		FVector RoofMeshScale;



#pragma endregion

	

private:
	
	bool bBOR_Floor = true;

	FVector ArenaCenterLoc;
	FVector FloorOriginOffset;
	FVector WallOriginOffset;
	FVector RoofOriginOffset;

	//Floors
	bool bFloorRotates = false;
	bool bMoveFloorWhenRotated = false;
	bool bWarpFloorPlacement = false;
	bool bWarpFloorScale = false;


	//Walls
	bool bWarpWallPlacement = false;
	bool bAddWallRotation = false;

	//Roofs
	bool bBringRoofForward = false;
	bool bRoofIncrementsForwardEachLevel = false;
	bool bRoofShouldRotate = false;
	bool bMoveRoofWhenRotated = false;
};
