## Build An Image

The `Dockerfile` can be used to build a docker image that contains SPERR. The build command is:
```
docker build -t sperr-docker:tag1 .
```

The build process will take a few minutes. After a successful build, a docker image becomes
availabel (`docker images` will show).

One can simply create a container and experiment with SPERR:
```
docker run -it sperr-docker:tag1
```
One can also mount a directory from the host system to the container
so SPERR can operate on real data:
```
docker run -it --mount type=bind,source=/absolute/path/to/host/directory,target=/data,readonly sperr-docker:tag1
```

## Pull An Image from Docker Hub

One can also pull an image from Docker Hub at
[shaomeng/sperr-docker](https://hub.docker.com/r/shaomeng/sperr-docker).
```
docker pull shaomeng/sperr-docker:latest
```
