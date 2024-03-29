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
#include "ArenaGeneratorLog.h"

// Sets default values
ABaseArenaGenerator::ABaseArenaGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; //Does not need to tick

	//WipeArena();

	ArenaBuildOrderRules = EArenaBuildOrderRules::PolygonLeadByRadius;
	ArenaPlacementOnActor = EOriginPlacementType::Center;
	DesiredArenaSides = 8;
	DesiredArenaFloorDimensions = 10;
	DesiredInscribedRadius = 2000;
	DesiredTilesPerSide = 5;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	
	ArenaSeed = 1010101;
	ArenaStream = FRandomStream(ArenaSeed);
	TotalInstances = 0;

	FocusGridIndex = 0;
	FocusPolygonIndex = 0;

}

// Called when the game starts or when spawned
void ABaseArenaGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	
}

void ABaseArenaGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	WipeArena(); //Need to handle components
}

void ABaseArenaGenerator::GenerateArena()
{
	//Clear previous arena
	WipeArena();


	ArenaGenLog_Info("Generating Arena...");

	//Calculate necessary values for generation based on the parameters provided and build order rules
	CalculateArenaParameters(ArenaBuildOrderRules);

	//Build patterns from PatternList
	BuildPatterns();
	//ArenaGenLog_Info("Total Instances: %d", TotalInstances);

}

void ABaseArenaGenerator::WipeArena()
{
	ArenaGenLog_Info("Wiping Arena...");
	
	//Clear list of used indices
	if (!UsedGroupIndices.IsEmpty()) {
		UsedGroupIndices.Empty();
	}

	//Clear list of reroute indices

	
	//Iterate through mesh instances and destroy before dereferencing
	for (auto& Inst : MeshInstances)
	{
		for (auto& Component : Inst) {
			if (Component) {
				Component->DestroyComponent();
			}
		}
		
	}
	MeshInstances.Empty();
	TotalInstances = 0;
	
	//Reset parameters for calculations
	OriginOffset = FVector(0);
	FocusGridIndex = 0;
	FocusPolygonIndex = 0;

	
}

void ABaseArenaGenerator::CalculateArenaParameters(EArenaBuildOrderRules BuildOrderRules)
{
	ArenaBuildOrderRules = BuildOrderRules;

	//For all cases, we need these.
	ArenaSides = FMath::Clamp(DesiredArenaSides, 3, MaxSides);
	InteriorAngle = ((ArenaSides - 2) * 180) / ArenaSides;
	ExteriorAngle = 360.f / ArenaSides;

	//Calculate remaining offsets

	//FloorOriginOffset = (((FloorMeshSize * (ArenaDimensions - 1) * -0.5f) * FloorMeshScale) + (FloorMeshSize * MeshOriginOffsetScalar(FloorOriginType))
	//	- (bMoveFloorWhenRotated ? RotatedMeshOffset(FloorOriginType, FloorMeshSize, 0) : FVector(0))) * FVector(1, 1, 0); //Z should be zero'd for floor.
	
	//ArenaCenterLoc = -(
	//	((ForwardVectorFromYaw(InteriorAngle / 2) * InscribedRadius) * FVector((static_cast<float>(TilesPerArenaSide) / (SideLength / WallMeshSize.X)))) -
	//	FVector(0, (FloorMeshSize * (ArenaDimensions-1) * 0.5f).Y - (FloorMeshSize * MeshOriginOffsetScalar(FloorOriginType)).Y, 0));

	//WallOriginOffset = bBOR_Floor ? FVector(ArenaCenterLoc.X, -ArenaCenterLoc.Y + FloorOriginOffset.Y, FloorOriginOffset.Z + FloorMeshSize.Z) //If inheriting offset from horizontal grid BOR
	//	: FVector(-(SideLength / 2), -Apothem, FloorOriginOffset.Z + FloorMeshSize.Z);//If inheriting offset from polygonal BOR

	//RoofOriginOffset = bBuildRoofAsCone ? FVector(WallOriginOffset.X, WallOriginOffset.Y, WallMeshSize.Z * SideTileHeight + WallOriginOffset.Z)
	//	: FVector(FloorOriginOffset.X, FloorOriginOffset.Y, WallMeshSize.Z * SideTileHeight + WallOriginOffset.Z);


	//Determine best starting indices for patterns
	bool bGrided = false; 
	bool bPolygoned = false;
	for (int i = 0; i < PatternList.Num(); i++) {
		//ArenaGenLog_Warning("FOUND PATTERNLIST index: %d.", i);
		if (bGrided && bPolygoned) { break; }
		if (PatternList[i].SectionType == EArenaSectionType::HorizontalGrid && !bGrided) { FocusGridIndex = PatternList[i].MeshGroupId; bGrided = true; ArenaGenLog_Info("Found grid request at index: %d.", i); }
		if (PatternList[i].SectionType == EArenaSectionType::Polygon && !bPolygoned) { FocusPolygonIndex = PatternList[i].MeshGroupId; bPolygoned = true; ArenaGenLog_Info("Found polygon request at index: %d.", i);}
	}

	//CALCULATE SECTION PARAMETERS. Takes index 0 & 1 based on if leading by floor, or by walls.
	switch (BuildOrderRules) {
		case EArenaBuildOrderRules::GridLeadsByDimensions:
		{
			//ArenaDimensions determines Inscribed Radius
			//Floors
			ArenaDimensions = DesiredArenaFloorDimensions;
			InscribedRadius = (MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X * (DesiredArenaFloorDimensions - 1) * 0.5);

			//Walls
			SideLength = 2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f);
			TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength / MeshGroups[PatternList[FocusPolygonIndex].MeshGroupId].MeshDimensions.X),
				1, //Min
				MaxTilesPerSideRow //Max
			);
			Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

		}break;
		case EArenaBuildOrderRules::GridLeadsByRadius:
		{
			//InscribedRadius determines arenadims w mesh size
			//Floors
			ArenaDimensions = (DesiredInscribedRadius / MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X) > 2 ? FMath::Floor(DesiredInscribedRadius / MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X) : 2;
			InscribedRadius = (MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X * (ArenaDimensions) * 0.5);
			//Walls
			SideLength = 2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f);
			TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength / MeshGroups[PatternList[FocusPolygonIndex].MeshGroupId].MeshDimensions.X), //TODO - better input method for indexing here
				1, //Min
				MaxTilesPerSideRow //Max
			);
			Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

		}break;
		case EArenaBuildOrderRules::PolygonLeadByDimensions:
		{
			//find inscribed radius from mesh size, desired tps, arena sides.

			TilesPerArenaSide = DesiredTilesPerSide;
			SideLength = MeshGroups[PatternList[FocusPolygonIndex].MeshGroupId].MeshDimensions.X * TilesPerArenaSide;

			InscribedRadius = (SideLength / 2.f) / FMath::Sin(FMath::DegreesToRadians(90.f - (InteriorAngle / 2))); //Hypotenuse = opposite divided by sine of adjacent angle 
			Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

			ArenaDimensions = FMath::CeilToInt((InscribedRadius * 2.f) / MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X);
		}break;
		case EArenaBuildOrderRules::PolygonLeadByRadius:
		{
			//Inscribedradius determines final amount of tiles per side 

			TilesPerArenaSide = FMath::Floor((2.f * CalculateOpposite(DesiredInscribedRadius, InteriorAngle / 2.f)) / MeshGroups[PatternList[FocusPolygonIndex].MeshGroupId].MeshDimensions.X);
			SideLength = MeshGroups[PatternList[FocusPolygonIndex].MeshGroupId].MeshDimensions.X * TilesPerArenaSide;

			InscribedRadius = (SideLength / 2.f) / FMath::Sin(FMath::DegreesToRadians(90.f - (InteriorAngle / 2))); //Hypotenuse = opposite/2 divided by sine of adjacent angle
			Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

			ArenaDimensions = FMath::CeilToInt((InscribedRadius * 2.f) / MeshGroups[PatternList[FocusGridIndex].MeshGroupId].MeshDimensions.X);
		}break;
	}

}

void ABaseArenaGenerator::BuildPatterns()
{
	if (PatternList.IsEmpty())
	{
		ArenaGenLog_Warning("PatternList is empty!");
	}
	else {
		ArenaGenLog_Info("PatternList size: %d", PatternList.Num());

		for (auto& pattern : PatternList) {
				BuildSection(pattern);
		}
	}
}

void ABaseArenaGenerator::BuildSection(FArenaSectionBuildRules& Section)
{
	ArenaGenLog_Info("Building Section...");

	//Determine arena parameters based off of current origin offsets etc.
	int GroupIdx = Section.MeshGroupId < MeshGroups.Num() ? Section.MeshGroupId : 0;
	int ReRouteIdx = 0;
	FVector MeshSize = MeshGroups[GroupIdx].MeshDimensions;
	FVector MeshScale = MeshGroups[GroupIdx].MeshScale;	
	float MeshScalar = PreviousMeshSize.X != 0.f ? MeshSize.X / PreviousMeshSize.X : 1.f;

	int CurrTilesPerSide = MeshScalar == 1.f ? TilesPerArenaSide : //Tiles per arena Side
		FMath::Clamp(FMath::Floor((2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f)) / MeshSize.X), 1, MaxTilesPerSideRow);

	if (PreviousTilesPerSide == 0) { PreviousTilesPerSide = TilesPerArenaSide; } // Prev Tiles cannot be zero

	//Update Origin Offset based on Arena placement on actor option and previous parameters
	switch (ArenaPlacementOnActor) {
	default:
		ArenaGenLog_Info("Hit default case on ArenaPlacement! Placing in center.");
		case EOriginPlacementType::Center:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(
					(MeshSize.X * (-0.5f * ArenaDimensions) * MeshScale.X),//X
					(MeshSize.Y * (-0.5f * ArenaDimensions) * MeshScale.Y),//Y
					OriginOffset.Z);
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {
				FVector PolygonOffset = (
					(ForwardVectorFromYaw(InteriorAngle/2) * InscribedRadius) * FVector(static_cast<float>(CurrTilesPerSide) / (SideLength / MeshSize.X))
					);

				OriginOffset = FVector(-PolygonOffset.X, -PolygonOffset.Y, OriginOffset.Z); //for Grid BOR
				// FVector(-(SideLength/2), -Apothem, OriginOffset.Z);
			}
		}
		break;
	}

	//Update rotation parameters
	float RotationIncr = 360.f / Section.YawPossibilities;
	int YawPosMax = Section.YawPossibilities-1;
	
	//Mesh Instancing, get mesh group
	if (!UsedGroupIndices.Contains(GroupIdx))
	{
		UsedGroupIndices.Add(GroupIdx);

		TArray<UInstancedStaticMeshComponent*> ToInstance;

		for (FArenaMesh& ArenaMesh : MeshGroups[GroupIdx].GroupMeshes)
		{
			UInstancedStaticMeshComponent* InstancedMesh =
				NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

			InstancedMesh->SetStaticMesh(ArenaMesh.Mesh);
			InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			InstancedMesh->RegisterComponent();

			ToInstance.Add(InstancedMesh);
		}

		//add tarray of instances to mesh instances
		//TODO reroute array for mesh instance group indices
		MeshInstances.Add(ToInstance);
		ReRouteIdx = UsedGroupIndices.Find(GroupIdx);
		//MeshInstances.EmplaceAt(GroupIdx, ToInstance);
		ArenaGenLog_Info("Adding the Mesh Group %d to Mesh Instances at index: %d ", GroupIdx, ReRouteIdx);
	}
	else 
	{
		ArenaGenLog_Info("Index: %d is already instanced, ignoring request", GroupIdx);
	}
	
	//Build Section
	switch (Section.SectionType) {
		case EArenaSectionType::Polygon:
		{
			FVector LastCachedPosition{ 0 };
			FVector SideAngleFV{ 0 };

			for (int SideIdx = 0; SideIdx < ArenaSides; ++SideIdx) 
			{
				//Cache last used position to update through next loop
				LastCachedPosition = LastCachedPosition + ((SideAngleFV * MeshSize.X) * MeshScalar);

				//Determine forward vector for placement
				SideAngleFV = ForwardVectorFromYaw(ExteriorAngle * SideIdx);

				//Cache Yaw rotation for the side
				float YawRotation = (360.f / ArenaSides) * SideIdx;

				//Determine right vector for placement offsets
				FVector SideAngleRV = FRotationMatrix(FRotator(0, YawRotation, 0)).GetScaledAxis(EAxis::Y);

				//TODO - orient local warp ranges by RV

				for (int LenIdx = 0; LenIdx < CurrTilesPerSide; ++LenIdx) 
				{
					LastCachedPosition = (SideAngleFV * MeshSize.X * (LenIdx > 0 ? 1 : 0)) + LastCachedPosition;

					for (int HeightIdx = 0; HeightIdx < Section.SectionAmount; ++HeightIdx)
					{
						//TODO - determine meshIdx
						int MeshIdx = 0;

						//Cache some random value between 0 & 3 to use for rotating roof pieces
						int RandomVal{ 0 };
						FVector RotationOffsetAdjustment;
						float HeightAdjustment = MeshSize.Z * Section.InitOffsetByHeightScalar;

						switch (Section.RotationRule) {
						case EPlacementOrientationRule::None:
							break;
						case EPlacementOrientationRule::RotateByYP:
							RandomVal = ArenaStream.RandRange(0, Section.YawPossibilities);
							RotationOffsetAdjustment =
								OffsetMeshAlongDirections(SideAngleFV, SideAngleRV, MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, RandomVal);
							break;
						case EPlacementOrientationRule::RotateYawRandomly:
							YawRotation = ArenaStream.FRandRange(0, 360.f);
							break;
						}

						FTransform TileTransform = FTransform(
						//ROTATION
							FRotator(
							Section.DefaultRotation.Pitch  //Pitch
							, Section.DefaultRotation.Yaw + YawRotation + (RotationIncr * RandomVal) //Yaw
							, Section.DefaultRotation.Roll), //Roll
						
						//LOCATION
							LastCachedPosition // Iterate on position...
							+ OriginOffset // Offset by the origin of our section 
							+ FVector(0, 0, (MeshSize.Z * (HeightIdx * Section.OffsetByHeightIncrement)) + HeightAdjustment) // Height Adjustment
							+ (SideAngleRV * MeshSize.Y * Section.InitOffsetByWidthScalar) // Initial width offset
							+ (SideAngleRV * MeshSize.Y * Section.OffsetByWidthIncrement * HeightIdx) // Offset by width each height increment
							+ RotationOffsetAdjustment // Adjust by offset caused by rotation and mesh origin type
							//TODO - Warping
							
						//SCALE
						, MeshScale);

						if (MeshInstances[ReRouteIdx][MeshIdx]) {
							MeshInstances[ReRouteIdx][MeshIdx]->AddInstance(TileTransform);
							TotalInstances++;
						}
						else {
							ArenaGenLog_Error("Could not find Mesh Instance of group: %d at index: %d", GroupIdx, MeshIdx);
						}

					}
				}
			}

			//Cache values before scope ends
			PreviousTilesPerSide = CurrTilesPerSide;
			PreviousLastPosition = LastCachedPosition;

			if (Section.bUpdatesOriginOffsetHeight) {
				OriginOffset = FVector(OriginOffset.X, OriginOffset.Y, OriginOffset.Z + (MeshSize.Z * Section.SectionAmount)); // Update OriginOffset by height of 
			}
		}
		break;

		case EArenaSectionType::HorizontalGrid:
		{
			//Cache midpoint index
			int Midpoint = FMath::Floor(ArenaDimensions / 2);

			int SectionDimensions = (MeshSize.X == PreviousMeshSize.X) ? ArenaDimensions :
				FMath::Floor((2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f)) / MeshSize.X);

			FVector PlacementFV = FVector(1.f, 0.f, 0.f); //ForwardVectorFromYaw(Section.DefaultRotation.Yaw);
			FVector PlacementRV = FVector(0.f, 1.f, 0.f);//FRotationMatrix(FRotator(0, Section.DefaultRotation.Yaw, 0)).GetScaledAxis(EAxis::Y);

			for (int TimesIdx = 0; TimesIdx < Section.SectionAmount; TimesIdx++) {
				for (int Row = 0; Row < SectionDimensions; Row++) {
					for (int Col = 0; Col < SectionDimensions; Col++) {

						//TODO - determine meshIdx
						int MeshIdx = 0;

						//Rotation values
						int RandomVal=0;
						FVector RotationOffsetAdjustment{ 0 };
						float YawRotation{ 0.f };

						switch (Section.RotationRule) {
							case EPlacementOrientationRule::RotateByYP:
							{
								RandomVal = ArenaStream.RandRange(0, YawPosMax);
							}break;

							case EPlacementOrientationRule::RotateYawRandomly:
							{
								YawRotation = ArenaStream.FRandRange(0, 360.f);
							}break;
						}
						
						RotationOffsetAdjustment = //OffsetMeshAlongDirections(PlacementFV, PlacementRV, MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, RandomVal);
									RotatedMeshOffset(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, RandomVal);

						//Transform
						FTransform TileTransform = FTransform(
							//ROTATION
							FRotator(
								Section.DefaultRotation.Pitch  //Pitch
								, YawRotation + (RotationIncr * RandomVal) //+ Section.DefaultRotation.Yaw //Yaw
								, Section.DefaultRotation.Roll //Roll
							),
							//LOCATION
							OriginOffset //Cached Origin offset
							+ FVector(0,0, MeshSize.Z * TimesIdx) //Height Offset for section amount
							+ (PlacementFV * MeshSize.X * MeshScale.X * Row) // Relative X placement
							+ (PlacementRV * MeshSize.Y * MeshScale.Y * Col) // Relative Y Placement
							+ RotationOffsetAdjustment //Offset from rotation by OriginType
							+ (Section.bWarpPlacement ? PlacementWarping(SectionDimensions / 2, SectionDimensions/2, Col, Row, Section.WarpRange, Section.WarpConcavityStrength, FVector(0, 0, 1)) : FVector(0)) //Warping along placement

							//SCALE
							, MeshScale
						);

						//Add instance
						if (MeshInstances[ReRouteIdx][MeshIdx]) {
							MeshInstances[ReRouteIdx][MeshIdx]->AddInstance(TileTransform);
							TotalInstances++;
						}
						else {
							ArenaGenLog_Error("Could not find Mesh Instance of group: %d at index: %d", GroupIdx, MeshIdx);
						}
					}
				}
			}

			if (Section.bUpdatesOriginOffsetHeight) {
				OriginOffset = FVector(OriginOffset.X, OriginOffset.Y, OriginOffset.Z + (MeshSize.Z * Section.SectionAmount)); // Update OriginOffset by height of 
			}
		}
		break;
	}

	//Cache values for next section
	PreviousMeshSize = MeshSize;
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

FVector ABaseArenaGenerator::RotatedMeshOffset(EOriginPlacementType OriginType, FVector& MeshSize, int RotationIndex)
{
	float X1 = 0;
	float Y1 = 0;

	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		if (RotationIndex == 1) {
			X1 = MeshSize.X;
		}
		else if (RotationIndex == 2) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::XY_Negative):
		if (RotationIndex == 0) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 1) {
			Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			X1 = MeshSize.X;
		}
		break;

	case(EOriginPlacementType::X_Positive_Y_Negative):
		if (RotationIndex == 0) {
			Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			X1 = MeshSize.X; 
		}
		else if (RotationIndex == 3) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::X_Negative_Y_Positive):
		if (RotationIndex == 0) {
			X1 = MeshSize.X;
		}
		else if (RotationIndex == 1) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::Center):
		return FVector(0);
		break;
	}

	return FVector(X1, Y1, 0);
}

FVector ABaseArenaGenerator::OffsetMeshAlongDirections(const FVector& FV, const FVector& RV, EOriginPlacementType OriginType, const FVector& MeshSize, int RotationIndex)
{
	FVector Xspan;
	FVector Yspan;

	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		if (RotationIndex == 1) {
			Xspan = FV * MeshSize.X;
		}
		else if (RotationIndex == 2) {
			Xspan = FV * MeshSize.X; Yspan = RV * MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Yspan = RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::XY_Negative):
		if (RotationIndex == 0) {
			Xspan = FV * MeshSize.X; Yspan = RV * MeshSize.Y;
		}
		else if (RotationIndex == 1) {
			Yspan = RV * MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Xspan = FV * MeshSize.X;
		}
		break;

	case(EOriginPlacementType::X_Positive_Y_Negative):
		if (RotationIndex == 0) {
			Yspan = RV * MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Xspan = FV * MeshSize.X;
		}
		else if (RotationIndex == 3) {
			Xspan = FV * MeshSize.X; Yspan = RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::X_Negative_Y_Positive):
		if (RotationIndex == 0) {
			Xspan = FV * MeshSize.X;
		}
		else if (RotationIndex == 1) {
			Xspan = FV * MeshSize.X; Yspan = RV * MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Yspan = RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::Center):
		return FVector(0);
		break;
	}
	return Xspan + Yspan;
}


FVector ABaseArenaGenerator::MeshOriginOffsetScalar(EOriginPlacementType OriginType)
{
	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		return FVector(-0.5, -0.5, 1);
		break;
	case(EOriginPlacementType::XY_Negative):
		return FVector(0.5, 0.5, 1);
		break;
	case(EOriginPlacementType::X_Positive_Y_Negative):
		return FVector(-0.5, 0.5, 1);
		break;
	case(EOriginPlacementType::X_Negative_Y_Positive):
		return FVector(0.5, -0.5, 1); 
		break;
	case(EOriginPlacementType::Center):
		return FVector(0, 0, 1);
		break;
	}

	return FVector(0);
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


/*
void ABaseArenaGenerator::BuildFloor()
{
	ArenaGenLog_Info("Building Floor...");
	//UE_LOG(LogArenaGenerator, Log, TEXT("Building Floor..."));

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
			float YawAngle = 90.f * RandomVal;

			//Determine floor tile transform
			FTransform FloorTileTransform = FTransform(
				FRotator(0.f, YawAngle, 0.f), //Rotation

				FloorOriginOffset // Location : Floor Origin Offset
				+ FVector(FloorMeshSize.X * Row * FloorMeshScale.X, FloorMeshSize.Y * Col * FloorMeshScale.Y, 0)//Location : MeshSize * indices
				+ (bWarpFloorPlacement ? PlacementWarping(FloorMidpoint, FloorMidpoint, Col, Row, FloorWarpRange, FloorWarpConcavityStrength, FVector(0,0,1)) : FVector(0)) //floor placement warping
				+ (bMoveFloorWhenRotated && bFloorRotates ? RotatedMeshOffset(FloorOriginType, FloorMeshSize, RandomVal) : FVector(0)) // Move floor if rotated

				,FVector(FloorMeshScale.X, FloorMeshScale.Y, FloorMeshScale.Z + (bWarpFloorScale ? ArenaStream.FRandRange(0, 0.25f ) : 0.f)) //Scale
			);


			int FloorMeshIdx = 0; //TODO - floor mesh selection patterns

			if (FloorMeshInstances[FloorMeshIdx]) {
				FloorMeshInstances[FloorMeshIdx]->AddInstance(FloorTileTransform);
			}
			else {
				ArenaGenLog_Error("Could not find Floor Mesh Instance index: %d", FloorMeshIdx);
			}
		}
	}
}

void ABaseArenaGenerator::BuildWalls()
{
	ArenaGenLog_Info("Building Walls...");
	//UE_LOG(LogArenaGenerator, Log, TEXT("Building Wall..."));

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
					+ (bBringWallBack ? SideAngleRV * -WallMeshSize.Y : FVector(0))

				,WallMeshScale);

				//TODO pattern matching for wall meshes
				int WallMeshIdx = 0;

				//All good, Add instance
				if (WallMeshInstances[WallMeshIdx]) {
					WallMeshInstances[WallMeshIdx]->AddInstance(WallTileTransform);
				}
				else {
					ArenaGenLog_Error("Could not find Wall Mesh Instance index: %d", WallMeshIdx);

				}

			}
		}
	}

}

void ABaseArenaGenerator::BuildRoof()
{
	ArenaGenLog_Info("Building Roofs...");


	for (UStaticMesh* Mesh : RoofMeshes) {
		UInstancedStaticMeshComponent* InstancedMesh =
			NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

		InstancedMesh->SetStaticMesh(Mesh);
		if (RoofMaterial) {
			InstancedMesh->SetMaterial(0, RoofMaterial);
		}
		InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		InstancedMesh->RegisterComponent();

		RoofMeshInstances.Add(InstancedMesh);
	}

	if (bBuildRoofAsCone) {

		//Get difference of wall and roof mesh scales to offset pieces correctly
		float MeshScalar = (WallMeshSize.X == RoofMeshSize.X) ? 1.f : WallMeshSize.X / RoofMeshSize.X;
		int RoofTilesPerSide = (WallMeshSize.X == RoofMeshSize.X) ? TilesPerArenaSide : //FMath::Clamp(TilesPerArenaSide * MeshScalar, 1, MaxTilesPerSideRow);
			FMath::Clamp(FMath::Floor((2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f)) / RoofMeshSize.X), 1, MaxTilesPerSideRow);

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

			for (int RoofLen = 0; RoofLen < RoofTilesPerSide; ++RoofLen) {

				LastCachedPosition = (RoofSideAngleFV * FVector(RoofMeshSize.X, RoofMeshSize.X, 0)) * (RoofLen > 0 ? 1 : 0) + LastCachedPosition;

				for (int RoofHeightIdx = 0; RoofHeightIdx < RoofTileHeight; ++RoofHeightIdx) {

					//Cache some random value between 0 & 3 to use for rotating roof pieces
					int RandomVal = bRoofRotates ? ArenaStream.RandRange(0, 3) : 0;

					FTransform RoofTileTransform = FTransform(
						FRotator(0.f, RoofYawRotation + (bMoveRoofWhenRotated ? (bRoofRotationIsQuad ? 90.f * RandomVal : ArenaStream.FRandRange(0.f, 360.f)) : 0.f), 0.f),
						LastCachedPosition //LastCachedPosition
						+ RoofOriginOffset //Offset and position
						+ FVector(0, 0, RoofMeshSize.Z * RoofHeightIdx) // Height Adjustment
						+ (bBringRoofForward ? RoofSideAngleRV * WallMeshSize.Y : (bBringRoofBack ? RoofSideAngleRV * -WallMeshSize.Y : FVector(0))) //If bringing roof forward or back
						+ (bRoofIncrementsForwardEachLevel ? (RoofSideAngleRV * RoofHeightIdx * RoofMeshSize.Y) : FVector(0))//if we move it forward each level
						+ (bRoofRotates && bMoveRoofWhenRotated ? OffsetMeshAlongDirections(RoofSideAngleFV, RoofSideAngleRV, RoofOriginType, RoofMeshSize, RandomVal) : FVector(0))//Location offsets from random rotation


					,RoofMeshScale);

					int RoofMeshIdx = 0;

					if (RoofMeshInstances[RoofMeshIdx]) {
						RoofMeshInstances[RoofMeshIdx]->AddInstance(RoofTileTransform);
					}
					else {
						ArenaGenLog_Error("Could not find Roof Mesh Instance index: %d", RoofMeshIdx);

					}

				}
			}
		}
	}
	else {

		//Cache midpoint index
		int RoofMidpoint = FMath::Floor(ArenaDimensions / 2);

		int RoofDimensions = (FloorMeshSize.X == RoofMeshSize.X) ? ArenaDimensions :
			FMath::Floor((2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f)) / RoofMeshSize.X);

		for (int Row = 0; Row < RoofDimensions; Row++) {
			for (int Col = 0; Col < RoofDimensions; Col++) {

				//Cache some random value between 0 & 3 to use for rotating roof pieces
				int RandomVal = bRoofRotates ? ArenaStream.RandRange(0, 3) : 0;

				FTransform RoofTileTransform = FTransform(
					FRotator((bFlipRoofMeshes ? 180.f : 0), 90.f * RandomVal, 0), //Rotation
					RoofOriginOffset + FVector(RoofMeshSize.X * Row * RoofMeshScale.X, RoofMeshSize.Y * Col * RoofMeshScale.Y, 0)
					+ RotatedMeshOffset(RoofOriginType, RoofMeshSize, RandomVal) //Location offsets from rotation
					+ (bWarpRoofPlacement ? PlacementWarping(RoofMidpoint, RoofMidpoint, Col, Row, RoofWarpRange, RoofWarpConcavityStrength, FVector(0, 0, 1)) : FVector(0))
					, RoofMeshScale
				);

				//TODO - roof pattern idx
				int RoofMeshIdx = 0;

				if (RoofMeshInstances[RoofMeshIdx]) {
					RoofMeshInstances[RoofMeshIdx]->AddInstance(RoofTileTransform);
				}
				else {
					ArenaGenLog_Error("Could not find Roof Mesh Instance index: %d", RoofMeshIdx);
				}
			}
		}

	}
}
*/
