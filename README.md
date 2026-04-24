**Ignite Game Studio - Technical Test**

*Pathfinding & Surface Navigation System*
*Implemented in Unreal Engine 5.7*

**Architecture Overview**

- The entire movement system is encapsulated in a single modular component (`ClimbingMovementComponent`)

**How to Test:**

*Controls (Keyboard + Mouse):*

- Left Mouse Click -> Move character to clicked location
- Q -> Rotate camera 90° left in Yaw
- E -> Rotate camera 90° right in Yaw
- R -> Reset camera to default Top-Down angle

**Test Scenario**

- The test scenario is located in the `L_Climbing` level
- Obstacles that the character **cannot** climb are shown in **Red**
- Everything else is climbable

**Debug Visualization**

- Enable or disable debug traces, on-screen prints, and logs via the **`bShowDebug`** boolean
- Exposed inside `BP_ClimbingCharacter` → `ClimbingMovementComponent` → **Debug** category

<img width="1909" height="966" alt="image" src="https://github.com/user-attachments/assets/298af427-24ca-4e6d-a034-6e5506b4e68f" />
