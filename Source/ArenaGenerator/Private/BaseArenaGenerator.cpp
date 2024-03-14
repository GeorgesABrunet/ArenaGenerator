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


#include "BaseArenaGenerator.h"
#include "Math/RandomStream.h"

// Sets default values
ABaseArenaGenerator::ABaseArenaGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//WipeArena();

	ArenaBuildOrderRules = EArenaBuildOrderRules::FloorLeadsByDimensions;
	bBuildArenaCenterOnActor = true;
	DesiredArenaSides = 8;
	DesiredArenaFloorDimensions = 10;
	DesiredInscribedRadius = 2000;
	DesiredTilesPerSide = 5;
	SideTileHeight = 1;

	FloorMeshSize = FVector{ 500.f, 500.f, 50.f };
	WallMeshSize = FVector{ 500.f, 50.f, 500.f };
	RoofMeshSize = FVector{ 500.f, 500.f, 500.f };

	FloorMeshScale = FVector{ 1 };
	WallMeshScale = FVector{ 1 };
	RoofMeshScale = FVector{ 1 };

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	//
}

// Called when the game starts or when spawned
void ABaseArenaGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	ArenaStream = FRandomStream(ArenaSeed);
}

// Called every frame
void ABaseArenaGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseArenaGenerator::GenerateArena()
{
}

void ABaseArenaGenerator::ParametrizeGeneration()
{
}

void ABaseArenaGenerator::CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules)
{
	ArenaBuildOrderRules = BuildOrderRules;

	switch (ArenaBuildOrderRules) {
		case EArenaBuildOrderRules::FloorLeadsByDimensions:
			//TODO - ArenaDimensions determines Inscribed Radius

			//Floors
			bBOR_Floor = true;
			ArenaDimensions = DesiredArenaFloorDimensions;
			InscribedRadius = (FloorMeshSize.X * (DesiredArenaFloorDimensions - 1));

			//Walls
			InteriorAngle = ((ArenaSides - 2) * 180) / ArenaSides;
			ExteriorAngle = 360.f / ArenaSides;
			SideLength = 2 * CalculateRightTriangleOpposite(InscribedRadius, InteriorAngle / 2);
			TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength/WallMeshSize.X), 
				1, //Min
				FMath::RoundToInt(SideLength / WallMeshSize.X) //Max
			);
				
			Apothem = CalculateRightTriangleAdjacent(InscribedRadius, InteriorAngle / 2);
			break;
		case EArenaBuildOrderRules::FloorLeadsByRadius:
			//TODO - Inscribedradius determines arenadims w mesh size

			//Floors
			bBOR_Floor = true;
			InscribedRadius = DesiredInscribedRadius;
			ArenaDimensions = ((InscribedRadius / FloorMeshSize.X) - 1) > 2 ? ((InscribedRadius / FloorMeshSize.X) - 1) : 2;

			//Walls

			break;
		case EArenaBuildOrderRules::WallsLeadByDimensions:
			//TODO - find inscribed radius from wall mesh size, desired tps, sides 

			bBOR_Floor = false;
			break;
		case EArenaBuildOrderRules::WallsLeadByRadius:
			//TODO - Inscribedradius determines final amount of tiles per side 

			bBOR_Floor = false;
			break;

		default:
			break;
	}

	//How much we need to offset the arena pieces as 
	ArenaCenterLoc = GetActorLocation() - (bBuildArenaCenterOnActor ? 
		(FloorMeshSize.Y * ArenaDimensions * -0.5f) + 
			(InscribedRadius * (TilesPerArenaSide/(SideLength/WallMeshSize.X)) * ForwardVectorFromYaw(InteriorAngle/2).X) : 0);
	
	FloorOriginOffset = (FloorMeshSize * ArenaDimensions * -0.5f) * FVector(1, 1, 0);

	WallOriginOffset = FVector(ArenaCenterLoc.X, ArenaCenterLoc.Y + FloorOriginOffset.Y, FloorOriginOffset.Z);

	//TODO determine roof origin offset for patterns
	RoofOriginOffset = FVector(0, 0, WallMeshSize.Z * SideTileHeight) //Height config
		+ WallOriginOffset;

}

void ABaseArenaGenerator::BuildFloor()
{

	for (int Row = 0; Row < ArenaDimensions - 1; Row++) {
		for (int Col = 0; Col < ArenaDimensions - 1; Col++) {
			int RandomVal = bFloorRotates ? ArenaStream.RandRange(0, 3) : 0;

			//Determine floor tile transform
			FTransform FloorTileTransform = FTransform(
				FRotator(0.f, 90.f * RandomVal, 0.f), //Rotation
				FloorOriginOffset // Location - Floor Origin Offset
				+ FVector(FloorMeshSize.X * Row * FloorMeshScale.X, FloorMeshSize.Y * Col * FloorMeshScale.Y, //Location : MeshSize * indices
					bWarpFloorPlacement ? 0.f : 0.f) //TODO floor placement warping 
				+ (bMoveFloorWhenRotated ? RotatedMeshOffset() : FVector(0)) // Move floor if rotated
				
				,FVector(FloorMeshScale.X, FloorMeshScale.Y, FloorMeshScale.Z + (bWarpFloorScale ? ArenaStream.FRandRange(0, 0.25f ) : 0.f)) //Scale
			);

		}
	}
}

void ABaseArenaGenerator::BuildWalls()
{
}



#pragma region Utility

float ABaseArenaGenerator::CalculateRightTriangleOpposite(float length, float angle)
{
	return (length * cos(angle));
}

float ABaseArenaGenerator::CalculateRightTriangleAdjacent(float length, float angle)
{
	return (length * sin(angle));
}

FVector ABaseArenaGenerator::ForwardVectorFromYaw(float yaw)
{
	float YawRad = FMath::DegreesToRadians(yaw);
	
	return FVector(FMath::Cos(YawRad),
		FMath::Sin(YawRad),
	0.f);
}

FVector ABaseArenaGenerator::RotatedMeshOffset()
{
	//TODO change based on origin location
	return FVector();
}

#pragma endregion
