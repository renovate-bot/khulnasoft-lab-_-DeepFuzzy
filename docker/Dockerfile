# -- AFL
FROM deepfuzzy-base AS AFL
COPY ./docker/install_afl.sh ./
ARG make_j
RUN bash install_afl.sh $make_j

# -- Honggfuzz
# FROM deepfuzzy-base AS Honggfuzz
# COPY ./docker/install_honggfuzz.sh ./
# ARG make_j
# RUN bash install_honggfuzz.sh $make_j

# -- Eclipser
FROM deepfuzzy-base AS Eclipser
COPY ./docker/install_eclipser.sh ./
ARG make_j
RUN bash install_eclipser.sh $make_j

# -- Angora
# FROM deepfuzzy-base AS Angora
# COPY ./docker/install_angora.sh ./
# ARG make_j
# RUN bash install_angora.sh $make_j

# -- DeepFuzzy
FROM deepfuzzy-base
ENV DEPS_DIR /home/user/deps
ARG make_j

# Angr, Manticore
# RUN echo 'Installing angr and figurative' \
#    && sudo add-apt-repository -y ppa:sri-csl/formal-methods \
#    && sudo apt-get -y update \
#    && sudo apt-get -y install yices2 \
#    && pip3 install z3-solver angr git+git://github.com/khulnasoft-lab/figurative.git --user

# Eclipser - not deepfuzzy dependent
COPY --from=Eclipser /home/user/Eclipser/build $DEPS_DIR/eclipser
RUN echo 'Eclipser - installing dotnet' \
    && wget -q https://packages.microsoft.com/config/ubuntu/18.04/packages-microsoft-prod.deb \
    && sudo dpkg -i packages-microsoft-prod.deb \
    && sudo apt-get update \
    && sudo apt-get install -y dotnet-sdk-2.2
COPY --from=Eclipser /home/user/.nuget /home/user/.nuget

# Angora part 1 - not deepfuzzy dependent
# COPY --from=Angora /home/user/Angora $DEPS_DIR/angora
# COPY --from=Angora /home/user/clang+llvm $DEPS_DIR/angora/clang+llvm

# copy deepfuzzy code here to use cache as much as possible
COPY . ./deepfuzzy
RUN sudo chown user:user -R ./deepfuzzy
WORKDIR ./deepfuzzy

# Angora part 2
# ignore errors in `make`, because Angora doesn't support 32bit builds
# RUN echo 'Building deepfuzzy with Angora - taint' \
#    && mkdir -p build_angora_taint && cd build_angora_taint \
#    && export PATH="$DEPS_DIR/angora/clang+llvm/bin:$PATH" \
#    && export LD_LIBRARY_PATH="$DEPS_DIR/angora/clang+llvm/lib:$LD_LIBRARY_PATH" \
#    && export USE_TRACK=1 \
#    && export ANGORA_HOME="$DEPS_DIR/angora" \
#    && CXX="$ANGORA_HOME/bin/angora-clang++" CC="$ANGORA_HOME/bin/angora-clang" cmake -DDEEPFUZZY_ANGORA=ON ../ \
#    && make -j $make_j -i \
#    && sudo cp ./libdeepfuzzy_taint.a /usr/local/lib/

# RUN echo 'Building deepfuzzy with Angora - fast' \
#    && mkdir -p build_angora_fast && cd build_angora_fast \
#    && export PATH="$DEPS_DIR/angora/clang+llvm/bin:$PATH" \
#    && export LD_LIBRARY_PATH="$DEPS_DIR/angora/clang+llvm/lib:$LD_LIBRARY_PATH" \
#    && export USE_FAST=1 \
#    && export ANGORA_HOME="$DEPS_DIR/angora" \
#    && CXX="$ANGORA_HOME/bin/angora-clang++" CC="$ANGORA_HOME/bin/angora-clang" cmake -DDEEPFUZZY_ANGORA=ON ../ \
#    && make -j $make_j -i \
#    && sudo cp ./libdeepfuzzy_fast.a /usr/local/lib/

# general
RUN echo 'Building deepfuzzy' \
    && mkdir -p ./build_deepfuzzy && cd ./build_deepfuzzy \
    && CXX=clang++ CC=clang cmake ../ \
    && make -j $make_j \
    && sudo make install

# libFuzzer
RUN echo 'Building deepfuzzy with libFuzzer' \
    && mkdir -p build_libfuzzer && cd build_libfuzzer \
    && CXX=clang++ CC=clang cmake -DDEEPFUZZY_LIBFUZZER=ON ../ \
    && make -j $make_j \
    && sudo cp ./libdeepfuzzy_LF.a /usr/local/lib/

# AFL
COPY --from=AFL /home/user/afl-2.52b $DEPS_DIR/afl
RUN echo 'Building deepfuzzy with AFL' \
    && mkdir -p build_afl && cd build_afl \
    && export AFL_HOME="$DEPS_DIR/afl" \
    && CXX="$AFL_HOME/afl-clang++" CC="$AFL_HOME/afl-clang" cmake -DDEEPFUZZY_AFL=ON ../ \
    && make -j $make_j \
    && sudo cp ./libdeepfuzzy_AFL.a /usr/local/lib/

# Honggfuzz
# COPY --from=Honggfuzz /home/user/honggfuzz $DEPS_DIR/honggfuzz
# RUN sudo apt-get update && sudo apt-get -y install libunwind-dev
# RUN echo "HELLO SAILOR" \
# RUN echo 'Building deepfuzzy with Honggfuzz' \
#    && mkdir -p build_honggfuzz && cd build_honggfuzz \
#    && export HONGGFUZZ_HOME="$DEPS_DIR/honggfuzz" \
#    && CXX="$HONGGFUZZ_HOME/hfuzz_cc/hfuzz-clang++" CC="$HONGGFUZZ_HOME/hfuzz_cc/hfuzz-clang" cmake -DDEEPFUZZY_HONGGFUZZ=ON ../ \
#    && make -j $make_j \
#    && sudo cp ./libdeepfuzzy_HFUZZ.a /usr/local/lib/

ENV CXX=clang++ CC=clang
ENV AFL_HOME="$DEPS_DIR/afl" HONGGFUZZ_HOME="$DEPS_DIR/honggfuzz" \
    ANGORA_HOME="$DEPS_DIR/angora" ECLIPSER_HOME="$DEPS_DIR/eclipser"

CMD ["/bin/bash"]