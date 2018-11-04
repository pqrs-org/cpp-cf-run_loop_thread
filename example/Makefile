all:
	mkdir -p build \
		&& cd build \
		&& cmake .. \
		&& make

clean:
	rm -rf build

run:
	./build/example

update_vendor:
	cget install -f cget-requirements.txt --prefix vendor
