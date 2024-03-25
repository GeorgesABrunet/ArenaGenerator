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
#include "Containers/UnrealString.h"
#include "ArenaGeneratorSettings.h"

DECLARE_LOG_CATEGORY_EXTERN(LogArenaGenerator, Log, All);

bool ShowLogsOnScreen(float& Duration);

#if NO_LOGGING
#define _ArenaGenLog_PrivateImpl(ShowOnScreen, Color, Verbosity, Format, ...){}
#else
// Private implementation. Do not use it.
#define _ArenaGenLog_PrivateImpl(ShowOnScreen, Color, Verbosity, Format, ...) \
{	\
	UE_LOG(LogArenaGenerator, Verbosity, TEXT(Format), ##__VA_ARGS__) \
	float Duration; \
	if (ShowOnScreen && ShowLogsOnScreen(Duration)) \
	{ \
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
	} \
}
#endif // NO_LOGGING

// Logs info message to output and on screen
#define ArenaGenLog_Info(Format, ...) \
	_ArenaGenLog_PrivateImpl(true, FColor::White, Log, Format, ##__VA_ARGS__)

// Logs info message only to output
#define ArenaGenLog_InfoSilent(Format, ...) \
	_ArenaGenLog_PrivateImpl(false, FColor::White, Log, Format, ##__VA_ARGS__)

// Logs warning message to output and on screen
#define ArenaGenLog_Warning(Format, ...) \
	_ArenaGenLog_PrivateImpl(true, FColor::Yellow, Warning, Format, ##__VA_ARGS__)

// Logs warning message *only* to output
#define ArenaGenLog_WarningSilent(Format, ...) \
	_ArenaGenLog_PrivateImpl(false, FColor::Yellow, Warning, Format, ##__VA_ARGS__)

// Logs error message to output and on screen
#define ArenaGenLog_Error(Format, ...) \
	_ArenaGenLog_PrivateImpl(true, FColor::Red, Error, Format, ##__VA_ARGS__)
