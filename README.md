# Do Not Crash

Simple plugin for GTA III, VC and SA that puts the player into any vehicle they collide with while driving.

If the colliding vehicle has a driver, the driver of that vehicle will also swap to your old vehicle.

The GTA SA version disables automatically if SAMP is loaded.

# Global config

If you want to have one config for all three games, create a folder called *GTA DoNotCrash* where the Grand Theft Auto III/Vice City/San Andreas folders are located.

Example: *C:/Program Files (x86)/GTA DoNotCrash/DoNotCrash.ini*.

# Per-Game config

To configure the games individually you can place the INI in the *scripts*, *plugins* or the respective GTA root directory of each GTA game.

The name of the ini file must be equal to the (default) asi file name:
- *DoNotCrash.III.ini*
- *DoNotCrash.VC.ini*
- *DoNotCrash.SA.ini*

If a config exists for both the game and in the *GTA DoNotCrash* directory, the game's INI will override the global one.

# Interference with missions

There are quite a few missions that are either a lot easier to play because the AI stops working, or incredibly hard (like Carmageddon (VC)). I aim to fix the AI as well as possible.
Take a look at the config to disable swapping to objective vehicles.

# To Do

- Restore ped tasks after swaps
- Add support for peds
  - In III and VC just change ped models and swap positions
  - In SA that may be more complex with CJ clothing etc.
- Add support for dynamic (moveable) objects
  - Controlled like in prophunt
  - Player dies if object is breakable and breaks
  - Make non-controlled breakable objects unbreakable
- Swap between peds, vehicles and objects respectively
- Prevent swapping to mission objective related vehicles and modifying objective related peds

# Dependencies/Credits

GTA SA Plugin SDK

https://github.com/DK22Pac/plugin-sdk/
