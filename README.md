# Do Not Crash

Simple plugin for GTA III, VC and SA that puts the player into any vehicle they collide with while driving.

If the colliding vehicle has a driver, the driver of that vehicle will also swap to your old vehicle.

The GTA SA version disables automatically if SAMP is loaded.

You can configure the delay between swaps in the INI, as well as some other options.
Currently SwapTypes are not in use, as there are only vehicle to vehicle swaps.

# To Do

- Restore ped tasks in III and VC
- Add support for peds
  - In III and VC just change ped models and swap positions
  - In SA that may be more complex with CJ clothing etc.
- Add support for dynamic (moveable) objects
  - Controlled like in prophunt
  - Player dies if object is breakable and breaks
  - Make non-controlled breakable objects unbreakable
- Swap between peds, vehicles and objects respectively

# Dependencies/Credits

GTA SA Plugin SDK

https://github.com/DK22Pac/plugin-sdk/
