# TrickRabbitFreeRun

The Gold Leaf Star Bunnies in SMG2!

## Configuration

Obj_arg0: Star ID to spawn after being caught  
Obj_arg1: Color Frame  
Obj_arg2: Near Sound Radius  
Obj_arg3: Use Provoking Message?  
Obj_arg7: Camera Vector Register Index  

SW_APPEAR: If used, the rabbit vanishes after being talked to. It appears when this switch is activated.  
However, the rabbit will stay at whatever position it vanished at.

Message: The label is TrickRabbit%03d. Requires a flow of four messages:  
1. Before the minigame
2. Provoking message
3. Before spawning Power Star
4. Idle after being released

SW_PARAM: Animations and stuff are scaled by ParamScale when this switch is activated.

LinkNamePos: Uses "隠れ位置" as the hiding position. However, this does not currently work.

SW_B: Starts the minigame even if the rabbit hasn't been talked to. Alternatively gets activated when the minigame begins after being talked to.  
Multiple rabbits can be linked via their SW_B.

DemoSimpleCast: Continues movement during cutscenes.

## Future plans

- make the TrickRabbitFreeRunCollect work
