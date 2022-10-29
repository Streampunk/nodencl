# Instructions for building the CircleCI docker image for testing

- install docker desktop
- cd to this directory
- docker build -t streampunkmedia/testopencl:x.y.z .
- run container locally to check build
- push to Docker Hub
- update config.yml to pull new version tag
- push to git to trigger new build and test

(x: NodeAPI base version, y: Ubuntu base version, z: build number)

Intel have just made a big change to put OpenCL into their oneapi product. This should now mean that a rebuild of the container will pick up any updates.

See https://circleci.com/developer/images/image/cimg/node for CircleCI docker image tags
