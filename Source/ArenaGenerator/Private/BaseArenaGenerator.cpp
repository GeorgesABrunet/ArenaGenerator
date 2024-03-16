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
#include "Components/InstancedStaticMeshComponent.h"
#include <Kismet/KismetMathLibrary.h>


DEFINE_LOG_CATEGORY(LogArenaGenerator)

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
	
	ArenaStream = FRandomStream(ArenaSeed);

}

// Called when the game starts or when spawned
void ABaseArenaGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	
}

// Called every frame
void ABaseArenaGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseArenaGenerator::GenerateArena()
{
	//Clear previous arena
	WipeArena();

	//TODO - branch building method for pattern list
	if (bBuildArenaUsingPatternList) {

		UE_LOG(LogArenaGenerator, Log, TEXT("Generating Arena from Patterns..."));
		//TODO - BUILD ITERATIVELY ON PATTERN INPUT

		// for pattern : level patterns
			//buildlevel(pattern)
	}
	else{

		UE_LOG(LogArenaGenerator, Log, TEXT("Generating Arena as Three Piece..."));
		//Calculate necessary values for generation based on the parameters provided and build order rules
		CalculateArenaParameters(ArenaBuildOrderRules);

		//BUILD 1 to 3 FLOOR ARENA

		//Build Floor as horizontal grid
		if (!FloorMeshes.IsEmpty() && ArenaBuildRules.bBuildFloor) {
			BuildFloor();
		}

		//Build Walls
		if (!WallMeshes.IsEmpty() && ArenaBuildRules.bBuildWalls && SideTileHeight > 0) {
			BuildWalls();
		}

		//Build Roof
		if (!RoofMeshes.IsEmpty() && ArenaBuildRules.bBuildRoof) {
			BuildRoof();
		}
	}

}

void ABaseArenaGenerator::WipeArena()
{
	UE_LOG(LogArenaGenerator, Log, TEXT("Wiping Arena..."));

	for (UInstancedStaticMeshComponent* Inst : FloorMeshInstances) {
		Inst->DestroyComponent();
	}
	FloorMeshInstances.Empty();

	for (UInstancedStaticMeshComponent* Inst : WallMeshInstances) {
		Inst->DestroyComponent();
	}
	WallMeshInstances.Empty();

	for (UInstancedStaticMeshComponent* Inst : RoofMeshInstances) {
		Inst->DestroyComponent();
	}
	RoofMeshInstances.Empty();
}

void ABaseArenaGenerator::ParametrizeGeneration()
{
	//TODO - allow parametrization from data table
}

void ABaseArenaGenerator::CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules)
{
	ArenaBuildOrderRules = BuildOrderRules;

	//floor rules
	bFloorRotates = FloorPlacementRules.bFloorRotates;
	bMoveFloorWhenRotated = FloorPlacementRules.bMoveFloorWhenRotated;
	bWarpFloorPlacement = FloorPlacementRules.bWarpFloorPlacement;
	bWarpFloorScale = FloorPlacementRules.bWarpFloorScale;

	//wall rules
	bWarpWallPlacement = WallPlacementRules.bWarpWallPlacement;
	bAddWallRotation = WallPlacementRules.bAddWallRotation;

	//roof rules
	bBuildRoofAsCone = RoofPlacementRules.bBuildRoofAsCone;
	bBringRoofForward = RoofPlacementRules.bBringRoofForward;
	bRoofIncrementsForwardEachLevel = RoofPlacementRules.bRoofIncrementsForwardEachLevel;
	bRoofRotates = RoofPlacementRules.bRoofShouldRotate;
	bMoveRoofWhenRotated = RoofPlacementRules.bMoveRoofWhenRotated;
	bFlipRoofMeshes = RoofPlacementRules.bFlipRoofMeshes;
	bWarpRoofPlacement = RoofPlacementRules.bWarpRoofPlacement;


	switch (BuildOrderRules) {
		case EArenaBuildOrderRules::FloorLeadsByDimensions:
			//ArenaDimensions determines Inscribed Radius

			//Floors
			bBOR_Floor = true;
			ArenaDimensions = DesiredArenaFloorDimensions;
			InscribedRadius = (FloorMeshSize.X * (DesiredArenaFloorDimensions - 1) * 0.5);

			//Walls
			ArenaSides = FMath::Clamp(DesiredArenaSides, 3, MaxSides);
			InteriorAngle = ((ArenaSides - 2) * 180) / ArenaSides;
			ExteriorAngle = 360.f / ArenaSides;

			SideLength = 2.f * CalculateOpposite(InscribedRadius, InteriorAngle/2.f);
				//abs((ArenaSides > 4 ? InscribedRadius/2 : InscribedRadius) * FMath::Cos(InteriorAngle / 2));
			TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength/WallMeshSize.X), 
				1, //Min
				MaxTilesPerSideRow //Max
			);
				
			Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));
			break;
		case EArenaBuildOrderRules::FloorLeadsByRadius:
			//TODO - InscribedRadius determines arenadims w mesh size

			//Floors
			bBOR_Floor = true;
			InscribedRadius = DesiredInscribedRadius;
			ArenaDimensions = (InscribedRadius / FloorMeshSize.X) > 2 ? FMath::Floor(InscribedRadius / FloorMeshSize.X) : 2;

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
	
	FloorOriginOffset = (FloorMeshSize * ArenaDimensions * -0.5f) * FVector(1, 1, 0);
	
	ArenaCenterLoc = -(
		((ForwardVectorFromYaw(InteriorAngle / 2) * InscribedRadius) * FVector((static_cast<float>(TilesPerArenaSide) / (SideLength / WallMeshSize.X)))) -
		FVector(0, (FloorMeshSize * ArenaDimensions * 0.5f).Y, 0));

	WallOriginOffset = FVector(ArenaCenterLoc.X, ArenaCenterLoc.Y + FloorOriginOffset.Y, FloorOriginOffset.Z);

	RoofOriginOffset = bBuildRoofAsCone ? FVector(WallOriginOffset.X, WallOriginOffset.Y, WallMeshSize.Z * SideTileHeight)
		: FVector(FloorOriginOffset.X, FloorOriginOffset.Y, WallMeshSize.Z * SideTileHeight);

}

void ABaseArenaGenerator::BuildFloor()
{
	UE_LOG(LogArenaGenerator, Log, TEXT("Building Floor..."));

	//Cache midpoint index
	int FloorMidpoint = FMath::Floor(ArenaDimensions / 2);

	for (UStaticMesh* Mesh : FloorMeshes) {
		UInstancedStaticMeshComponent* InstancedMesh =
			NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

		InstancedMesh->SetStaticMesh(Mesh);
		if (FloorMaterial) {
			InstancedMesh->SetMaterial(0, FloorMaterial);
		}
		InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		InstancedMesh->RegisterComponent();
		
		FloorMeshInstances.Add(InstancedMesh);
	}


	for (int Row = 0; Row < ArenaDimensions; Row++) {
		for (int Col = 0; Col < ArenaDimensions; Col++) {
			//Cache some random value between 0 & 3 to use for rotating floor pieces
			int RandomVal = bFloorRotates ? ArenaStream.RandRange(0, 3) : 0;

			//Determine floor tile transform
			FTransform FloorTileTransform = FTransform(
				FRotator(0.f, 90.f * RandomVal, 0.f), //Rotation
				FloorOriginOffset // Location : Floor Origin Offset
				+ FVector(FloorMeshSize.X * Row * FloorMeshScale.X, FloorMeshSize.Y * Col * FloorMeshScale.Y, 0)//Location : MeshSize * indices
				+ (bWarpFloorPlacement ? PlacementWarping(FloorMidpoint, FloorMidpoint, Col, Row, FloorWarpRange, FloorWarpConcavityStrength, FVector(0,0,1)) : FVector(0)) //TODO floor placement warping 
				+ (bMoveFloorWhenRotated ? RotatedMeshOffset() : FVector(0)) // Move floor if rotated
				,FVector(FloorMeshScale.X, FloorMeshScale.Y, FloorMeshScale.Z + (bWarpFloorScale ? ArenaStream.FRandRange(0, 0.25f ) : 0.f)) //Scale
			);
			
			
			int FloorMeshIdx = 0; //TODO - floor mesh selection patterns

			if (FloorMeshInstances[FloorMeshIdx]) {
				FloorMeshInstances[FloorMeshIdx]->AddInstance(FloorTileTransform);
			}
			else {
				UE_LOG(LogArenaGenerator, Error, TEXT("Could not find Floor Mesh Instance index: %d"), FloorMeshIdx);
			}
		}
	}
}

void ABaseArenaGenerator::BuildWalls()
{
	UE_LOG(LogArenaGenerator, Log, TEXT("Building Wall..."));

	//cache midpoints
	int ColMidpoint = FMath::Clamp(SideTileHeight / 2, 1, SideTileHeight);
	int RowMidpoint = FMath::Clamp(TilesPerArenaSide / 2, 1, TilesPerArenaSide);

	for (UStaticMesh* Mesh : WallMeshes) {
		UInstancedStaticMeshComponent* InstancedMesh =
			NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

		InstancedMesh->SetStaticMesh(Mesh);
		//TODO - Set material
		if (WallMaterial) {
			InstancedMesh->SetMaterial(0, WallMaterial);
		}
		InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		InstancedMesh->RegisterComponent();

		WallMeshInstances.Add(InstancedMesh);
	}

	FVector LastCachedPosition{ 0 };
	FVector SideAngleFV{ 0 };

	for (int SideIdx = 0; SideIdx < ArenaSides; SideIdx++) {

		//Get Yaw Rotation for wall pieces
		float YawRotation = (360.f / ArenaSides) * SideIdx;

		//RightVector
		FVector SideAngleRV = FRotationMatrix(FRotator(0, YawRotation, 0)).GetScaledAxis(EAxis::Y);

		//Adjust last cached position to offset by meshsize in side angle's direction
		LastCachedPosition = (SideAngleFV * WallMeshSize.X) + LastCachedPosition;
		
		//Get Side angle's forward vector
		SideAngleFV = ForwardVectorFromYaw(ExteriorAngle * SideIdx);
		
		//Orient Wall Warp Range by wall rotation.
		FVector WallOffsetRanges = (FRotator(0, YawRotation, 0).RotateVector(WallWarpRange)).GetAbs();
			//(WallWarpRange.RotateAngleAxis(YawRotation, FVector(0, 0, 1)));


		for (int Len = 0; Len < TilesPerArenaSide; ++Len) {
			//Iterate LastCachedPosition by mesh size in new side angle's direction. Don't include placement jitter here.
			LastCachedPosition = (SideAngleFV * FVector(WallMeshSize.X, WallMeshSize.X, 0)) * (Len > 0 ? 1 : 0) + LastCachedPosition;

			for (int Height = 0; Height < SideTileHeight; ++Height) {

				FTransform WallTileTransform = FTransform(
				FRotator(0, YawRotation + (bAddWallRotation ? ArenaStream.FRandRange(0.f, 360.f) : 0.f), 0), // Rotation
					WallOriginOffset + LastCachedPosition + FVector(0, 0, WallMeshSize.Z * Height) //Location : Origin offset & tiling
					+ (bWarpWallPlacement ? PlacementWarping(ColMidpoint, RowMidpoint, Len, Height, WallOffsetRanges, WallWarpConcavityStrength, SideAngleRV) : FVector(0)) //LocationWarping
				,WallMeshScale);

				//TODO pattern matching for wall meshes
				int WallMeshIdx = 0;

				//All good, Add instance
				if (WallMeshInstances[WallMeshIdx]) {
					WallMeshInstances[WallMeshIdx]->AddInstance(WallTileTransform);
				}
				else {
					UE_LOG(LogArenaGenerator, Error, TEXT("Could not find Wall Mesh Instance index: %d"), WallMeshIdx);
				}
				
			}
		}
	}

}

void ABaseArenaGenerator::BuildRoof()
{
	UE_LOG(LogArenaGenerator, Log, TEXT("Building Roof..."));

	for (UStaticMesh* Mesh : RoofMeshes) {
		UInstancedStaticMeshComponent* InstancedMesh =
			NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

		InstancedMesh->SetStaticMesh(Mesh);
		//TODO - Set material
		if (RoofMaterial) {
			InstancedMesh->SetMaterial(0, RoofMaterial);
		}
		InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		InstancedMesh->RegisterComponent();

		RoofMeshInstances.Add(InstancedMesh);
	}

	if (bBuildRoofAsCone) {

		//Get difference of wall and roof mesh scales to offset pieces correctly
		float MeshScalar = WallMeshSize.X / RoofMeshSize.X;

		FVector LastCachedPosition{ 0 };
		FVector RoofSideAngleFV{ 0 };

		for (int RoofSideIdx = 0; RoofSideIdx < ArenaSides; RoofSideIdx++) {
			
			//Cache last used position to update through next loop
			LastCachedPosition = LastCachedPosition + ((RoofSideAngleFV * RoofMeshSize.X) * MeshScalar);
			
			//Determine forward vector for placement
			RoofSideAngleFV = ForwardVectorFromYaw(ExteriorAngle * RoofSideIdx);

			//Cache Yaw rotation for the side
			float RoofYawRotation = (360.f / ArenaSides) * RoofSideIdx;

			//Determine right vector for placement offsets
			FVector RoofSideAngleRV = FRotationMatrix(FRotator(0, RoofYawRotation, 0)).GetScaledAxis(EAxis::Y);
				
			FVector RoofOffsetRanges = RoofWarpRange.RotateAngleAxis(RoofYawRotation, FVector(0, 0, 1)).GetAbs();

			for (int RoofLen = 0; RoofLen < TilesPerArenaSide; ++RoofLen) {

				LastCachedPosition = (RoofSideAngleFV * FVector(RoofMeshSize.X, RoofMeshSize.X, 0)) * (RoofLen > 0 ? 1 : 0) + LastCachedPosition;

				for (int RoofHeightIdx = 0; RoofHeightIdx < RoofTileHeight; ++RoofHeightIdx) {

					FTransform RoofTileTransform = FTransform(
						FRotator(0.f, RoofYawRotation + (bMoveRoofWhenRotated ? ArenaStream.FRandRange(0.f, 360.f) : 0.f), 0.f),
						LastCachedPosition //LastCachedPosition
						+ RoofOriginOffset //Offset and position
						+ FVector(0, 0, RoofMeshSize.Z * RoofHeightIdx) // Height Adjustment
						+ (bBringRoofForward ? RoofSideAngleRV * WallMeshSize.Y : FVector(0)) 
						+ (bRoofIncrementsForwardEachLevel ? (RoofSideAngleRV * RoofHeightIdx * RoofMeshSize.Y) : FVector(0))
					
					,RoofMeshScale);

					int RoofMeshIdx = 0;

					if (RoofMeshInstances[RoofMeshIdx]) {
						RoofMeshInstances[RoofMeshIdx]->AddInstance(RoofTileTransform);
					}
					else {
						UE_LOG(LogArenaGenerator, Error, TEXT("Could not find Roof Mesh Instance index: %d"), RoofMeshIdx);
					}
					
				}
			}
		}
	}
	else {

		//Cache midpoint index
		int RoofMidpoint = FMath::Floor(ArenaDimensions / 2);

		for (int Row = 0; Row < ArenaDimensions; Row++) {
			for (int Col = 0; Col < ArenaDimensions; Col++) {

				//Cache some random value between 0 & 3 to use for rotating roof pieces
				int RandomVal = bRoofRotates ? ArenaStream.RandRange(0, 3) : 0;

				FTransform RoofTileTransform = FTransform(
					FRotator((bFlipRoofMeshes ? 180.f : 0), 90.f * RandomVal, 0), //Rotation
					RoofOriginOffset + FVector(RoofMeshSize.X * Row * RoofMeshScale.X, RoofMeshSize.Y * Col * RoofMeshScale.Y, 0)
					+ (bRoofRotates && bMoveRoofWhenRotated ? 0 : 0) //TODO - location offsets from rotation, considering origin location on mesh
					+ (bWarpRoofPlacement ? PlacementWarping(RoofMidpoint, RoofMidpoint, Col, Row, RoofWarpRange, RoofWarpConcavityStrength, FVector(0, 0, 1)) : FVector(0))
					, RoofMeshScale
				);

				//TODO - roof pattern idx
				int RoofMeshIdx = 0;

				if (RoofMeshInstances[RoofMeshIdx]) {
					RoofMeshInstances[RoofMeshIdx]->AddInstance(RoofTileTransform);
				}
				else {
					UE_LOG(LogArenaGenerator, Error, TEXT("Could not find Roof Mesh Instance index: %d"), RoofMeshIdx);
				}
			}
		}

	}
}



#pragma region Utility

float ABaseArenaGenerator::CalculateOpposite(float length, float angle)
{
	return (length * FMath::Cos(FMath::DegreesToRadians(angle)));
}

float ABaseArenaGenerator::CalculateAdjacent(float length, float angle)
{
	return (length * FMath::Sin(FMath::DegreesToRadians(angle)));
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
	return FVector(0, 0, 0);
}

FVector ABaseArenaGenerator::PlacementWarping(int ColMidpoint, int RowMidpoint, int Col, int Row, FVector OffsetRanges, float ConcavityStrength, FVector WarpDirection)
{
	float ConcaveWarp = ConcavityStrength *
		(FMath::Clamp((FMath::Lerp(0.f, 1.f, FMath::Clamp((static_cast<float>(abs(Col - ColMidpoint)) / RowMidpoint), 0.f, 1.f)) *
		FMath::Lerp(0.f, 1.f, FMath::Clamp((static_cast<float>(abs(Row - RowMidpoint)) / RowMidpoint), 0.f, 1.f))), 0.f, 1.f));

	return  FVector(ArenaStream.FRandRange(OffsetRanges.X * -1, OffsetRanges.X), 
		ArenaStream.FRandRange(OffsetRanges.Y * -1, OffsetRanges.Y),
		ArenaStream.FRandRange(OffsetRanges.Z * -1, OffsetRanges.Z)) 
		+ (WarpDirection * ConcaveWarp);
}



#pragma endregion

