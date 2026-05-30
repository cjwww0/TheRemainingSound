# Codex Handoff (TRS)

This note is for resuming work after switching Codex/ChatGPT accounts on the same machine.

## Project

- UE project: `D:\AllAboutUE\UE5_Projects\TheRemainingSound\HorrorMechanics.uproject`
- Git remote: `cjwww0/TheRemainingSound.git`
- Working branch: `codex/trs-p0-phase0`

## Current Focus

- P14/P15: Workbench 5-slot system based on `BP_PuzzleActor`.
- Current testing uses `BP_CombinedCircle_ItemData` for pickup/place flow.

## Key Blueprints

- `Content/HorrorMechanics/Blueprint/Puzzle/BP_WorkbenchPanel_PuzzleActor.uasset`
- `Content/HorrorMechanics/Blueprint/BP_WorkbenchPickup_01..05.uasset` (workbench part pickups; key-only logic removed)
- `Content/HorrorMechanics/Blueprint/UI/BP_HorrorHUD.uasset`

## Key C++ (HorrorMechanics module)

- `Source/HorrorMechanics/Public/Puzzle/RemainWorkbenchPuzzleComponent.h`
- `Source/HorrorMechanics/Private/Puzzle/RemainWorkbenchPuzzleComponent.cpp`
- `Source/HorrorMechanics/Public/Puzzle/RemainWorkshopLightFaultController.h`
- `Source/HorrorMechanics/Private/Puzzle/RemainWorkshopLightFaultController.cpp`
- `Source/HorrorMechanics/Public/Puzzle/RemainPuzzleInventoryLibrary.h`
- `Source/HorrorMechanics/Private/Puzzle/RemainPuzzleInventoryLibrary.cpp`
- `Source/HorrorMechanics/Public/Debug/RemainInventoryDebugLibrary.h`
- `Source/HorrorMechanics/Private/Debug/RemainInventoryDebugLibrary.cpp`

## UE Build Command

Use this when UE is closed:

`D:\AllAboutUE\UE5.7.4\UE_5.7\Engine\Build\BatchFiles\Build.bat HorrorMechanicsEditor Win64 Development D:\AllAboutUE\UE5_Projects\TheRemainingSound\HorrorMechanics.uproject -WaitMutex -NoHotReloadFromIDE`

## Runtime Debug

- `URemainWorkbenchPuzzleComponent::BuildInventoryAcceptanceDebugString(Inventory)` prints:
  - current required class
  - inventory entry classes
  - extracted candidate classes
  - acceptance result per entry

