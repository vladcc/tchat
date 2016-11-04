# tchat
Chat 1 on 1 over LAN in the terminal

Hey there! Welcome to the readme.

This project is my first attempt in socket programming. I've read up on data abstraction and project planning since my last endeavour,
and what do you know? Thinking before writing really does make things eaiser.

With tchat you can chat 1 on 1 with your favorite person in your local network.
It works for Windows and Linux and can be build with the win_tchat.make and lin_tchat.make files respectively.
Don't forget to define the operating system in os_def.h first. For Windows I've used TDM-GCC and mingw32-make.

To use tchat one person has to host with tchat -h, and the other can connect with tchat -c ip/hostname.
Your hostname is your default chat username. You can specify another one with -u. The default TCP port is 5555 and can be
changed with the -p flag. For more info type tchat -?

Thanks for reading!
