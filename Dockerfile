FROM alpine:3.18 AS builder
RUN apk add --no-cache cmake make g++ musl-dev linux-headers
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)

FROM alpine:3.18
RUN apk add --no-cache libstdc++
COPY --from=builder /app/build/proxy /usr/local/bin/proxy
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/proxy"]
CMD ["-p","8080","-b","backend1:80,backend2:80"]