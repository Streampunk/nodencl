FROM cimg/node:14.18

# install Intel CPU OpenCL driver
#  - use wget to fetch the Intel repository public key
RUN sudo apt-get update \
  && cd /tmp \
  && wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  && sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  && rm GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  && sudo add-apt-repository "deb https://apt.repos.intel.com/oneapi all main" \
  && sudo apt install intel-oneapi-runtime-libs \
  && sudo apt install intel-oneapi-runtime-opencl

# delete all the apt list files since they're big and get stale quickly
RUN sudo rm -rf /var/lib/apt/lists/*
# this forces "apt-get update" in dependent images, which is also good
