# znc-idlerpg
ZNC module to handle logins to IdleRPG games/channels on IRC automatically. It can handle multiple channels on a network at the same time.

## Instructions
- Compile the module with `znc-buildmod idlerpg.cpp` and copy it to your modules directory (and/or refer to https://wiki.znc.in/Compiling_modules for help)
- Load the mod in each network your want it to be active via `/znc loadmod idlerpg`
- Setup your IRPG account(s) via `/znc *idlerpg add #channel irpgbotnickname username password`
- That's it!

## Functionality
It'll try to login when the bot joins the channel or if you join the channel (possibly after a reconnect of either). It'll wait 10 seconds after the join to login and only does so if the botnick is on the channel and has operator status (to make sure you are not sending your login to an imposter). Technically, most of it can be done via the perform module already, but this won't check if the bot actually is online and has operator status and also won't work if the bot reconnects and doesn't log you in automatically.