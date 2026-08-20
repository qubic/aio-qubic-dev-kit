# Build core-lite as an unattended long-run local testnet node.
# Build context must be the core-lite repo root.
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    cmake \
    nasm \
    libc++-dev \
    libc++abi-dev \
    libjsoncpp-dev \
    libhiredis-dev \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    libboost-stacktrace-dev \
    libzstd-dev \
    libfmt-dev \
    liblzo2-dev \
    liblz4-dev \
    libc-ares-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build/core-lite
COPY . .

# Ticks per epoch before forced seamless epoch transition (default: 60 days at 1s ticks)
ARG EPOCH_TICK_CAPACITY=5184000
# OFF: balances come solely from the spectrum.<EPOCH> state file loaded at genesis (./states)
ARG PREFILL_QUS=OFF
# ON: shrink node-local buffers (score cache, dejavu, logging/swap caches, oracle,
# miner flags) — cuts RAM from ~30 GB to a fraction. Stock LITE_RAM also shrinks
# spectrum/assets depth 24->18, which breaks bob's wire structs and state files;
# the sed below pins depth at 24 so bob compatibility is preserved.
ARG LITE_RAM=ON
# Empty (default): no oracle machine, zero address never dialed. Set to the
# oracle-machine service IP (compose: ORACLE_MACHINE_IP) to compile it in —
# oracleMachineIPs is a compile-time table in private_settings.h.
ARG ORACLE_MACHINE_IP=""

RUN sed -i 's/#define SPECTRUM_DEPTH 18 .*/#define SPECTRUM_DEPTH 24/; s/#define ASSETS_DEPTH 18 .*/#define ASSETS_DEPTH 24/' \
    src/network_messages/common_def.h

RUN if [ -n "$ORACLE_MACHINE_IP" ]; then \
        octets=$(echo "$ORACLE_MACHINE_IP" | tr '.' ','); \
        sed -i "s|{0, 0, 0, 0}, // no oracle machine.*|{${octets}}, // oracle machine (ORACLE_MACHINE_IP build arg)|" \
            src/private_settings.h; \
        grep -q "{${octets}}, // oracle machine" src/private_settings.h; \
    fi

# Logging events stay enabled: bob requires them to index.
RUN rm -rf build \
    && mkdir -p build && cd build \
    && cmake .. \
        -D CMAKE_C_COMPILER=clang \
        -D CMAKE_CXX_COMPILER=clang++ \
        -D BUILD_TESTS:BOOL=OFF \
        -D BUILD_BINARY:BOOL=ON \
        -D CMAKE_BUILD_TYPE=Release \
        -D ENABLE_AVX512=OFF \
        -D LONG_RUN_LOCAL_TESTNET=ON \
        -D LONG_RUN_EPOCH_TICK_CAPACITY=${EPOCH_TICK_CAPACITY} \
        -D TESTNET_PREFILL_QUS=${PREFILL_QUS} \
        -D TESTNET_LITE_RAM=${LITE_RAM} \
    && cmake --build . --target Qubic -- -j$(nproc)

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libc++1 \
    libc++abi1 \
    libjsoncpp25 \
    libhiredis1.1.0 \
    libssl3 \
    libzstd1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/core-lite/build/src/Qubic /qubic/Qubic
RUN chmod +x /qubic/Qubic

# Node writes all state (spectrum, universe, system, swap) to cwd
WORKDIR /qubic/data

# 31841 = P2P (testnet), 41841 = HTTP RPC/explorer
EXPOSE 31841 41841

ENTRYPOINT ["/qubic/Qubic"]
CMD ["--tick-duration", "1000"]
