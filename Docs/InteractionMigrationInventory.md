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

## 2026-06-07 Re-Export Snapshot

`Scripts/Unreal/export_interaction_actor_inventory.py` now scans:

- `/Game/HorrorMechanics/Demo/Maps/DemoScene_01`
- `/Game/HorrorMechanics/Demo/Maps/FinalHouse`
- `/Game/Anemoia/MAIN/Maps/Demonstration`

Current result:

| Map | Total Actors | Likely Interaction Actors |
| --- | ---: | ---: |
| `/Game/HorrorMechanics/Demo/Maps/DemoScene_01` | 248 | 71 |
| `/Game/HorrorMechanics/Demo/Maps/FinalHouse` | 863 | 0 |
| `/Game/Anemoia/MAIN/Maps/Demonstration` | 2 | 0 |

Interpretation: the imported scenes are currently art/layout layers only. Main-route gameplay actors still live in `DemoScene_01` at template coordinates. The next implementation pass should therefore be interaction placement and regression, not new feature development.

## Immediate Main-Route Placement Pass

Goal: make the existing stage 0/1 route playable in the new scene before adding stage 2 features.

Work order:

1. Set `PlayerStart` in the intended new-scene start room and verify `BP_HorrorGameMode`, `CH_PlayerCharacter`, `BP_HorrorHUD`, and input are still active.
2. Create a `Gameplay_Logic` actor folder in `DemoScene_01` and move/place proxy gameplay actors there.
3. Place core story interactables first: radio, letter, diary/document, old photo, panel pickup, P10 controller, P11 nurse, workbench, workbench parts, and `BP_WorkshopLightFaultController`.
4. Place route gates after the story loop is stable: doors, keys, keypad/combination lock, checkpoint trigger, save machine.
5. Preserve optional template systems outside the main route: furniture/drawers, weapon/equipment pickups, extra documents, and sample puzzle actors.
6. After each placement group, run PIE and validate raycast prompt, `E` interaction, UI mode, inventory state, event trigger, and save/load state where applicable.

Do not start P16-P23 until this pass can be played from spawn through P14/P15 in the new scene.

## 2026-06-07 Main-Route Rough Placement

Implemented with `Scripts/Unreal/place_main_route_in_finalhouse.py`.

Output report:

- `Saved/Migration/main_route_placement_report.csv`

Result:

| Status | Count | Meaning |
| --- | ---: | --- |
| `moved` | 19 | Main-route actors moved from template coordinates into the `FinalHouse` world-space area. |
| `foldered` | 18 | Extra documents/items preserved in `Template_Reference` without changing their world position. |
| `missing` | 4 | Optional/debug duplicate labels were not present; no main-route dependency. |

Placed groups:

- `Gameplay_Logic/00_Player`: `Player Start2`.
- `Gameplay_Logic/01_Opening`: three opening documents and one opening inventory pickup candidate near the first-floor table area.
- `Gameplay_Logic/02_P10_P11`: `BP_PanelPickupShockController` near the table/cup area and `BP_NurseEncounter_01` near the entrance sightline.
- `Gameplay_Logic/03_Workbench`: `BP_WorkbenchPanel_PuzzleActor`, `BP_WorkshopLightFaultController`, and `BP_WorkbenchPickup_01..05`.
- `Gameplay_Logic/04_Gates_Checkpoints`: one checkpoint, save machine, keypad candidate, combination lock candidate, and `HM_BidirectionalDoor6`.

Important limitations:

- `FinalHouse` is a `LevelInstance`; its internal light actors are not persistent-level actors. `BP_WorkshopLightFaultController` was moved near the workshop but its final controlled-light references still need viewport verification or persistent proxy lights.
- This is a rough placement pass for regression. Visual composition, exact collision alignment, prompt wording, and route pacing still need manual viewport adjustment.
- P10 Chaos cup break remains deferred. For this pass, validate screen flash, audio/light feedback, and P11 trigger chain instead.

## 2026-06-07 Static Validation

Implemented with `Scripts/Unreal/validate_main_route_placement.py`.

Output report:

- `Saved/Migration/main_route_static_validation.csv`

Result:

| Status | Count | Meaning |
| --- | ---: | --- |
| `PASS` | 50 | Main-route actor existence, folder, bounds, tag, and workbench slot checks passed. |
| `WARN` | 1 | `BP_WorkshopLightFaultController.ControlledLights` is empty. Assign final-scene lights manually or use persistent proxy lights. |
| `FAIL` | 0 | No blocking static validation failure. |
| `MANUAL` | 8 | PIE-only checks that cannot be proven by static script. |

Manual checks still required:

- PIE spawn, camera, HUD, and player input.
- Interact trace is not blocked by imported `FinalHouse` collision.
- Opening documents can be opened and closed.
- Document dizziness triggers while document UI is still open.
- P10 screen flash/audio/light feedback after panel pickup.
- P11 nurse appears in the intended sightline and proximity feedback works.
- Workbench Choose UI opens only for the current required part.
- Save/load restores workbench and event state.

## 2026-06-07 PIE Regression Notes

User-verified:

- Spawn point was manually adjusted in the editor.
- Crosshair prompt and `E` interaction work in the new scene.
- Opening documents and in-document dizziness work.
- P11 nurse encounter works.
- Workbench first step works.
- Picking up the first workbench part triggers the P10 screen flash.

P10 cup diagnosis:

- `BP_PanelPickupShockController` has moved to the new scene.
- Its `BreakableCup` references still point to old-template actors:
  - `SM_GlassBottle` at `1160.000,-210.000,1.909`
  - `GC_SM_GlassBottle_Test` at `1160.000,-210.000,1.909`
- This explains why the P10 trigger works but the cup is not visible in the new scene.
- Use `Scripts/Unreal/inspect_p10_breakable_refs.py` to regenerate `Saved/Migration/p10_breakable_refs.csv`.

P10 cup placement fix:

- Implemented with `Scripts/Unreal/place_p10_cup_in_finalhouse.py`.
- `SM_GlassBottle` and `GC_SM_GlassBottle_Test` moved to `8336.684,301.052,-158.000`.
- Both actors are now in `Gameplay_Logic/02_P10_P11`.
- Existing `BreakableCup` references were preserved.
- Placement report: `Saved/Migration/p10_cup_placement_report.csv`.

## 2026-06-07 P15 Proxy Fault Lights

Implemented with `Scripts/Unreal/setup_p15_proxy_fault_lights.py`.

Created persistent-level proxy lights:

| Actor | Location | Normal Intensity | Fault Intensity |
| --- | --- | ---: | ---: |
| `P15_ProxyLivingRoomFaultLight` | `8335.000,285.000,45.000` | 4200 | 900 |
| `P15_ProxyWorkshopFaultLight` | `6730.000,650.000,675.000` | 3600 | 650 |

Result:

- `BP_WorkshopLightFaultController.ControlledLights` now has 2 configured light refs.
- The previous static warning for empty `ControlledLights` is resolved.
- Current remaining static warning is expected during testing: `BP_WorkbenchPanel_PuzzleActor` is outside the original rough second-floor bounds because it was temporarily moved near the spawn point for faster testing.
- Placement report: `Saved/Migration/p15_proxy_fault_lights_report.csv`.
