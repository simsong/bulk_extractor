.PHONY: build clean debug interactive

build:
	-@docker rm -f build-win32-container 2>/dev/null || true
	docker build -t build-win32 .
	docker run --name build-win32-container build-win32
	docker cp build-win32-container:/app/hello_world.exe .
	@docker rm -f build-win32-container

clean:
	-docker rm -f build-win32-container 2>/dev/null || true
	-docker rmi build-win32 2>/dev/null || true
	-rm -f hello_world.exe

interactive:
	docker run -it --rm --name debug-fedora fedora:latest bash
