FROM debian:bookworm AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake build-essential flex git libabsl-dev libexpat1-dev \
    libgcrypt20-dev libgpg-error-dev libre2-dev libssl-dev libtool make \
    pkg-config procps python3 zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN bash bootstrap.sh \
 && ./configure --disable-libewf --enable-silent-rules \
 && make -j2

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libabsl20220623 libexpat1 libgcrypt20 libgpg-error0 libre2-9 \
    libssl3 libsqlite3-0 libstdc++6 zlib1g \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --create-home --uid 10001 bulk_extractor

COPY --from=build /src/src/bulk_extractor /usr/local/bin/bulk_extractor
USER bulk_extractor
WORKDIR /work
ENTRYPOINT ["bulk_extractor"]
