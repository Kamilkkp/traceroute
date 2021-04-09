all :  traceroute

traceroute :  main.o traceroute.o
	gcc -std=gnu99 -Wall -Wextra -o traceroute main.o traceroute.o

main.o: main.c traceroute.h
	gcc -std=gnu99 -Wall -Wextra -c main.c

traceroute.o : traceroute.c traceroute.h
	gcc -std=gnu99 -Wall -Wextra -c traceroute.c


clean :
	rm -f traceroute.o main.o
distclean :
	rm -f traceroute traceroute.o main.o

