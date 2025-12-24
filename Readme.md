> [!WARNING]
> This is a WIP side project that is incomplete and insecure! It is only meant to be used as a playground for myself

> [!CAUTION]
> Code in this repository might be harmful. Only run and use things you understand!

# Malvex C2

<img src="./Resources/Logo.png" height="200px" width="auto" align="left" />

Welcome to the repository of the Malvex C2 framework. It is meant as an educational C2 Framework to show you how other C2 frameworks might work and operate.
It will also mature and be extended upon as time goes on. The goal is however to keep it as simple and easy to understand as possible.
So it might be used as a base for your very own C2 framework. The Ccurrent Implant only supports Linux based machines

![Screenshot](./Resources/Screenshot.png)

## Installation
1) Download the latest Release.zip
2) scp the server.elf onto your Linux based C2 server
3) run the server.elf and go through the setup process
4) Extract the client directory to your Linux PC
5) Run the client.elf from the extracted directory
6) Enter your server credentials
7) Under the "Builder" Tab in the client you can configure the implant
8) Once configured implant is run it will reach out to the C2 server and you will have full control over it

## Features
- easy installation
- easy configuation
- cool looking UI
- remote shell management
- credential stealer
- keylogger
- beaconing over HTTPS
- Browser Impersonation for Beaconing
- Systemd service based persistance

### Architecture Overview

![Screenshot](./Resources/Architecture.png)

### Database Overview

![Screenshot](./Resources/Database.png)

## Extending
This section might be populated later if there is enough legitimate interest.

### Custom Implants
### Adding Commands

## Credits
- Me, for coding this

## Contact
- **Discord:** @erarnitox