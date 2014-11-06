all: coordinator participant

coordinator: coordinator.c
	gcc -o coordinator coordinator.c -pthread
	
participant: participant.c
	gcc -o participant participant.c -pthread

clean:
	rm coordinator participant
