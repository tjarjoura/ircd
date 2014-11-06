An implementation of (parts of) the irc server protocol written in C. Currently in the process of debugging to the point that it works and supports basic join/privmsg messages.

## Features implemented:

1. Connection registration commands
2. Client data structure and functions
3. Channel data structure and functions
4. Main loop(select() based)
5. Sending/receiving correctly formatted IRC messages over the network
6. Join/part channels
7. Quit command
8. Welcome messages
