FROM ubuntu:23.10

SHELL ["/bin/bash", "-c"]

ARG USER=vscode

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    wget \
    git \
    build-essential \
    gcc-mipsel-linux-gnu \
    bear \
    gdb \
    clangd \
    && apt-get clean -y

# Create a new user and give them sudo rights
RUN useradd -d /home/$USER -s /bin/bash -m $USER && \
    echo "$USER ALL=NOPASSWD: ALL" >> /etc/sudoers

USER $USER
ENV HOME /home/$USER
WORKDIR $HOME

# Run bear to generate compile_commands.json
CMD bear -- make
