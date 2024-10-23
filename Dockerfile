FROM jjangchan/cpp-tool:1.0.3

CMD ["/bin/bash"]

COPY ./ ./source/

RUN rm -rf /var/lib/apt/lists/*
RUN apt-get update -y
RUN cd ./source && mkdir build
RUN cd ./source/build && cmake .. && make
