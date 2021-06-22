# znc-idlerpg
ZNC module to handle logins to an IdleRPG game/channel on IRC automatically

## Instructions
- Can be loaded as a network module
- Compile the module with `znc-buildmod idlerpg.cpp` and copy it to your modules directory
- Load the mod in each network your want it to be active via `/znc loadmod idlerpg`
- Setup your IRPG account via `/znc *idlerpg set #channel irpgbotnickname username password`
- That's it!

## Functionality
Trying to login if the bot joins the channel or if you join the channel (possibly after a disconnect of either). It'll wait 10 seconds before it does after the join and only does so if the botnick is on the channel and has operator status (to make sure you are not sending your login to an imposter). Technically, most of it can be done via the perform module already, but this won't check if the bot actually is online and has operator status and also won't work if the bot reconnects and doesn't log you in automatically.
