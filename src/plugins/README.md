# Configurations of All Plugins

## @arealoot.c
**Command Usage:**
```
	@arealoot
```
Allows to autoloot items in area defined by admin, when someone picks an item.

For Changing the Arealoot Range: Either change it via source:
```c
int arealoot_range = 3;		(x by x In Range)
```
Or Add this to any one of conf file in conf/battle

	//Arealoot, determine the range of arealoot
	//Min: 1, Max: 10
	//Default: 3
	//2: 2x2 range, 3: 3x3 range
	//arealoot_range: 3
    
## Critical-Magic.c
Allows to damage Critical.

Or Add this to any one of conf file in conf/battle

	//x can be 1 or 2
	//1 = Blue Critical
	//2 = Red Critical
	magic_critical_color: 1

## afk.c

**Command:** @afk

**battle_configs:**
- afk_timeout: seconds

**Mapflags:**
- noafk

**P.S:** (Mapflags don't work with setmapflag commands just parsing it with script works)

## auraset.c

**Command:** @aura <aura1> <aura2> <aura3>

(Aura2 and Aura3 are optional, -1 = don't change value)

**ScriptCommand:** aura(<aura1>{,<aura2>{,<aura3>}});

## autonext.c
Automatically goes to next screen.

## dispbottom2.c
**ScriptCommand:** Format : dispbottom2("0xFF00FF","Message"{,"Player Name"});




