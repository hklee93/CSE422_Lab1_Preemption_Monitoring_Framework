all:
	cd mod && make
	cd user && make

clean:
	cd mod && make clean
	cd user && make clean
