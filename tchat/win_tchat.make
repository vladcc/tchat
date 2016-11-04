# links to the winsock library
tchat: tchat.o network.o display.o
	gcc ./ofiles/tchat.o ./ofiles/network.o ./ofiles/display.o -o tchat -lWs2_32 -s

tchat.o: tchat.c
	gcc tchat.c -c -o ./ofiles/tchat.o
	
network.o: network.c
	gcc network.c -c -o ./ofiles/network.o

display.o: display.c
	gcc display.c -c -o ./ofiles/display.o
