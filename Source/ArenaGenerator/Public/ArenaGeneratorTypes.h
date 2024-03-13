// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//This will address parameters of arena generation by data tables 
UENUM(BlueprintType, meta = (DisplayName = "Build Rule Presets"))
enum class EPresetBuildRules : uint8
{
	None,
	Colosseum,
	Cave,
	Plane,
};


