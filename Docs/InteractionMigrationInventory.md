# Template Interaction Migration Inventory

This document tracks the migration from the original HorrorMechanics demo gameplay space into the imported final scenes. The rule for this phase is conservative: preserve template logic, do not batch delete old content, and use proxy/gameplay actors positioned against the new scene art.

## Migration Rules

- Keep framework classes unchanged: `BP_HorrorGameMode`, `BP_HorrorHUD`, `CH_PlayerCharacter`, `BP_HorrorGameState`, `BP_HorrorGameInstance`.
- Keep imported scene art in place. Move gameplay actors and proxy collision to match the new art, not the other way around.
- Do not directly modify imported art meshes for gameplay logic. Use template actors or proxy actors that reference/drive scene meshes.
- Old template actors should be moved or hidden under `Template_Reference` only after their replacement is verified.
- New gameplay actors should be grouped under `Gameplay_Logic`.
- Checkpoint GUIDs are gameplay identity. If an actor is duplicated/replaced, verify save/load behavior before treating it as migrated.

## Status Legend

- `MainRoute`: should be on the critical playable route.
- `Optional`: preserved in the new scene but not required for the main route.
- `DebugOnly`: kept outside the player path for testing template behavior.
- `TemplateReference`: keep hidden/grouped as old reference.
- `ManualPlacement`: requires UE viewport alignment against new art.
- `CodeReady`: logic exists and should only need placement/config.

## Core Framework

| Area | Assets / Classes | Current Status | Migration Action | Validation |
| --- | --- | --- | --- | --- |
| Game mode | `/Game/HorrorMechanics/Blueprint/Game/BP_HorrorGameMode` | CodeReady | Keep as map GameMode. Do not replace. | PIE spawns player and loads checkpoints. |
| Player | `/Game/HorrorMechanics/ExampleAssets/Blueprint/Characters/CH_PlayerCharacter` | CodeReady | Keep player class. Move `PlayerStart` to final scene. | Walk-only movement, interact ray, HUD all work. |
| HUD/UI | `BP_HorrorHUD`, `UI_MainHUD`, `UI_PuzzleScreen`, inventory/document widgets | CodeReady | Keep existing UI stack. | Interaction prompts, inventory, documents, puzzle UI show correctly. |
| Game state | `BP_HorrorGameState`, `BP_Inventory`, checkpoint data structs | CodeReady | Keep as global runtime state. | Inventory and checkpoint restore work. |

## Template Interaction Inventory

| Category | Primary Assets | Status | Placement Strategy | Notes |
| --- | --- | --- | --- | --- |
| Document pickups | `BP_Document`, document data tables, `UI_DocumentScreen` | MainRoute / Optional | Place document actors at final-scene reading locations. | Existing document dizziness and close handling must remain connected through `RemainNarrativeState`. |
| Inventory pickups | `BP_InventoryItem`, `BP_InventoryKey`, `BP_SimpleCollectible`, `BP_WorkbenchPickup_01..05` | MainRoute / Optional | Use existing pickup actors or subclassed variants; do not hard-code new item behavior in scene art. | Workbench parts now use `WorkbenchPart_ItemData_01..05`. |
| Doors | `BP_DoorAbstractClass`, `BP_AnimatedDoor`, `BP_BidirectionalDoor` | MainRoute / Optional | For moving doors, use template door actors aligned to final-scene openings. For static art doors, place proxy trigger and drive a movable door mesh. | Door actor collision/overlap volumes must be hand aligned. |
| Furniture / drawers | `BP_Furniture`, drawer/cabinet setup structs and curves | Optional / DebugOnly until placed | Use template furniture actor only where final art has movable drawers/cabinets. | Requires manual component mapping for drawer mesh, door hinge, key GUID, and jammed state. |
| Puzzle base | `BP_PuzzleActor`, `UI_PuzzleScreen`, `BP_ExaminableSpot` | CodeReady | Keep as base for all puzzle-mode interactions. | `PuzzleModeActivated`, `RequestChooseItem`, `UseItem`, `FinalizePuzzle` are the main hooks. |
| Electronic keypad | `BP_Keypad_PuzzleActor`, keypad mesh/materials | MainRoute candidate | Place on final-scene door/wall as a puzzle actor. | Needs final password/design assignment. |
| Combination lock | `BP_CombinationLock_PuzzleActor`, padlock/lock assets | Optional / DebugOnly | Preserve in debug area unless a story lock needs it. | Good regression test for puzzle input. |
| Circle/slot puzzle | `BP_CirclePanel_PuzzleActor`, `BP_WorkbenchPanel_PuzzleActor` | MainRoute for workbench | Keep workbench as current `BP_WorkbenchPanel_PuzzleActor`. | P14/P15 current logic is code-backed and should be placed in workshop. |
| Examinable objects | `BP_AbstractExaminableObject`, `BP_ExaminableObject`, `UI_ExaminingScreen` | MainRoute / Optional | Use for inspectable story props and inventory pickups. | Requires visual placement and collision check. |
| Wieldables | `BP_WieldableActor`, `BP_Flashlight_ItemData`, `BP_Pistol_ItemData`, `BP_Axe_ItemData`, weapon actors | Optional / DebugOnly unless design requires | Preserve weapon/equipment chain in debug area first. Promote to main route only with design approval. | Weapon systems can affect horror pacing; keep out of main path by default. |
| Flashlight | `BP_Flashlight_ExaminableActor`, `BP_Flashlight_WieldableActor` | MainRoute candidate | Place as pickup if final scene requires darkness navigation. | Validate equip/toggle and battery item behavior. |
| Switch/lights | `BP_Switch`, `BP_EmissiveMeshLight`, `BP_EventLight`, `BP_WorkshopLightFaultController` | MainRoute | Use proxy controller for imported scene lights. | P15 controller should drive final scene light refs, not imported meshes directly unless movable. |
| Save/checkpoint | `BP_CheckpointTrigger`, `BP_SaveMachine`, `BP_ActorCheckpointInterface` | MainRoute | Put checkpoint volumes at route milestones. | Verify GUID and actor state restore after migration. |
| P10 panel shock | `BP_PanelPickupShockController`, `RemainPickupShockController`, `RemainBreakableSwapComponent` | MainRoute, partial | Keep current controller. Cup break is deferred until Chaos setup is stable. | Screen flash works; Chaos cup impulse/fracture remains unresolved. |
| P11 nurse encounter | `BP_NurseEncounter_01`, `RemainNurseEncounterActor` | MainRoute | Place at living-room doorway or final equivalent. | Trigger after P10, disappear after configured duration, proximity feedback works. |
| P14/P15 workbench | `BP_WorkbenchPanel_PuzzleActor`, `BP_WorkbenchPickup_01..05`, violin part actors | MainRoute | Place workbench in final workshop; place parts sequentially in final route. | Pickups are hidden/unlocked sequentially; workbench filters current required part. |

## MainRoute Initial Placement Targets

| Order | Gameplay Beat | Actor(s) | Target Zone | Placement Type |
| --- | --- | --- | --- | --- |
| 1 | Spawn into final scene | `PlayerStart`, `CH_PlayerCharacter` | Final scene start room | ManualPlacement |
| 2 | Opening exploration documents | `BP_Document` variants | Living room / hallway / workshop | ManualPlacement |
| 3 | P10 shock | `BP_PanelPickupShockController`, target panel pickup, flash/camera shake/audio refs | Living room panel pickup location | ManualPlacement |
| 4 | P11 first nurse | `BP_NurseEncounter_01` | Living room doorway or equivalent sightline | ManualPlacement |
| 5 | Workbench route | `BP_WorkbenchPanel_PuzzleActor`, `BP_WorkshopLightFaultController` | Workshop workbench | ManualPlacement |
| 6 | Five violin parts | `BP_WorkbenchPickup_01..05`, `BP_ViolinPart_*` visuals | Route locations across final scenes | ManualPlacement |
| 7 | Door/key gating | `BP_DoorAbstractClass`, `BP_InventoryKey` or virtual key | Main route gates | ManualPlacement |
| 8 | Keypad or lock puzzle | `BP_Keypad_PuzzleActor` or `BP_CombinationLock_PuzzleActor` | One designed lock location | ManualPlacement |
| 9 | Checkpoints | `BP_CheckpointTrigger`, optional `BP_SaveMachine` | After major events | ManualPlacement |

## Optional / Debug Area

These should be preserved outside the formal player path until their design use is confirmed:

- `BP_DrawersExample`, `BP_DoorsExample`, `BP_Furniture` variants.
- `BP_Chain_PuzzleActor`, `BP_HiddenItem_PuzzleActor`, `BP_PocketWatch_PuzzleActor`, `BP_SmallWoodenChest_PuzzleActor`.
- `BP_Axe_Weapon`, `BP_Pistol_Weapon`, related ammo/data pickups.
- Extra `BP_Document` samples and inventory test pickups.

## Manual Placement Checklist

For every migrated interaction actor:

- Actor is in folder `Gameplay_Logic` or `Gameplay_Logic/<Category>`.
- Old equivalent actor is hidden/grouped under `Template_Reference` only after replacement works.
- Collision surface is visible to the player's interact ray.
- Prompt text appears when aiming at the intended point.
- Interaction works with keyboard `E` and expected UI mode.
- If the actor implements checkpoint behavior, save/load restores its state.
- If the actor references imported scene meshes/lights, those refs are valid after reopening the editor.

## Next Data Needed From UE

Run the editor script at `Scripts/Unreal/export_interaction_actor_inventory.py` after opening the project. It writes CSV files under `Saved/Migration/`:

- `interaction_actor_inventory_all.csv`: all actors in configured maps.
- `interaction_actor_inventory_filtered.csv`: likely gameplay/interaction actors only.

Use the filtered CSV as the placement worklist, then move actors in the UE viewport rather than guessing coordinates from code.

## Latest Export Snapshot

Generated on 2026-06-05 with `Scripts/Unreal/export_interaction_actor_inventory.py`.

| Map | Likely Interaction Actors |
| --- | ---: |
| `/Game/HorrorMechanics/Demo/Maps/DemoScene_01` | 66 |
| `/Game/HorrorMechanics/Demo/Maps/FinalHouse` | 0 |

Top detected template actor groups:

| Actor Group | Count |
| --- | ---: |
| `BP_Document` | 16 |
| `BP_InventoryItem` | 11 |
| `BP_DoorWall` | 7 |
| `BP_ExaminableObject` | 5 |
| `BP_BidirectionalDoor` | 4 |
| `BP_InventoryKey` | 3 |
| `BP_Switch` | 2 |
| `BP_DoorsExample` | 2 |
| `BP_WorkbenchPanel_PuzzleActor` | 1 |
| `BP_PanelPickupShockController` | 1 |
| `BP_NurseEncounter_01` | 1 |
| `BP_Keypad_PuzzleActor` | 1 |
| `BP_CombinationLock_PuzzleActor` | 1 |
| `BP_CirclePanel_PuzzleActor` | 1 |
| `BP_CheckpointTrigger` | 1 |
| `BP_SaveMachine` | 1 |
| `BP_Flashlight_ExaminableActor` | 1 |
| `BP_Pistol_ExaminableActor` | 1 |
